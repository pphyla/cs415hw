#!/bin/bash
# CIS 415 Lab 4 checker - Processes and the exec system call
#
# Usage: ./checker.sh [lab_dir] [n1 n2 ...]
#   lab_dir : directory containing lab4.c, iobound.c, Makefile (default: .)
#   n...    : process counts to test (default: 1 3 5; keep <= 7 per lab warning)
#
# The lab is built and run inside a temp copy, so nothing in lab_dir is modified.
# gnome-terminal is replaced by a stub so script_print() never opens windows.

LAB_DIR="$(cd "${1:-.}" && pwd)" || exit 1
shift
COUNTS=("$@")
[ ${#COUNTS[@]} -eq 0 ] && COUNTS=(1 3 5)
SECONDS_PER_CHILD=5

PASS=0
FAIL=0
pass() { echo "  [PASS] $1"; PASS=$((PASS + 1)); }
fail() { echo "  [FAIL] $1"; FAIL=$((FAIL + 1)); }
info() { echo "  [INFO] $1"; }

for n in "${COUNTS[@]}"; do
    if ! [[ "$n" =~ ^[1-9][0-9]*$ ]] || [ "$n" -gt 7 ]; then
        echo "Invalid process count '$n' (must be 1..7)"; exit 1
    fi
done

WORK="$(mktemp -d)"
trap 'pkill -P $$ 2>/dev/null; rm -rf "$WORK"' EXIT

# ---------------------------------------------------------------- setup
echo "== Setup"
for f in lab4.c iobound.c Makefile; do
    if [ -f "$LAB_DIR/$f" ]; then
        cp "$LAB_DIR/$f" "$WORK/"
    else
        fail "missing $f in $LAB_DIR"
    fi
done
[ $FAIL -gt 0 ] && { echo "Cannot continue."; exit 1; }

# Stub gnome-terminal: records its args instead of opening a window.
mkdir -p "$WORK/stubbin"
cat > "$WORK/stubbin/gnome-terminal" <<EOF
#!/bin/bash
echo "\$*" >> "$WORK/gnome_calls.log"
exit 0
EOF
chmod +x "$WORK/stubbin/gnome-terminal"
export PATH="$WORK/stubbin:$PATH"

cd "$WORK" || exit 1

# ---------------------------------------------------------------- build
echo "== Build"
rm -f lab4 iobound
if make > build.log 2>&1 && [ -x lab4 ] && [ -x iobound ]; then
    pass "make builds lab4 and iobound"
else
    fail "make failed"; cat build.log; exit 1
fi
if grep -qi "warning" build.log; then
    info "compiler warnings:"; grep -i "warning" build.log | sed 's/^/         /'
fi

# ---------------------------------------------------------------- source checks
echo "== Source checks (lab4.c)"
src_has() { grep -Eq "$1" lab4.c; }
src_has '\bfork[[:space:]]*\('                && pass "uses fork()"    || fail "fork() not found"
src_has '\bexecvp[[:space:]]*\('              && pass "uses execvp()"  || fail "execvp() not found"
src_has '\b(waitpid|wait)[[:space:]]*\('      && pass "uses wait()/waitpid()" || fail "wait()/waitpid() not found"
src_has 'pid_t[[:space:]]*\*|pid_t[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]*\[' \
                                              && pass "declares a pid_t array" || fail "no pid_t array/pointer found"
if src_has '\bmalloc[[:space:]]*\(' && ! src_has '\bfree[[:space:]]*\('; then
    fail "malloc() without free()"
else
    pass "dynamic memory freed (or none used)"
fi
src_has 'script_print[[:space:]]*\([^)]*\)[[:space:]]*;' \
                                              && pass "calls script_print()" || fail "script_print() is never called"

# ---------------------------------------------------------------- argument handling
echo "== Argument handling"
out=$(timeout 5 ./lab4 2>&1); rc=$?
if [ $rc -ne 124 ] && echo "$out" | grep -qi "wrong number of arguments"; then
    pass "no arguments -> 'Wrong number of arguments'"
else
    fail "no arguments should print 'Wrong number of arguments' and exit (got: $out)"
fi
out=$(timeout 5 ./lab4 1 2 2>&1); rc=$?
if [ $rc -ne 124 ] && echo "$out" | grep -qi "wrong number of arguments"; then
    pass "too many arguments -> 'Wrong number of arguments'"
else
    fail "too many arguments should print 'Wrong number of arguments' and exit"
fi

# ---------------------------------------------------------------- run tests
run_test() {
    local n=$1
    local out="run_$n.out" limit=$((SECONDS_PER_CHILD * n + 30))
    echo "== ./lab4 $n"
    rm -f top_script.sh gnome_calls.log

    # stdbuf -oL is inherited across exec, so iobound's printf lines arrive in order.
    stdbuf -oL -eL ./lab4 "$n" > "$out" 2>&1 &
    local lpid=$! peak=0 cnt start end
    start=$(date +%s.%N)

    # Sample live iobound children of lab4 to measure concurrency.
    while kill -0 "$lpid" 2>/dev/null; do
        cnt=$(pgrep -P "$lpid" -x iobound | wc -l)
        [ "$cnt" -gt "$peak" ] && peak=$cnt
        if [ "$(echo "$(date +%s.%N) - $start > $limit" | bc)" -eq 1 ]; then
            kill -9 "$lpid" 2>/dev/null; pkill -9 -x iobound -u "$(id -u)" 2>/dev/null
            fail "timed out after ${limit}s"; return
        fi
        sleep 0.1
    done
    wait "$lpid"; local rc=$?
    end=$(date +%s.%N)
    info "exit code $rc, wall time $(printf '%.1f' "$(echo "$end - $start" | bc)")s, peak concurrent iobound = $peak"

    # PIDs reported by iobound
    local begin_pids fin_pids
    begin_pids=$(grep -Eo 'Process: [0-9]+ - Begin+ing' "$out" | grep -Eo '[0-9]+' | sort -n)
    fin_pids=$(grep -Eo 'Process: [0-9]+ - Finished' "$out" | grep -Eo '[0-9]+' | sort -n)
    local nb nf
    nb=$(echo -n "$begin_pids" | grep -c .)
    nf=$(echo -n "$fin_pids" | grep -c .)

    [ "$rc" -eq 0 ] && pass "lab4 exits with status 0" || fail "lab4 exit status $rc"
    [ "$nb" -eq "$n" ] && pass "$n iobound processes started" || fail "expected $n iobound starts, got $nb"
    [ "$nf" -eq "$n" ] && pass "$n iobound processes finished before lab4 exited (parent waits)" \
                       || fail "expected $n iobound finishes when lab4 exited, got $nf (parent not waiting?)"
    [ "$(echo "$begin_pids" | sort -u | grep -c .)" -eq "$nb" ] && [ "$begin_pids" == "$fin_pids" ] \
        && pass "each child has a distinct PID and ran to completion" \
        || fail "started/finished PIDs do not match"

    # No child should still be alive after the parent returns.
    local alive=0 p
    for p in $begin_pids; do
        [ "$(ps -o comm= -p "$p" 2>/dev/null)" == "iobound" ] && alive=$((alive + 1))
    done
    [ $alive -eq 0 ] && pass "no iobound child outlives lab4" || fail "$alive iobound child(ren) still running after lab4 exited"

    # All children should run at the same time, not one after another.
    if [ "$n" -eq 1 ] || [ "$peak" -eq "$n" ]; then
        pass "all $n children ran concurrently"
    else
        fail "expected $n concurrent iobound children, observed at most $peak (forking and waiting one at a time?)"
    fi
    if [ "$n" -gt 1 ]; then
        local first_fin last_begin
        first_fin=$(grep -n 'Finished' "$out" | head -1 | cut -d: -f1)
        last_begin=$(grep -En 'Begin+ing' "$out" | tail -1 | cut -d: -f1)
        [ -n "$first_fin" ] && [ -n "$last_begin" ] && [ "$last_begin" -lt "$first_fin" ] \
            && pass "all children started before any finished" \
            || fail "some child started only after another finished (not concurrent)"
    fi

    grep -q "execvp failed\|exec.*No such file" "$out" && fail "exec error reported in output"

    # script_print: top_script.sh must list exactly the child PIDs.
    if [ -f top_script.sh ]; then
        local script_pids
        script_pids=$(grep -Eo -- '-p [0-9]+' top_script.sh | grep -Eo '[0-9]+' | sort -n)
        if head -1 top_script.sh | grep -q '^#!/bin/bash' && grep -q '^top' top_script.sh \
           && [ "$script_pids" == "$begin_pids" ]; then
            pass "top_script.sh lists all $n child PIDs"
        else
            fail "top_script.sh does not match child PIDs"; sed 's/^/         /' top_script.sh
        fi
    else
        fail "top_script.sh was not generated (script_print not called?)"
    fi
    if [ -f gnome_calls.log ] && grep -q -- '-- bash top_script.sh' gnome_calls.log; then
        pass "script_print launched 'gnome-terminal -- bash top_script.sh'"
    else
        fail "script_print did not exec gnome-terminal with top_script.sh"
    fi

    echo "  ---- program output ----"
    sed 's/^/  | /' "$out"
}

for n in "${COUNTS[@]}"; do
    run_test "$n"
done

# ---------------------------------------------------------------- memory check
if command -v valgrind > /dev/null; then
    echo "== Valgrind (./lab4 2)"
    valgrind --leak-check=full --errors-for-leak-kinds=definite --error-exitcode=42 \
        ./lab4 2 > valgrind.log 2>&1
    if [ $? -eq 42 ]; then
        fail "valgrind reports memory errors/leaks"
        grep -E "definitely lost|ERROR SUMMARY" valgrind.log | sed 's/^/         /'
    else
        pass "no definite leaks or memory errors"
    fi
else
    info "valgrind not installed; skipping memory check"
fi

# ---------------------------------------------------------------- summary
echo
echo "== Summary: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
