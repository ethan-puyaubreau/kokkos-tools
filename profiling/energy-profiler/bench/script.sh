#!/bin/bash

# --- Configuration ---
NUM_ITERATIONS=10 # Change this to the desired number of runs (N)
COMMAND="KOKKOS_TOOLS_LIBS=/home/efp/Kokkos/energy_sandbox/kokkos-tools-energy/build/profiling/energy-profiler/variorum/libkp_energy_variorum.so /home/efp/Desktop/devel/kokkos-sandbox/src/basic_bench/build_cuda/EnergyBench"
HOSTNAME=$(hostname) # Gets the current hostname
OUTPUT_DIR_PREFIX="energy_bench_batch" # Prefix for your batch directories
# --------------------

echo "Starting EnergyBench runs..."
echo "Number of iterations: $NUM_ITERATIONS"
echo "Command to execute: $COMMAND"
echo "Hostname for file filtering: $HOSTNAME"
echo "Output directory prefix: $OUTPUT_DIR_PREFIX"
echo "---"

for i in $(seq 1 $NUM_ITERATIONS); do
    BATCH_DIR="${OUTPUT_DIR_PREFIX}_${i}"
    echo "---"
    echo "Running iteration $i of $NUM_ITERATIONS..."
    echo "Creating directory: $BATCH_DIR"
    mkdir -p "$BATCH_DIR"

    echo "Executing command: $COMMAND"
    eval $COMMAND # Use eval to ensure the full command with environment variables is executed

    echo "Copying files prefixed with '$HOSTNAME' to '$BATCH_DIR'..."
    shopt -s nullglob
    FILES_TO_COPY="${HOSTNAME}*"
    mv $FILES_TO_COPY "$BATCH_DIR/"
    shopt -u nullglob

    echo "Iteration $i complete."
done

echo "---"
echo "All EnergyBench runs completed."
echo "Results are organized in directories like '${OUTPUT_DIR_PREFIX}_1', '${OUTPUT_DIR_PREFIX}_2', etc."