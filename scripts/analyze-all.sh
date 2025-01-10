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
case $ACTION in
check)
    PSI_FILE+=c0
    ;;
*)
    PSI_FILE+=d
    ;;
esac
PSI_FILE+=.smt2
[[ -r $PSI_FILE ]] || {
    printf "Psi file '%s' is not readable.\n" "$PSI_FILE" >&2
    exit 2
}

case $ACTION in
compare-subset)
    EXPERIMENT_MAX_WIDTH=80
    ;;
*)
    EXPERIMENT_MAX_WIDTH=40
    ;;
esac

case $ACTION in
count-fixed|compare-subset)
    printf "%${EXPERIMENT_MAX_WIDTH}s" experiment
    ;;
esac

case $ACTION in
count-fixed)
    DIMENSION_CAPTION='%dimension'
    DIMENSION_MAX_WIDTH=${#DIMENSION_CAPTION}

    printf " | %s" "$DIMENSION_CAPTION"
    ;;
compare-subset)
    SUBSET_CAPTION='<'
    SUBSET_MAX_WIDTH=3
    EQUAL_CAPTION='='
    EQUAL_MAX_WIDTH=3
    SUPSET_CAPTION='>'
    SUPSET_MAX_WIDTH=3
    UNCOMPARABLE_CAPTION='NC'
    UNCOMPARABLE_MAX_WIDTH=3

    printf " | %${SUBSET_MAX_WIDTH}s" $SUBSET_CAPTION
    printf " | %${EQUAL_MAX_WIDTH}s" $EQUAL_CAPTION
    printf " | %${SUPSET_MAX_WIDTH}s" $SUPSET_CAPTION
    printf " | %${UNCOMPARABLE_MAX_WIDTH}s" $UNCOMPARABLE_CAPTION
    ;;
esac

case $ACTION in
count-fixed|compare-subset)
    printf "\n\n"
    ;;
esac

function set_phi_filename {
    local -n lexperiment=$1
    (( $do_reverse )) && {
        lexperiment+=_reverse
    }

    local -n lphi_file=$2
    lphi_file="${PHI_DIR}/${lexperiment}.phi.txt"
    [[ -r $lphi_file ]] || {
        printf "File '%s' is not a readable.\n" "$lphi_file" >&2
        exit 1
    }
}

for do_reverse in 0 1; do
    for exp_idx in ${!EXPERIMENTS[@]}; do
        experiment=${EXPERIMENTS[$exp_idx]}

        set_phi_filename experiment phi_file

        case $ACTION in
        compare-subset)
            ARGS=(${EXPERIMENTS[@]:$(($exp_idx+1))})
            ;;
        *)
            ARGS=(dummy)
            ;;
        esac

        for arg in ${ARGS[@]}; do
            phi_files=("$phi_file")

            case $ACTION in
            check)
                printf "Analyzing %s ...\n" "$phi_file"
                ;;
            count-fixed)
                printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
                ;;
            compare-subset)
                experiment2=$arg
                set_phi_filename experiment2 phi_file2
                phi_files+=("$phi_file2")

                printf "%${EXPERIMENT_MAX_WIDTH}s" "$experiment vs. $experiment2"
                ;;
            *)
                ;;
            esac

            out=$($ANALYZE_SCRIPT $ACTION "$PSI_FILE" "${phi_files[@]}") || exit $?

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
            compare-subset)
                outs=($(sed -n '2s/[^:]*: \([0-9]*\)/\1 /pg' <<<"$out"))
                subset_out=${outs[0]}
                equal_out=${outs[1]}
                supset_out=${outs[2]}
                uncomparable_out=${outs[3]}

                printf " | %${SUBSET_MAX_WIDTH}d" $subset_out
                printf " | %${EQUAL_MAX_WIDTH}d" $equal_out
                printf " | %${SUPSET_MAX_WIDTH}d" $supset_out
                printf " | %${UNCOMPARABLE_MAX_WIDTH}d" $uncomparable_out
                printf "\n"
                ;;
            esac
        done

        case $ACTION in
        compare-subset)
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
