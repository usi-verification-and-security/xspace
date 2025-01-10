#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

ANALYZE_SCRIPT="$SCRIPTS_DIR/analyze.sh"

source "$SCRIPTS_DIR/experiments"

[[ -z $1 ]] && {
    printf "Provide an action for the '%s' script:\n" $ANALYZE_SCRIPT >&2
    exec $ANALYZE_SCRIPT
}

ACTION=$1
shift

[[ -z $1 ]] && {
    printf "Provide phi data directory.\n" >&2
    exit 1
}

PHI_DIR="$1"
shift
[[ -d $PHI_DIR && -r $PHI_DIR ]] || {
    printf "'%s' is not a readable directory.\n" "$PHI_DIR" >&2
    exit 1
}

PSI_FILE="$PHI_DIR/psi_"
if [[ $ACTION == check ]]; then
    PSI_FILE+=c0
else
    PSI_FILE+=d
fi
PSI_FILE+=.smt2
[[ -r $PSI_FILE ]] || {
    printf "Psi file '%s' is not readable.\n" "$PSI_FILE" >&2
    exit 2
}

EXPERIMENT_MAX_WIDTH=40

DIMENSION_CAPTION='%dimension'
DIMENSION_MAX_WIDTH=${#DIMENSION_CAPTION}

case $ACTION in
check);;
count-fixed)
    printf "%${EXPERIMENT_MAX_WIDTH}s | %s\n\n" experiment "$DIMENSION_CAPTION"
    ;;
esac

for do_reverse in 0 1; do
    for exp_idx in ${!EXPERIMENTS[@]}; do
        _experiment=${EXPERIMENTS[$exp_idx]}
        experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"

        experiment=$_experiment
        (( $do_reverse )) && {
            experiment+=_reverse
        }

        phi_file="${PHI_DIR}/${experiment}.phi.txt"
        [[ -r $phi_file ]] || {
            printf "File '%s' is not a readable.\n" "$phi_file" >&2
            exit 1
        }

        case $ACTION in
        check)
            printf "Analyzing %s ...\n" "$phi_file"
            ;;
        count-fixed)
            printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
            ;;
        esac

        out=$($ANALYZE_SCRIPT $ACTION "$PSI_FILE" "$phi_file") || exit $?

        case $ACTION in
        check)
            [[ $out =~ ^OK ]] || {
                printf "\n%s\n" "$out" >&2
                exit 4
            }
            ;;
        count-fixed)
            perc_fixed_features=$(sed -n 's/^.*#fixed features: \([^%]*\)%.*$/\1/p'  <<<"$out")

            perc_dimension=$(bc -l <<<"100 - $perc_fixed_features")

            printf " |%${DIMENSION_MAX_WIDTH}.1f%%" $perc_dimension
            printf "\n"
            ;;
        esac
    done

    printf "\n"
done

case $ACTION in
check)
    printf "OK!\n"
    ;;
esac

exit 0
