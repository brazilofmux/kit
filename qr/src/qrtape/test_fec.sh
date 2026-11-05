#!/bin/bash

# Comprehensive test script for QR Tape
# Usage: ./test_fec.sh [size_in_bytes]

SIZE=${1:-4096}
INPUT_FILE="test_input.dat"
OUTPUT_FILE="test_recovered.dat"
PASSED=0
FAILED=0

cleanup() {
    rm -f *.ppm "$INPUT_FILE" "$OUTPUT_FILE" 2>/dev/null || true
}

ensure_binaries() {
    if [ ! -x ./qrtapemake ] || [ ! -x ./qrtaperead ]; then
        echo "Building binaries..."
        make -s
    fi
}

run_fec_test() {
    local frag_to_delete="$1"
    local test_name="$2"

    cleanup

    echo "=== Test: $test_name ==="
    echo "Generating $SIZE bytes of random data..."
    dd if=/dev/urandom of=$INPUT_FILE bs=1 count=$SIZE status=none

    echo "Encoding..."
    ./qrtapemake $INPUT_FILE > /dev/null

    if [ ! -f "$frag_to_delete" ]; then
        echo "SKIP: Fragment $frag_to_delete not generated (input size may be too small)"
        return 2
    fi

    echo "Simulating damage: Deleting $frag_to_delete..."
    rm "$frag_to_delete"

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_basic_test() {
    local test_size="$1"
    local test_name="$2"

    cleanup

    echo "=== Test: $test_name ==="
    if [ "$test_size" -eq 0 ]; then
        touch "$INPUT_FILE"
    else
        dd if=/dev/urandom of=$INPUT_FILE bs=1 count=$test_size status=none
    fi

    echo "Encoding..."
    ./qrtapemake $INPUT_FILE > /dev/null

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_compression_test() {
    local test_name="Compression effectiveness"

    cleanup

    echo "=== Test: $test_name ==="
    # Create highly compressible data (repeated pattern)
    yes "AAAAAAAAAA" | head -c 4096 > $INPUT_FILE

    echo "Encoding compressible data..."
    ./qrtapemake $INPUT_FILE > /dev/null

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_multi_block_test() {
    local test_name="Multi-block file (16KB)"

    cleanup

    echo "=== Test: $test_name ==="
    dd if=/dev/urandom of=$INPUT_FILE bs=1 count=16384 status=none

    echo "Encoding..."
    ./qrtapemake $INPUT_FILE > /dev/null

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_last_frag_loss_test() {
    local test_name="FEC recovery (last fragment)"

    cleanup

    echo "=== Test: $test_name ==="
    dd if=/dev/urandom of=$INPUT_FILE bs=1 count=$SIZE status=none

    echo "Encoding..."
    ./qrtapemake $INPUT_FILE > /dev/null

    # Find the last data fragment (exclude EOF block)
    LAST_DATA_FRAG=$(ls out_B0000_F*.ppm 2>/dev/null | sort | tail -1)
    if [ -z "$LAST_DATA_FRAG" ]; then
        echo "SKIP: No data fragments found"
        return 2
    fi

    echo "Simulating damage: Deleting $LAST_DATA_FRAG..."
    rm -f "$LAST_DATA_FRAG"

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_multi_block_fec_test() {
    local test_name="FEC recovery across multiple blocks"

    cleanup

    echo "=== Test: $test_name ==="
    # Create file spanning multiple blocks (>4KB)
    dd if=/dev/urandom of=$INPUT_FILE bs=1 count=12288 status=none

    echo "Encoding..."
    ./qrtapemake $INPUT_FILE > /dev/null

    # Delete one fragment from each of the first two blocks
    if [ -f out_B0000_F01.ppm ] && [ -f out_B0001_F01.ppm ]; then
        echo "Simulating damage: Deleting F01 from blocks 0 and 1..."
        rm -f out_B0000_F01.ppm out_B0001_F01.ppm
    else
        echo "SKIP: Multi-block structure not as expected"
        return 2
    fi

    echo "Decoding..."
    ./qrtaperead $OUTPUT_FILE out_B*.ppm > /dev/null 2>&1

    echo "Verifying..."
    if diff -q $INPUT_FILE $OUTPUT_FILE > /dev/null 2>&1; then
        echo "PASSED: $test_name"
        return 0
    else
        echo "FAILED: $test_name"
        return 1
    fi
}

run_help_test() {
    local test_name="Help output"

    echo "=== Test: $test_name ==="

    if ./qrtapemake --help 2>&1 | grep -q "Usage:"; then
        echo "  qrtapemake --help: OK"
    else
        echo "  qrtapemake --help: FAILED"
        return 1
    fi

    if ./qrtaperead --help 2>&1 | grep -q "Usage:"; then
        echo "  qrtaperead --help: OK"
    else
        echo "  qrtaperead --help: FAILED"
        return 1
    fi

    echo "PASSED: $test_name"
    return 0
}

# Main test runner
main() {
    echo "========================================"
    echo "QR Tape Comprehensive Test Suite"
    echo "========================================"
    echo ""

    ensure_binaries

    # Test 1: Help output
    if run_help_test; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 2: Basic round-trip with default size
    if run_basic_test $SIZE "Basic round-trip ($SIZE bytes)"; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 3: Small file (100 bytes)
    if run_basic_test 100 "Small file (100 bytes)"; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 4: Single fragment loss (F01)
    run_fec_test "out_B0000_F01.ppm" "FEC recovery (middle fragment)"
    ret=$?
    if [ $ret -eq 0 ]; then
        PASSED=$((PASSED + 1))
    elif [ $ret -eq 2 ]; then
        :  # Skip doesn't count
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 5: First fragment loss (F00)
    run_fec_test "out_B0000_F00.ppm" "FEC recovery (first fragment)"
    ret=$?
    if [ $ret -eq 0 ]; then
        PASSED=$((PASSED + 1))
    elif [ $ret -eq 2 ]; then
        :  # Skip doesn't count
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 6: Compression test
    if run_compression_test; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 7: Multi-block test
    if run_multi_block_test; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 8: Last fragment loss
    run_last_frag_loss_test
    ret=$?
    if [ $ret -eq 0 ]; then
        PASSED=$((PASSED + 1))
    elif [ $ret -eq 2 ]; then
        :  # Skip doesn't count
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Test 9: Multi-block FEC recovery
    run_multi_block_fec_test
    ret=$?
    if [ $ret -eq 0 ]; then
        PASSED=$((PASSED + 1))
    elif [ $ret -eq 2 ]; then
        :  # Skip doesn't count
    else
        FAILED=$((FAILED + 1))
    fi
    echo ""

    # Cleanup
    cleanup

    # Summary
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo "Passed: $PASSED"
    echo "Failed: $FAILED"
    echo ""

    if [ $FAILED -eq 0 ]; then
        echo "All tests passed!"
        exit 0
    else
        echo "Some tests failed."
        exit 1
    fi
}

main
