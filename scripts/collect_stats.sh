#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

STATS_SCRIPT="$SCRIPTS_DIR/stats.awk"

source "$SCRIPTS_DIR/experiments"

[[ -z $1 ]] && {
    printf "Provide stats data directory.\n" >&2
    exit 1
}

STATS_DIR="$1"
[[ -d $STATS_DIR && -r $STATS_DIR ]] || {
    printf "'%s' is not a readable directory.\n" "$STATS_DIR" >&2
    exit 1
}

EXPERIMENT_MAX_WIDTH=40

DIMENSION_CAPTION='%dimension'
DIMENSION_MAX_WIDTH=${#DIMENSION_CAPTION}

TERMS_CAPTION='#terms'
TERMS_MAX_WIDTH=${#TERMS_CAPTION}

CHECKS_CAPTION='#checks'
CHECKS_MAX_WIDTH=${#CHECKS_CAPTION}

printf "%${EXPERIMENT_MAX_WIDTH}s | %s | %s | %s\n\n" experiment "$DIMENSION_CAPTION" "$TERMS_CAPTION" "$CHECKS_CAPTION"

for do_reverse in 0 1; do
    for experiment in ${EXPERIMENTS[@]}; do
        (( $do_reverse )) && {
            experiment+=_reverse
        }

        stats_file="${STATS_DIR}/${experiment}.stats.txt"
        [[ -r $stats_file ]] || {
            printf "File '%s' is not a readable.\n" "$stats_file" >&2
            exit 1
        }

        stats=$($STATS_SCRIPT "$stats_file")
        perc_features=$(sed -n 's/^.*#any features: \([^%]*\)%.*$/\1/p'  <<<"$stats")
        perc_fixed_features=$(sed -n 's/^.*#fixed features: \([^%]*\)%.*$/\1/p'  <<<"$stats")
        nterms=$(sed -n 's/^.*#terms: \(.*\)$/\1/p'  <<<"$stats")
        nchecks=$(sed -n 's/^.*#checks: \(.*\)$/\1/p'  <<<"$stats")

        perc_dimension=$(bc -l <<<"100 - $perc_fixed_features")

        printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
        printf " |%${DIMENSION_MAX_WIDTH}.1f%%" $perc_dimension
        printf " | %${TERMS_MAX_WIDTH}.1f" $nterms
        printf " | %${CHECKS_MAX_WIDTH}.1f" $nchecks
        printf "\n"
    done

    printf "\n"
done
