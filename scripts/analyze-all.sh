#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

ANALYZE_SCRIPT="$SCRIPTS_DIR/analyze.sh"

source "$SCRIPTS_DIR/experiments"

function usage {
    printf "USAGE: %s <action> <dir> [short]\n" "$0"
    $ANALYZE_SCRIPT |& grep ACTIONS

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && {
    usage 1 >&2
}

ACTION=$1
shift

[[ -z $1 ]] && {
    printf "Provide phi data directory.\n" >&2
    usage 1 >&2
}

PHI_DIR="$1"
shift
[[ -d $PHI_DIR && -r $PHI_DIR ]] || {
    printf "'%s' is not a readable directory.\n" "$PHI_DIR" >&2
    usage 1 >&2
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
    usage 2 >&2
}

[[ -n $1 ]] && {
    SHORT="$1"
    shift

    [[ $SHORT == short ]] || {
        printf "Expected 'short' to analyze shortened experiments, got: %s\n" "$SHORT" >&2
        usage 1 >&2
    }
}

case $ACTION in
compare-subset)
    EXPERIMENT_MAX_WIDTH=70
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
    printf "\n"
    ;;
esac

function set_phi_filename {
    local experiment=$1
    local -n lphi_file=$2

    local experiment_stem=$experiment
    (( $do_reverse )) && experiment_stem+=_reverse
    [[ -n $SHORT ]] && experiment_stem+=_short

    lphi_file="${PHI_DIR}/${experiment_stem}.phi.txt"
    [[ -r $lphi_file ]] || {
        printf "File '%s' is not a readable.\n" "$lphi_file" >&2
        exit 1
    }
}

for do_reverse in 0 1; do
    case $ACTION in
    count-fixed|compare-subset)
        if (( $do_reverse )); then
            printf "REVERSE:\n"
        else
            printf "REGULAR:\n"
        fi
        ;;
    esac

    for exp_idx in ${!EXPERIMENTS[@]}; do
        experiment=${EXPERIMENTS[$exp_idx]}

        set_phi_filename $experiment phi_file

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
                set_phi_filename $experiment2 phi_file2
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
done

case $ACTION in
check)
    printf "OK!\n"
    ;;
esac

exit 0
