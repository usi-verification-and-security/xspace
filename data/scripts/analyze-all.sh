#!/bin/bash

SCRIPTS_DIR=$(dirname "$0")

ANALYZE_SCRIPT="$SCRIPTS_DIR/analyze.sh"

source "$SCRIPTS_DIR/lib/experiments"

function usage {
    printf "USAGE: %s <action> <dir> [short] [<filter_regex>] [<filter_regex2>]\n" "$0"
    $ANALYZE_SCRIPT |& grep ACTIONS
    printf "\t[<filter_regex2>] is only to be used with binary actions\n"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

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

[[ $1 == short ]] && {
    SHORT=$1
    shift
}

[[ -n $1 ]] && {
    FILTER="$1"
    shift
}

case $ACTION in
compare-subset)
    [[ -n $1 ]] && {
        FILTER2="$1"
        shift
    }

    EXPERIMENT_MAX_WIDTH=70
    ;;
*)
    EXPERIMENT_MAX_WIDTH=40
    ;;
esac

[[ -n $1 ]] && {
    printf "Additional arguments: %s\n" "$*" >&2
    usage 1 >&2
}

case $ACTION in
count-fixed|compare-subset)
    printf "%${EXPERIMENT_MAX_WIDTH}s" experiment
    ;;
esac

case $ACTION in
count-fixed)
    FIXED_CAPTION='%fixed'
    FIXED_MAX_WIDTH=${#FIXED_CAPTION}

    DIMENSION_CAPTION='%dimension'
    DIMENSION_MAX_WIDTH=${#DIMENSION_CAPTION}

    printf " | %s" "$FIXED_CAPTION"
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
    (( $do_reverse )) && experiment_stem=reverse/$experiment_stem
    [[ -n $SHORT ]] && experiment_stem=short/$experiment_stem

    lphi_file="${PHI_DIR}/${experiment_stem}.phi.txt"
    [[ -r $lphi_file ]] || {
        printf "File '%s' is not a readable.\n" "$lphi_file" >&2
        exit 1
    }
}

[[ -n $FILTER ]] && {
    KEPT_IDXS=()
    for exp_idx in ${!EXPERIMENT_NAMES[@]}; do
        experiment=${EXPERIMENT_NAMES[$exp_idx]}
        [[ $experiment =~ $FILTER ]] && KEPT_IDXS+=($exp_idx)
    done
}

MODEL=$(basename "$PHI_DIR")

SCRIPT_NAME=$(basename -s .sh "$0")
SCRIPT_OUTPUT_CACHE_FILE_REVERSE="$SCRIPTS_DIR/cache/$MODEL/${SHORT}/reverse/${SCRIPT_NAME}.${ACTION}.txt"
SCRIPT_OUTPUT_CACHE_FILE="${SCRIPT_OUTPUT_CACHE_FILE_REVERSE/reverse\//}"

mkdir -p $(dirname "$SCRIPT_OUTPUT_CACHE_FILE_REVERSE") >/dev/null || exit $?

function get_cache_line {
    local -n lcache_line=$1
    local experiment=$2
    local experiment2=$3
    local try_swap=$4

    [[ -z $CACHE ]] && return 1

    local grep_cmd=(grep -m 1)

    local prefix=' '
    local suffix=' '

    case $ACTION in
    check)
        prefix=/
        suffix=\.
        ;;
    esac

    local experiment_regex="${prefix}${experiment}${suffix}"

    case $ACTION in
    check|count-fixed)
        lcache_line=$(${grep_cmd[@]} "${experiment_regex}" <<<"$CACHE")
        ;;
    compare-subset)
        local mid='vs.'
        local experiment2_regex="${prefix}${experiment2}${suffix}"
        lcache_line=$(${grep_cmd[@]} "${experiment_regex}${mid}${experiment2_regex}" <<<"$CACHE")
        [[ -n $try_swap && -z $lcache_line ]] && {
            lcache_line=$(${grep_cmd[@]} "${experiment2_regex}${mid}${experiment_regex}" <<<"$CACHE")
            [[ -n $lcache_line ]] && {
                lcache_line="${lcache_line/${experiment2_regex}/|${experiment2_regex}}"
                local IFS='|'
                local array=($lcache_line)

                array[1]="${experiment_regex}${mid}${experiment2_regex}"
                local tmp="${array[2]}"
                array[2]="${array[4]}"
                array[4]="$tmp"
                lcache_line="${array[*]}"
                lcache_line="${lcache_line/|/}"
            }
        }
        ;;
    esac

    [[ -n $lcache_line ]]
}

for do_reverse in 0 1; do
    if (( $do_reverse )); then
        declare -n lSCRIPT_OUTPUT_CACHE_FILE=SCRIPT_OUTPUT_CACHE_FILE_REVERSE
    else
        declare -n lSCRIPT_OUTPUT_CACHE_FILE=SCRIPT_OUTPUT_CACHE_FILE
    fi

    [[ -r $lSCRIPT_OUTPUT_CACHE_FILE ]] && {
        CACHE=$(<"$lSCRIPT_OUTPUT_CACHE_FILE")
    }

    case $ACTION in
    *)
        exec 3>&1
        exec > >(tee -i "${lSCRIPT_OUTPUT_CACHE_FILE}")
        >"${lSCRIPT_OUTPUT_CACHE_FILE}"
        [[ -n $FILTER ]] && FILTERED_OUTPUT_CACHE_FILE=$(mktemp)
        ;;
    esac

    case $ACTION in
    count-fixed|compare-subset)
        if (( $do_reverse )); then
            printf "REVERSE:\n"
        else
            printf "REGULAR:\n"
        fi
        ;;
    esac

    for exp_idx in ${!EXPERIMENT_NAMES[@]}; do
        experiment=${EXPERIMENT_NAMES[$exp_idx]}

        set_phi_filename $experiment phi_file

        case $ACTION in
        compare-subset)
            ARGS=(${EXPERIMENT_NAMES[@]:$(($exp_idx+1))})
            [[ -n $FILTER ]] && {
                aux=(${EXPERIMENT_NAMES[@]::$exp_idx})
                for fidx in ${KEPT_IDXS[@]}; do
                    (( $fidx >= $exp_idx )) && break
                    unset -v aux[$fidx]
                done
                ARGS=(${aux[@]} ${ARGS[@]})
            }
            ;;
        *)
            ARGS=(dummy)
            ;;
        esac

        any=0
        for arg in ${ARGS[@]}; do
            phi_files=("$phi_file")
            unset experiment2

            case $ACTION in
            compare-subset)
                experiment2=$arg

                set_phi_filename $experiment2 phi_file2
                phi_files+=("$phi_file2")
                ;;
            esac

            [[ -n $FILTER && ! $experiment =~ $FILTER ]] && {
                get_cache_line cache_line $experiment $experiment2 && printf "%s\n" "$cache_line" >>"$FILTERED_OUTPUT_CACHE_FILE"
                continue
            }

            case $ACTION in
            compare-subset)
                [[ -n $FILTER2 && ! $experiment2 =~ $FILTER2 ]] && {
                    get_cache_line cache_line $experiment $experiment2 1 && printf "%s\n" "$cache_line" >>"$FILTERED_OUTPUT_CACHE_FILE"
                    continue
                }
                ;;
            esac

            any=1

            get_cache_line cache_line $experiment $experiment2 1 && {
                printf "%s\n" "$cache_line"
                continue
            }

            case $ACTION in
            check)
                printf "Analyzing %s ... " "$phi_file"
                ;;
            count-fixed)
                printf "%${EXPERIMENT_MAX_WIDTH}s" $experiment
                ;;
            compare-subset)
                printf "%${EXPERIMENT_MAX_WIDTH}s" "$experiment vs. $experiment2"
                ;;
            esac

            out=$($ANALYZE_SCRIPT $ACTION "$PSI_FILE" "${phi_files[@]}") || exit $?

            case $ACTION in
            check)
                if [[ $out =~ ^OK ]]; then
                    printf "%s\n" "$out"
                else
                    printf "\n%s\n" "$out" >&2
                    exit 4
                fi
                ;;
            count-fixed)
                perc_fixed_features=$(sed -n 's/^.*#fixed features: \([^%]*\)%.*$/\1/p'  <<<"$out")

                perc_dimension=$(bc -l <<<"100 - $perc_fixed_features")

                printf " |%${FIXED_MAX_WIDTH}.1f%%" $perc_fixed_features
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
            (( $any )) && printf "\n"
            ;;
        esac
    done

    case $ACTION in
    *)
        exec >&3 3>&-
        [[ -n $FILTER ]] && {
            cat "$FILTERED_OUTPUT_CACHE_FILE" >>"${lSCRIPT_OUTPUT_CACHE_FILE}"
            rm "$FILTERED_OUTPUT_CACHE_FILE"
        }
        ;;
    esac
done

exit 0
