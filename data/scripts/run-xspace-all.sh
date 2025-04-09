#!/bin/bash

export DIRNAME=$(dirname "$0")

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
export CONSECUTIVE

read_max_samples "$1" && shift

[[ -n $1 && $1 != -n ]] && {
    export FILTER="$1"
    shift
}

export DRY_RUN=0
[[ $1 =~ ^- ]] && {
    if [[ $1 == -n ]]; then
        DRY_RUN=1
    else
        printf "Unrecognized option: %s\n" "$1" >&2
        usage 1 >&2
    fi
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
    EXPERIMENT_NAMES_VAR=EXPERIMENT_NAMES
else
    EXPERIMENT_NAMES_VAR=CONSECUTIVE_EXPERIMENTS_NAMES
fi
export EXPERIMENT_NAMES_VAR

function run1 {
    source "$DIRNAME/lib/run-xspace"

    local exp_idx=$1

    local -n lexperiment_names=$EXPERIMENT_NAMES_VAR

    local experiment=${lexperiment_names[$exp_idx]}
    [[ -n $FILTER && ! $experiment =~ $FILTER ]] && {
        printf "Skipping %s ...\n" $experiment
        return 0
    }

    local experiment_strategies
    local {src,dst}_experiment
    if (( ! $CONSECUTIVE )); then
        experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"
    else
        src_experiment=${CONSECUTIVE_EXPERIMENTS_SRC_NAMES[$exp_idx]}
        dst_experiment=${CONSECUTIVE_EXPERIMENTS_DST_NAMES[$exp_idx]}

        find_strategies_for_experiment $dst_experiment experiment_strategies
    fi

    printf "Running %s in the background ...\n" $experiment

    (( $DRY_RUN )) && return 0

    for rev in '' reverse; do
        SRC_EXPERIMENT=$src_experiment "$DIRNAME/run-xspace.sh" "$OUTPUT_DIR" "$experiment_strategies" $experiment $rev $MAX_SAMPLES &
    done

    wait || {
        printf "%s failed!\n" $experiment
        return 1
    }

    printf "Finished %s\n" $experiment
}
export -f run1

[[ -z $CPU_PERCENTAGE ]] && CPU_PERCENTAGE=60

## It also runs reverse and non-reverse in parallel -> adjust
CPU_PERCENTAGE=$(( $CPU_PERCENTAGE/2 ))

declare -n lEXPERIMENT_NAMES=$EXPERIMENT_NAMES_VAR

parallel --line-buffer --jobs ${CPU_PERCENTAGE}% 'run1 {}' ::: ${!lEXPERIMENT_NAMES[@]}

(( $? )) && {
    printf "\nFailed.\n"
    exit 1
}

printf "\nDone.\n"
exit 0
