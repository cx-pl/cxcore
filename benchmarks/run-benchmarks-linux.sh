#!/usr/bin/env bash
set -euo pipefail

benchmark_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cxcore_root="$(cd "$benchmark_root/.." && pwd)"
iterations="${1:-10000}"
samples="${2:-5}"
results_root="$benchmark_root/results"
timestamp="$(date -u +%Y%m%d-%H%M%S)"
csv="$results_root/cxcore-Linux-$timestamp.csv"
report="$results_root/cxcore-Linux-$timestamp.txt"
mkdir -p "$results_root"

cmake_bin="${CMAKE_BIN:-}"
if [[ -z "$cmake_bin" ]]; then
    if [[ -x "$HOME/.local/bin/cmake" ]]; then
        cmake_bin="$HOME/.local/bin/cmake"
    else
        cmake_bin="$(command -v cmake || true)"
    fi
fi
if [[ -z "$cmake_bin" ]]; then
    echo "CMake 3.31 or newer is required; set CMAKE_BIN to its executable." >&2
    exit 1
fi
cmake_version="$("$cmake_bin" --version | awk 'NR == 1 { print $3 }')"
if [[ "$(printf '%s\n' 3.31 "$cmake_version" | sort -V | head -n1)" != "3.31" ]]; then
    echo "CMake 3.31 or newer is required; found $cmake_version at $cmake_bin." >&2
    echo "Set CMAKE_BIN to a newer CMake executable." >&2
    exit 1
fi

if ! [[ "$iterations" =~ ^[1-9][0-9]*$ ]] || ! [[ "$samples" =~ ^[3-9][0-9]*$ ]]; then
    echo "Usage: $0 [iterations>=1] [samples>=3]" >&2
    exit 2
fi

cat > "$report" <<EOF
Date: $(date --iso-8601=seconds)
Machine: $(hostname)
CPU: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | xargs)
OS: $(uname -srmo)
cxcore commit: $(git -c "safe.directory=$cxcore_root" -C "$cxcore_root" rev-parse HEAD)
Iterations per operation/sample: $iterations
Samples per operation: $samples
Warmup passes per operation: 2 at one tenth iterations
Benchmark thread affinity: first CPU allowed by the process cpuset
Configuration: Release, CMake/GCC
CMake: $cmake_version
Compiler: $(cc --version | head -n1)
EOF

echo 'typeinfo_position,linkage,arch,benchmark,sample,iterations,elapsed_ms,ns_per_operation,checksum,arena_peak_used_bytes,arena_capacity_bytes,arena_growth_count,arena_reset_count,durable_copy_bytes,gc_collection_count_delta,gc_collected_bytes_delta,gc_pause_ns_delta,heap_live_bytes_after,request_latency_p50_ns,request_latency_p95_ns,request_latency_p99_ns,request_latency_max_ns' > "$csv"

for linkage in shared static; do
    static="OFF"
    if [[ "$linkage" == static ]]; then static="ON"; fi
    build_root="$benchmark_root/out/linux-x64-$linkage"
    "$cmake_bin" -S "$benchmark_root" -B "$build_root" \
        -DCXCORE_SOURCE_DIR="$cxcore_root" \
        -DCX_BENCH_STATIC="$static" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTING=OFF
    "$cmake_bin" --build "$build_root" --target cxcore_benchmarks --parallel
    executable="$build_root/bin/cxcore_benchmarks"
    output="$("$executable" --iterations "$iterations" --samples "$samples")"
    while IFS= read -r row; do
        [[ -z "$row" ]] && continue
        [[ "$row" == arch,benchmark,* ]] && continue
        printf 'Current,%s,%s\n' "$linkage" "$row" >> "$csv"
    done <<< "$output"
    printf 'Completed: Linux x64 %s\n' "$linkage" >> "$report"
done

printf 'CSV: %s\nReport: %s\n' "$csv" "$report"
