#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

STATS_SCRIPT="$SCRIPTS_DIR/stats.awk"

source "$SCRIPTS_DIR/lib/experiments"

function usage {
    printf "USAGE: %s <dir> [short]\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && {
    printf "Provide stats data directory.\n" >&2
    usage 1 >&2
}

STATS_DIR="$1"
shift
[[ -d $STATS_DIR && -r $STATS_DIR ]] || {
    printf "'%s' is not a readable directory.\n" "$STATS_DIR" >&2
    usage 1 >&2
}

[[ -n $1 ]] && {
    SHORT="$1"
    shift

    [[ $SHORT == short ]] || {
        printf "Expected 'short' to collect stats from shortened experiments, got: %s\n" "$SHORT" >&2
        usage 1 >&2
    }
}

EXPERIMENT_MAX_WIDTH=40

FEATURES_CAPTION='%features'
FEATURES_MAX_WIDTH=${#FEATURES_CAPTION}

FIXED_CAPTION='%fixed'
FIXED_MAX_WIDTH=${#FIXED_CAPTION}

DIMENSION_CAPTION='%dimension'
DIMENSION_MAX_WIDTH=${#DIMENSION_CAPTION}

TERMS_CAPTION='#terms'
TERMS_MAX_WIDTH=${#TERMS_CAPTION}

CHECKS_CAPTION='#checks'
CHECKS_MAX_WIDTH=${#CHECKS_CAPTION}

## Workaround until all experiments with outdated termSize are re-run
function compute_term_size {
    local phi_file="$1"
    local n_lines=$2

    tr ' ' '\n' <"$phi_file" | grep -c '=' | { tr -d '\n'; printf "/$n_lines\n"; } | bc -l
}

printf "%${EXPERIMENT_MAX_WIDTH}s" experiment
printf " | %s" "$FEATURES_CAPTION"
printf " | %s" "$FIXED_CAPTION"
printf " | %s" "$DIMENSION_CAPTION"
printf " | %s" "$TERMS_CAPTION"
printf " | %s" "$CHECKS_CAPTION"
printf " | avg. time [s]\n"

for do_reverse in 0 1; do
    if (( $do_reverse )); then
        printf "REVERSE:\n"
    else
        printf "REGULAR:\n"
    fi

    for experiment in ${EXPERIMENTS[@]}; do
        experiment_stem=$experiment
        (( $do_reverse )) && experiment_stem=reverse/$experiment_stem
        [[ -n $SHORT ]] && experiment_stem=short/$experiment_stem

        stats_file="${STATS_DIR}/${experiment_stem}.stats.txt"
        [[ -r $stats_file ]] || {
            printf "File '%s' is not a readable.\n" "$stats_file" >&2
            exit 1
        }

        time_file="${STATS_DIR}/${experiment_stem}.time.txt"
        [[ -r $time_file ]] || {
            printf "File '%s' is not a readable.\n" "$time_file" >&2
            exit 1
        }

        phi_file="${STATS_DIR}/${experiment_stem}.phi.txt"
        [[ -r $phi_file ]] || {
            printf "File '%s' is not a readable.\n" "$phi_file" >&2
            exit 1
        }

        stats=$($STATS_SCRIPT "$stats_file")
        size=$(sed -n 's/^Total:[^0-9]*\([0-9]*\)$/\1/p' <<<"$stats")
        perc_features=$(sed -n 's/^.*#any features: \([^%]*\)%.*$/\1/p' <<<"$stats")
        perc_fixed_features=$(sed -n 's/^.*#fixed features: \([^%]*\)%.*$/\1/p' <<<"$stats")
        # nterms=$(sed -n 's/^.*#terms: \(.*\)$/\1/p' <<<"$stats")
        nterms=$(compute_term_size "$phi_file" $size)
        nchecks=$(sed -n 's/^.*#checks: \(.*\)$/\1/p' <<<"$stats")

        perc_dimension=$(bc -l <<<"100 - $perc_fixed_features")

        time_str=$(sed -n 's/^user[^0-9]*\([0-9].*\)$/\1/p' <"$time_file")

        time_min=${time_str%%m*}
        time_s=${time_str##*m}
        time_s=${time_s%s}
        total_time_s=$(bc -l <<<"${time_min}*60 + ${time_s}")
        avg_time_s=$(bc -l <<<"${total_time_s}/${size}")

        printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
        printf " |%${FEATURES_MAX_WIDTH}.1f%%" $perc_features
        printf " |%${FIXED_MAX_WIDTH}.1f%%" $perc_fixed_features
        printf " |%${DIMENSION_MAX_WIDTH}.1f%%" $perc_dimension
        printf " | %${TERMS_MAX_WIDTH}.1f" $nterms
        printf " | %${CHECKS_MAX_WIDTH}.1f" $nchecks
        printf " | %.2f" $avg_time_s
        printf "\n"
    done
done
