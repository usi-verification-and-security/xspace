#!/bin/bash

DIRNAME=$(dirname "$0")

source "$DIRNAME/lib/run-xspace"

function usage {
    printf "USAGE: %s <output_dir> <exp_strategies_spec> <name> [reverse] [short | <max_samples>] <args>...\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

read_output_dir "$1" && shift

[[ -z $1 || $1 =~ ^(reverse|short)$ ]] && usage 1 >&2
EXPERIMENT_STRATEGIES="$1"
shift

[[ -z $1 || $1 =~ ^(reverse|short)$ ]] && usage 1 >&2
EXPERIMENT="$1"
shift

[[ $1 == reverse ]] && {
    REVERSE=1
    shift
}

read_max_samples "$1" && shift

[[ $1 =~ ^(reverse|short)$ ]] && usage 1 >&2

set_cmd

OPTIONS=(-sv)

[[ -n $MAX_SAMPLES ]] && {
    if (( $MAX_SAMPLES != $MAX_SAMPLES_SHORT )); then
        OUTPUT_DIR+=/Sn$MAX_SAMPLES
    else
        OUTPUT_DIR+=/short
    fi
    OPTIONS+=(-Sn$MAX_SAMPLES)
}

[[ -n $REVERSE ]] && {
    OUTPUT_DIR+=/reverse
    OPTIONS+=(-r)
}

mkdir -p "$OUTPUT_DIR" >/dev/null || exit $?
{ time ${CMD} "$MODEL" "$DATASET" "$EXPERIMENT_STRATEGIES" ${OPTIONS[@]} "$@" >"${OUTPUT_DIR}/${EXPERIMENT}.phi.txt" 2>"${OUTPUT_DIR}/${EXPERIMENT}.stats.txt" ; } 2>"${OUTPUT_DIR}/${EXPERIMENT}.time.txt"
