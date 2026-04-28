#!/bin/bash

PROG="./filediffadvanced"
PASS=0
FAIL=0
TMPDIR=$(mktemp -d)

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

echo "=== filediffadvanced test suite ==="
echo ""

# --- Test 1: Identical text files ---
echo "Test 1: Identical text files"
printf "line1\nline2\nline3\n" > "$TMPDIR/a.txt"
cp "$TMPDIR/a.txt" "$TMPDIR/b.txt"
$PROG -t "$TMPDIR/a.txt" "$TMPDIR/b.txt" > "$TMPDIR/out.txt"
rc=$?
if [ $rc -eq 0 ] && [ ! -s "$TMPDIR/out.txt" ]; then
    pass "identical files return 0 with no output"
else
    fail "identical files (exit=$rc)"
fi

# --- Test 2: Text files with differences ---
echo "Test 2: Text diff"
printf "line1\nline2\nline3\n" > "$TMPDIR/a.txt"
printf "line1\nmodified\nline3\nline4\n" > "$TMPDIR/b.txt"
$PROG -t "$TMPDIR/a.txt" "$TMPDIR/b.txt" > "$TMPDIR/out.txt" || true
if grep -q "modified" "$TMPDIR/out.txt" && grep -q "@@" "$TMPDIR/out.txt"; then
    pass "unified diff shows changes"
else
    fail "unified diff output"
fi

# --- Test 3: Side-by-side mode ---
echo "Test 3: Side-by-side mode"
$PROG -t -y "$TMPDIR/a.txt" "$TMPDIR/b.txt" > "$TMPDIR/out.txt" || true
if grep -q "File 1" "$TMPDIR/out.txt"; then
    pass "side-by-side output"
else
    fail "side-by-side output"
fi

# --- Test 4: Binary identical ---
echo "Test 4: Identical binary files"
dd if=/dev/urandom of="$TMPDIR/bin1" bs=1024 count=10 2>/dev/null
cp "$TMPDIR/bin1" "$TMPDIR/bin2"
$PROG -b "$TMPDIR/bin1" "$TMPDIR/bin2" > "$TMPDIR/out.txt"
rc=$?
if [ $rc -eq 0 ] && grep -q "identical" "$TMPDIR/out.txt"; then
    pass "identical binary files"
else
    fail "identical binary files (exit=$rc)"
fi

# --- Test 5: Binary files with differences ---
echo "Test 5: Binary diff"
cp "$TMPDIR/bin1" "$TMPDIR/bin3"
printf '\xFF' | dd of="$TMPDIR/bin3" bs=1 seek=100 conv=notrunc 2>/dev/null
$PROG -b "$TMPDIR/bin1" "$TMPDIR/bin3" > "$TMPDIR/out.txt"
rc=$?
if [ $rc -eq 1 ] && grep -q "offset" "$TMPDIR/out.txt"; then
    pass "binary diff detects byte difference"
else
    fail "binary diff (exit=$rc)"
fi

# --- Test 6: Brief mode ---
echo "Test 6: Brief mode"
$PROG -q "$TMPDIR/a.txt" "$TMPDIR/b.txt" > "$TMPDIR/out.txt" || true
if grep -q "differ" "$TMPDIR/out.txt"; then
    pass "brief mode reports files differ"
else
    fail "brief mode"
fi

# --- Test 7: Brief mode identical ---
echo "Test 7: Brief mode identical"
cp "$TMPDIR/a.txt" "$TMPDIR/c.txt"
$PROG -q "$TMPDIR/a.txt" "$TMPDIR/c.txt" > "$TMPDIR/out.txt"
rc=$?
if [ $rc -eq 0 ] && grep -q "identical" "$TMPDIR/out.txt"; then
    pass "brief mode reports identical"
else
    fail "brief mode identical (exit=$rc)"
fi

# --- Test 8: Metrics flag ---
echo "Test 8: Metrics output"
$PROG -t -m "$TMPDIR/a.txt" "$TMPDIR/b.txt" > /dev/null 2> "$TMPDIR/metrics.txt" || true
if grep -q "Wall time" "$TMPDIR/metrics.txt" && grep -q "Throughput" "$TMPDIR/metrics.txt"; then
    pass "metrics output"
else
    fail "metrics output"
fi

# --- Test 9: Parallel binary diff ---
echo "Test 9: Parallel binary diff"
$PROG -b -p -j 4 "$TMPDIR/bin1" "$TMPDIR/bin3" > "$TMPDIR/out.txt" || true
if grep -q "Thread" "$TMPDIR/out.txt"; then
    pass "parallel binary diff"
else
    fail "parallel binary diff"
fi

# --- Test 10: Output to file ---
echo "Test 10: Output to file"
$PROG -t -o "$TMPDIR/result.txt" "$TMPDIR/a.txt" "$TMPDIR/b.txt" || true
if [ -f "$TMPDIR/result.txt" ] && grep -q "modified" "$TMPDIR/result.txt"; then
    pass "output to file"
else
    fail "output to file"
fi

# --- Test 11: Missing file ---
echo "Test 11: Missing file"
$PROG "$TMPDIR/a.txt" "$TMPDIR/nonexistent" > /dev/null 2>&1
rc=$?
if [ $rc -eq 2 ]; then
    pass "missing file returns exit 2"
else
    fail "missing file (exit=$rc, expected 2)"
fi

# --- Test 12: Empty files ---
echo "Test 12: Empty files"
touch "$TMPDIR/empty1" "$TMPDIR/empty2"
$PROG -q "$TMPDIR/empty1" "$TMPDIR/empty2" > "$TMPDIR/out.txt"
rc=$?
if [ $rc -eq 0 ]; then
    pass "empty files are identical"
else
    fail "empty files (exit=$rc)"
fi

# --- Test 13: Auto-detect binary ---
echo "Test 13: Auto-detect binary mode"
$PROG -v "$TMPDIR/bin1" "$TMPDIR/bin3" > /dev/null 2> "$TMPDIR/verbose.txt" || true
if grep -q "binary" "$TMPDIR/verbose.txt"; then
    pass "auto-detects binary mode"
else
    fail "auto-detect binary"
fi

# --- Test 14: Verbose output ---
echo "Test 14: Verbose output"
$PROG -v -t "$TMPDIR/a.txt" "$TMPDIR/b.txt" > /dev/null 2> "$TMPDIR/verbose.txt" || true
if grep -q "File 1:" "$TMPDIR/verbose.txt" && grep -q "File 2:" "$TMPDIR/verbose.txt"; then
    pass "verbose shows file info"
else
    fail "verbose output"
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ $FAIL -eq 0 ] && exit 0 || exit 1
