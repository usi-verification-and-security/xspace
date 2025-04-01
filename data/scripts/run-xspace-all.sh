#!/bin/bash

DIRNAME=$(dirname "$0")

source "$DIRNAME/lib/run-xspace"

function usage {
    printf "USAGE: %s <output_dir> [consecutive] [short] [<filter_regex>] [-n]\n" "$0"

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

[[ -n $1 && $1 != -n ]] && {
    FILTER="$1"
    shift
}

DRY_RUN=0
[[ $1 == -n ]] && {
    DRY_RUN=1
    shift
}

[[ -n $1 ]] && {
    printf "Additional arguments: %s\n" "$*" >&2
    usage 1 >&2
}

set_cmd

printf "Output directory: %s\n" "$OUTPUT_DIR"
printf "Model: %s\n" "$MODEL"
printf "Dataset: %s\n" "$DATASET"
printf "\n"

(( $DRY_RUN )) && printf "DRY RUN - only printing what would be run\n\n"

if (( ! $CONSECUTIVE )); then
    declare -n lEXPERIMENT_NAMES=EXPERIMENT_NAMES
else
    declare -n lEXPERIMENT_NAMES=CONSECUTIVE_EXPERIMENTS_NAMES
fi

for exp_idx in ${!lEXPERIMENT_NAMES[@]}; do
    experiment=${lEXPERIMENT_NAMES[$exp_idx]}
    [[ -n $FILTER && ! $experiment =~ $FILTER ]] && {
        printf "Skipping %s ...\n" $experiment
        continue
    }

    if (( ! $CONSECUTIVE )); then
        unset {src,dst}_experiment

        experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"
    else
        src_experiment=${CONSECUTIVE_EXPERIMENTS_SRC_NAMES[$exp_idx]}
        dst_experiment=${CONSECUTIVE_EXPERIMENTS_DST_NAMES[$exp_idx]}

        find_strategies_for_experiment $dst_experiment experiment_strategies
    fi

    printf "Running %s in the background ...\n" $experiment

    (( $DRY_RUN )) && continue

    for rev in '' reverse; do
        SRC_EXPERIMENT=$src_experiment "$DIRNAME/run-xspace.sh" "$OUTPUT_DIR" "$experiment_strategies" $experiment $rev $MAX_SAMPLES &
    done
done

printf "\nDone.\n"
exit 0
