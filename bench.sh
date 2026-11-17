#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage : bash bench.sh {x86|mac} THREADS [THREADS ...]" >&2
    echo "Exemple : ITERATIONS=2 bash bench.sh mac 2 4" >&2
    exit 2
}

(( $# >= 2 )) || usage
machine=$1
shift

case "$machine" in
    x86)
        [[ "$(uname -m)" == x86_64 ]] || { echo "La série AVX requiert une machine x86-64." >&2; exit 2; }
        ;;
    mac)
        [[ "$(uname -s)" == Darwin ]] || { echo "La série Mac requiert macOS." >&2; exit 2; }
        ;;
    *) usage ;;
esac

iterations=${ITERATIONS:-30}
[[ "$iterations" =~ ^[1-9][0-9]*$ ]] || usage
for threads in "$@"; do
    [[ "$threads" =~ ^[1-9][0-9]*$ ]] || usage
done

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
python=${PYTHON:-python3}
export MPLBACKEND=Agg
if ! "$python" -c 'import matplotlib'; then
    echo "Installe les dépendances avec $python -m pip install -r requirements.txt." >&2
    exit 1
fi

images=(image1.pgm MontagneFoncee.pgm Ordi-1970.pgm stavrovouni.pgm)
for image in "${images[@]}"; do
    if [[ ! -f "img/$image" ]]; then
        echo "Image introuvable : img/$image" >&2
        exit 1
    fi
done

results="results/benchmark_$machine"
mkdir -p "$results"
scratch=$(mktemp -d /tmp/pgm-bench.XXXXXX)
trap 'rm -rf -- "$scratch"' EXIT
for image in "${images[@]}"; do
    cp -- "img/$image" "$scratch/$image"
done

if [[ "$machine" == mac ]]; then
    sdk=$(xcrun --sdk macosx --show-sdk-path)
    make -B CC=gcc-12 \
        CPPFLAGS="-D_POSIX_C_SOURCE=200809L -isysroot $sdk -isystem $sdk/usr/include" \
        SRC=implementations/code_openmp.c
    compiler=gcc-12
else
    avx=(code_avx_int code_avx_short code_avx_char)
    for implementation in code_sequentiel "${avx[@]}" code_openmp; do
        make -B CC=gcc SRC="implementations/$implementation.c"
    done
    compiler=gcc
fi

{
    uname -a
    printf 'Processeurs logiques en ligne : %s\n' "$(getconf _NPROCESSORS_ONLN)"
    printf 'Nombres de threads demandés : %s\n' "$*"
    "$compiler" --version
    if [[ "$machine" == mac ]]; then
        system_profiler SPHardwareDataType | awk '/Chip:|Total Number of Cores:/'
    elif command -v lscpu >/dev/null; then
        lscpu
    fi
} > "$results/machine.txt"

benchmark() {
    local output=$1
    shift
    "$python" scripts/global_benchmarking.py "$scratch/$image" -n "$iterations" \
        -c "$@" -o "$results/$output"
}

for image in "${images[@]}"; do
    name=${image%.pgm}

    if [[ "$machine" == x86 ]]; then
        benchmark "avx_vs_sequentiel/$name" "${avx[@]}" code_sequentiel
    fi

    for threads in "$@"; do
        export OMP_DYNAMIC=FALSE OMP_NUM_THREADS="$threads"
        if [[ "$machine" == mac ]]; then
            benchmark "openmp/$name/${threads}_threads" code_openmp
        else
            benchmark "avx_vs_openmp/$name/${threads}_threads" "${avx[@]}" code_openmp
        fi
    done
done

printf 'Résultats dans %s\n' "$results"
