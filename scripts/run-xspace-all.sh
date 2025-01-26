#!/bin/bash

DIRNAME=$(dirname "$0")

source "$DIRNAME/run-xspace_lib"
source "$DIRNAME/experiments"

function usage {
    printf "USAGE: %s <output_dir> [short] [<filter_regex>]\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

read_output_dir "$1" && shift

read_max_samples "$1" && shift

[[ -n $1 ]] && {
    FILTER="$1"
    shift
}

set_cmd

printf "Output directory: %s\n" "$OUTPUT_DIR"
printf "Model: %s\n" "$MODEL"
printf "Dataset: %s\n" "$DATASET"
printf "\n"

for exp_idx in ${!EXPERIMENTS[@]}; do
    experiment=${EXPERIMENTS[$exp_idx]}
    [[ -n $FILTER && ! $experiment =~ $FILTER ]] && {
        printf "Skipping %s ...\n" $experiment
        continue
    }

    printf "Running %s in the background ...\n" $experiment

    experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"

    for rev in '' reverse; do
        "$DIRNAME/run-xspace.sh" "$OUTPUT_DIR" "$experiment_strategies" $experiment $rev $MAX_SAMPLES &
    done
done

printf "\nDone.\n"
exit 0
