#!/bin/bash

DIRNAME=$(dirname "$0")

source "$DIRNAME/lib/run-xspace"

function usage {
    printf "USAGE: %s <output_dir> [consecutive] [short] [<filter_regex>]\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

read_output_dir "$1" && shift

if [[ $1 == consecutive ]]; then
    CONSECUTIVE=1
    shift
else
    CONSECUTIVE=0
fi

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

if (( ! $CONSECUTIVE )); then
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
else
    for exp_idx in ${!CONSECUTIVE_EXPERIMENTS_SRC[@]}; do
        src_experiment=${CONSECUTIVE_EXPERIMENTS_SRC[$exp_idx]}
        dst_experiment=${CONSECUTIVE_EXPERIMENTS_DST[$exp_idx]}
        [[ -n $FILTER && ! $src_experiment =~ $FILTER && ! $dst_experiment =~ $FILTER ]] && {
            printf "Skipping %s ...\n" $src_experiment
            continue
        }

        printf "Running %s on top of %s in the background ...\n" $dst_experiment $src_experiment

        find_strategies_for_experiment $dst_experiment experiment_strategies

        for rev in '' reverse; do
            SRC_EXPERIMENT=$src_experiment "$DIRNAME/run-xspace.sh" "$OUTPUT_DIR" "$experiment_strategies" $dst_experiment $rev $MAX_SAMPLES &
        done
    done
fi

printf "\nDone.\n"
exit 0
