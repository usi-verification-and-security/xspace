#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

STATS_SCRIPT="$SCRIPTS_DIR/stats.awk"

source "$SCRIPTS_DIR/lib/experiments"

function usage {
    printf "USAGE: %s <explanations_dir> <experiments_spec> [[+]consecutive] [[+]reverse] [<max_samples>]\n" "$0"

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

read_experiments_spec "$1" || usage $? >&2
shift

maybe_read_consecutive "$1" && shift
maybe_read_reverse "$1" && shift
maybe_read_max_samples "$1" && shift

[[ -n $1 ]] && {
    FILTER="$1"
    shift
}

if [[ -z $INCLUDE_CONSECUTIVE ]]; then
    declare -n lEXPERIMENT_NAMES=EXPERIMENT_NAMES
    declare -n lMAX_EXPERIMENT_NAMES_LEN=MAX_EXPERIMENT_NAMES_LEN
elif (( $CONSECUTIVE_ONLY )); then
    declare -n lEXPERIMENT_NAMES=CONSECUTIVE_EXPERIMENTS_NAMES
    declare -n lMAX_EXPERIMENT_NAMES_LEN=MAX_CONSECUTIVE_EXPERIMENTS_NAMES_LEN
else
    declare -n lEXPERIMENT_NAMES=EXPERIMENT_NAMES_WITH_CONSECUTIVE
    declare -n lMAX_EXPERIMENT_NAMES_LEN=MAX_EXPERIMENT_NAMES_WITH_CONSECUTIVE_LEN
fi

EXPERIMENT_MAX_WIDTH=$(( 1 + $lMAX_EXPERIMENT_NAMES_LEN ))

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

function compute_term_size {
    local phi_file="$1"
    local n_lines=$2

    tr ' ' '\n' <"$phi_file" | grep -c '=' | { tr -d '\n'; printf "/$n_lines\n"; } | bc -l | xargs -i printf '%.1f' {}
}

PRINTED_HEADER=0
PRINTED_REVERSE=0
PRINTED_REGULAR=0

function print_header {
    local reverse=$1
    local dataset_size=$2
    local n_features=$3

    (( $PRINTED_HEADER )) || {
        printf 'Dataset size: %d\n' $dataset_size
        printf 'Number of features: %d\n' $n_features
        printf '\n'

        printf "%${EXPERIMENT_MAX_WIDTH}s" experiment
        printf " | %s" "$FEATURES_CAPTION"
        printf " | %s" "$FIXED_CAPTION"
        printf " | %s" "$DIMENSION_CAPTION"
        printf " | %s" "$TERMS_CAPTION"
        printf " | %s" "$CHECKS_CAPTION"
        printf " | time [s]\n"

        PRINTED_HEADER=1
    }


    if (( $reverse )); then
        (( $PRINTED_REVERSE )) || {
            printf "REVERSE:\n"
            PRINTED_REVERSE=1
        }
    else
        (( $PRINTED_REGULAR )) || {
            printf "REGULAR:\n"
            PRINTED_REGULAR=1
        }
    fi
}

do_reverse_args=(0)
[[ -n $INCLUDE_REVERSE ]] && {
    (( $REVERSE_ONLY )) && do_reverse_args=()
    do_reverse_args+=(1)
}

ERR_FILE=$(mktemp)

function cleanup {
    rm $ERR_FILE

    exit $1
}

for do_reverse in ${do_reverse_args[@]}; do
    for experiment in ${lEXPERIMENT_NAMES[@]}; do
        experiment_stem=$experiment
        [[ -n $FILTER && ! $experiment =~ $FILTER ]] && continue

        (( $do_reverse )) && experiment_stem=reverse/$experiment_stem
        [[ -n $MAX_SAMPLES ]] && experiment_stem=$MAX_SAMPLES_NAME/$experiment_stem

        stats_file="${STATS_DIR}/${experiment_stem}.stats.txt"
        [[ -r $stats_file ]] || {
            printf "File '%s' is not a readable.\n" "$stats_file" >&2
            cleanup 1
        }

        time_file="${STATS_DIR}/${experiment_stem}.time.txt"
        [[ -r $time_file ]] || {
            printf "File '%s' is not a readable.\n" "$time_file" >&2
            cleanup 1
        }

        phi_file="${STATS_DIR}/${experiment_stem}.phi.txt"
        [[ -r $phi_file ]] || {
            printf "File '%s' is not a readable.\n" "$phi_file" >&2
            cleanup 1
        }

        stats=$($STATS_SCRIPT "$stats_file" 2>$ERR_FILE)
        size=$(sed -n 's/^Total:[^0-9]*\([0-9]*\)$/\1/p' <<<"$stats")
        features=$(sed -n 's/^Features:[^0-9]*\([0-9]*\)$/\1/p' <<<"$stats")

        print_header $do_reverse $size $features

        time_str=$(sed -n 's/^user[^0-9]*\([0-9].*\)$/\1/p' <"$time_file")
        if [[ -n $time_str ]]; then
            [[ -s $ERR_FILE ]] && {
                cat $ERR_FILE >&2
                cleanup 3
            }

            perc_features=$(sed -n 's/^.*#any features: \([^%]*\)%.*$/\1/p' <<<"$stats")
            perc_fixed_features=$(sed -n 's/^.*#fixed features: \([^%]*\)%.*$/\1/p' <<<"$stats")
            nterms_stats=$(sed -n 's/^.*#terms: \(.*\)$/\1/p' <<<"$stats")
            nchecks=$(sed -n 's/^.*#checks: \(.*\)$/\1/p' <<<"$stats")

            perc_dimension=$(bc -l <<<"100 - $perc_fixed_features")

            nterms=$(compute_term_size "$phi_file" $size)
            [[ -z $nterms_stats ]] && {
                ##! fragile
                nterms_stats=$(bc -l <<<"($features * $perc_features)/100")
                nterms_stats=$(printf '%.1f' $nterms_stats)
            }
            [[ $nterms == $nterms_stats ]] || {
                printf "%s: encountered inconsistency: stats.termSize != termSize(phi): %s != %s\n" $experiment_stem "$nterms_stats" "$nterms" >&2
                cleanup 3
            }

            time_min=${time_str%%m*}
            time_s=${time_str##*m}
            time_s=${time_s%s}
            total_time_s=$(bc -l <<<"${time_min}*60 + ${time_s}")
            avg_time_s=$(bc -l <<<"${total_time_s}/${size}")

            perc_features_str=$(printf "%.1f%%" $perc_features)
            perc_fixed_features_str=$(printf "%.1f%%" $perc_fixed_features)
            perc_dimension_str=$(printf "%.1f%%" $perc_dimension)
            nterms_str=$(printf "%.1f" $nterms)
            nchecks_str=$(printf "%.1f" $nchecks)
            avg_time_s_str=$(printf "%.2f" $avg_time_s)
        else
            perc_features_str=X
            perc_fixed_features_str=X
            perc_dimension_str=X
            nterms_str=X
            nchecks_str=X
            avg_time_s_str=X
        fi

        printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
        printf " | %${FEATURES_MAX_WIDTH}s" $perc_features_str
        printf " | %${FIXED_MAX_WIDTH}s" $perc_fixed_features_str
        printf " | %${DIMENSION_MAX_WIDTH}s" $perc_dimension_str
        printf " | %${TERMS_MAX_WIDTH}s" $nterms_str
        printf " | %${CHECKS_MAX_WIDTH}s" $nchecks_str
        printf " | %s" $avg_time_s_str
        printf "\n"
    done
done

cleanup 0
