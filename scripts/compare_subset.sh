#!/bin/bash

## Compare two formulas whether they are a subset of another
## > F_1 \subseteq F_2
## > \forall x . [D(x) \land \phi_1(x)] => [D(x) \land \phi_2(x)]
## > \neg \exists x . D(x) \land \phi_1(x) \land [D(x) => \neg \phi_2(x)]
## > \neg \exists x . D(x) \land \phi_1(x) \land \neg \phi_2(x)

## In "relaxed" mode, consider subset relationship also under translation
## > \exists k . F_1 + k \subseteq F_2
## > \exists k \forall x . [D(x) \land \phi_1(x)] => [D(x + k) \land \phi_2(x + k)]
## > (\neg \forall k \exists x . D(x) \land \phi_1(x) \land [D(x + k) => \neg \phi_2(x + k)])

[[ -z $1 ]] && {
    printf "USAGE: %s <psi_d> <f1> <f2> [<max_rows>]\n" "$0"
    exit 0
}

PSI_D_FILE="$1"

[[ -r $PSI_D_FILE ]] || {
    printf "Not readable psi file: %s\n" "$PSI_D_FILE" >&2
    exit 1
}

FILE1="$2"

[[ -r $FILE1 ]] || {
    printf "Not readable first data file: %s\n" "$FILE1" >&2
    exit 1
}

FILE2="$3"

[[ -r $FILE2 ]] || {
    printf "Not readable second data file: %s\n" "$FILE2" >&2
    exit 1
}

MAX_LINES=$4

N_LINES1=$(sed '/^ *$/d' <"$FILE1" | wc -l)
N_LINES2=$(sed '/^ *$/d' <"$FILE2" | wc -l)

if [[ -z $MAX_LINES ]]; then
    (( $N_LINES1 != $N_LINES2 )) && {
        printf "The number of lines of the data files do not match: %d != %d\n" $N_LINES1 $N_LINES2 >&2
        exit 2
    }
    N_LINES=$N_LINES1
else
    if (( $N_LINES1 <= $N_LINES2 )); then
        N_LINES=$N_LINES1
    else
        N_LINES=$N_LINES2
    fi

    if [[ $MAX_LINES == max ]]; then
        MAX_LINES=$N_LINES
    else
        [[ $MAX_LINES =~ ^[1-9][0-9]*$ ]] || {
            printf "Expected max. no. lines, got: %s\n" "$MAX_LINES" >&2
            exit 2
        }

        (( $MAX_LINES > $N_LINES )) && {
            printf "The entered max. no. lines is greater than the min. no. lines of the files: %d > %d\n" $MAX_LINES $N_LINES >&2
            exit 2
        }
    fi
fi

####################################################################################################

function sub_vars {
    local -n str=$1
    local -n array_sub=$2

    for i in ${!VARIABLES[@]}; do
        j=$(( ${#VARIABLES[@]} - $i - 1 ))
        local var=${VARIABLES[$j]}
        local sub="${array_sub[$j]}"
        sub="${sub/$var/${var^^}}"
        str="${str//$var/$sub}"
    done
    for i in ${!VARIABLES[@]}; do
        j=$(( ${#VARIABLES[@]} - $i - 1 ))
        local var=${VARIABLES[$j]}
        str="${str//${var^^}/$var}"
    done
}

PSI_D_WITH_K_FILE=$(mktemp --suffix=.smt2)
sed 's/QF_LRA/LRA/;s/assert/& (forall ()\n(=> (and/;$d' <"$PSI_D_FILE" >$PSI_D_WITH_K_FILE

DOMAINS=$(sed -n '/and/,$p' <"$PSI_D_FILE")
DOMAINS="${DOMAINS%)}"

VARIABLES=($(sed -rn 's/.*declare-fun ([^ ]*) .*/\1/p' <"$PSI_D_FILE"))
for var in ${VARIABLES[@]}; do
    VAR_Q_PAIRS+=("($var Real)")

    kvar=${var/x/k}
    K_VARIABLES+=($kvar)
    sed -i "/declare-fun/s/$var /$kvar /" $PSI_D_WITH_K_FILE

    relaxed_vterm="(+ $var $kvar)"
    RELAXED_VAR_TERMS+=("$relaxed_vterm")
done

RELAXED_DOMAINS="$DOMAINS"
sub_vars RELAXED_DOMAINS RELAXED_VAR_TERMS

sed -i "s/forall (/&${VAR_Q_PAIRS[*]}/" $PSI_D_WITH_K_FILE

####################################################################################################

SOLVER=cvc5
# SOLVER=z3
# SOLVER=/home/tomaqa/Data/Prog/C++/opensmt/build/opensmt

command -v $SOLVER &>/dev/null || {
    printf "Solver %s is not executable: %s\n" $SOLVER >&2
    exit 2
}

IFILES_SUBSET=()
IFILES_SUPSET=()

OFILES_SUBSET=()
OFILES_SUPSET=()

IFILES_SUBSET_RELAXED=()
IFILES_SUPSET_RELAXED=()

OFILES_SUBSET_RELAXED=()
OFILES_SUPSET_RELAXED=()

# set -e

function cleanup {
    local code=$1

    [[ -n $code && $code != 0 ]] && {
        kill 0
        wait
    }

    for idx in ${!IFILES_SUBSET[@]}; do
        local ifile_subset=${IFILES_SUBSET[$idx]}
        local ofile_subset=${OFILES_SUBSET[$idx]}
        local ifile_supset=${IFILES_SUPSET[$idx]}
        local ofile_supset=${OFILES_SUPSET[$idx]}
        local ifile_subset_relaxed=${IFILES_SUBSET_RELAXED[$idx]}
        local ofile_subset_relaxed=${OFILES_SUBSET_RELAXED[$idx]}
        local ifile_supset_relaxed=${IFILES_SUPSET_RELAXED[$idx]}
        local ofile_supset_relaxed=${OFILES_SUPSET_RELAXED[$idx]}
        rm -f $ifile_subset
        rm -f $ofile_subset
        rm -f $ifile_supset
        rm -f $ofile_supset
        rm -f $ifile_subset_relaxed
        rm -f $ofile_subset_relaxed
        rm -f $ifile_supset_relaxed
        rm -f $ofile_supset_relaxed
    done

    rm -f $PSI_D_WITH_K_FILE

    [[ -n $code ]] && exit $code
}

trap 'cleanup 1' INT TERM QUIT

N_CPU=$(nproc --all)
MAX_PROC=$(( 1 + $N_CPU/2 ))
N_PROC=0

PIDS=()

function relax {
    local -n line=$1
    local -n line_relaxed=$2

    line_relaxed=$'(and\n'"$line"
    sub_vars line_relaxed RELAXED_VAR_TERMS
    line_relaxed+=$'\n'"$RELAXED_DOMAINS)"
}

cnt=0
while true; do
    while read line1 && [[ -z ${line1// } ]]; do :; done
    while read line2 <&3 && [[ -z ${line2// } ]]; do :; done
    [[ -z $line1 ]] && break
    [[ -z $line2 ]] && {
        printf "Unexpected missing data from the second file at cnt=%d\n" $cnt >&2
        cleanup 9
    }

    ifile_subset=$(mktemp --suffix=.smt2)
    ofile_subset=${ifile_subset}.out
    IFILES_SUBSET+=($ifile_subset)
    OFILES_SUBSET+=($ofile_subset)
    ifile_supset=$(mktemp --suffix=.smt2)
    ofile_supset=${ifile_supset}.out
    IFILES_SUPSET+=($ifile_supset)
    OFILES_SUPSET+=($ofile_supset)

    ifile_subset_relaxed=$(mktemp --suffix=.smt2)
    ofile_subset_relaxed=${ifile_subset_relaxed}.out
    IFILES_SUBSET_RELAXED+=($ifile_subset_relaxed)
    OFILES_SUBSET_RELAXED+=($ofile_subset_relaxed)
    ifile_supset_relaxed=$(mktemp --suffix=.smt2)
    ofile_supset_relaxed=${ifile_supset_relaxed}.out
    IFILES_SUPSET_RELAXED+=($ifile_supset_relaxed)
    OFILES_SUPSET_RELAXED+=($ofile_supset_relaxed)

    cp "$PSI_D_FILE" $ifile_subset
    cp "$PSI_D_FILE" $ifile_supset

    cp $PSI_D_WITH_K_FILE $ifile_subset_relaxed
    cp $PSI_D_WITH_K_FILE $ifile_supset_relaxed

    printf "(assert (and\n%s\n(not %s)\n))\n(check-sat)\n" "$line1" "$line2" >>$ifile_subset
    printf "(assert (and\n%s\n(not %s)\n))\n(check-sat)\n" "$line2" "$line1" >>$ifile_supset

    relax line1 line1_relaxed
    relax line2 line2_relaxed

    printf "%s\n)\n%s\n)))\n(check-sat)\n" "$line1" "$line2_relaxed" >>$ifile_subset_relaxed
    printf "%s\n)\n%s\n)))\n(check-sat)\n" "$line2" "$line1_relaxed" >>$ifile_supset_relaxed

    while (( $N_PROC >= $MAX_PROC )); do
        # wait -n -p pid || {
        #     printf "Process %d exited with error.\n" $pid >&2
        wait -n || {
            printf "Process exited with error.\n" >&2
            cleanup 4
        }
        (( --N_PROC ))
    done

    $SOLVER $ifile_subset &>$ofile_subset &
    PIDS+=($!)
    (( ++N_PROC ))
    $SOLVER $ifile_supset &>$ofile_supset &
    PIDS+=($!)
    (( ++N_PROC ))

    $SOLVER $ifile_subset_relaxed &>$ofile_subset_relaxed &
    PIDS+=($!)
    (( ++N_PROC ))
    $SOLVER $ifile_supset_relaxed &>$ofile_supset_relaxed &
    PIDS+=($!)
    (( ++N_PROC ))

    [[ -z $MAX_LINES ]] && continue
    (( ++cnt ))
    (( $cnt >= $MAX_LINES )) && break
done <"$FILE1" 3<"$FILE2"

[[ -z $MAX_LINES ]] && cnt=${#IFILES_SUBSET[@]}

[[ -z $MAX_LINES && $cnt != $N_LINES ]] && {
    printf "Unexpected mismatch of the # processed files: %d != %d\n" $cnt $N_LINES >&2
    cleanup 9
}
[[ -n $MAX_LINES && $cnt != $MAX_LINES ]] && {
    printf "Unexpected mismatch of the bounded # processed files: %d != %d\n" $cnt $MAX_LINES >&2
    cleanup 9
}

wait || {
    printf "Error when waiting on the rest of processes.\n" >&2
    cleanup 4
}

function get_result {
    local ofile_subset_id=$1
    local ofile_supset_id=$2
    local -n subset_res=$3
    local -n supset_res=$4
    local negate=$5

    for s in subset supset; do
        local -n ofile_id=ofile_${s}_id
        local -n ofile=$ofile_id

        local sat_res=$(cat $ofile)
        local -n res=${s}_res
        [[ $sat_res =~ ^(un|)sat$ ]] || {
            printf "Unexpected output of $s query: %s\n" $sat_res >&2
            less $ofile
            cleanup 3
        }
        if [[ $sat_res == sat ]]; then
            res=$(( ! negate ))
        else
            res=$(( negate ))
        fi
    done
}

function count_results {
    local -n subset_cnt=$1
    local -n supset_cnt=$2
    local -n equal_cnt=$3
    local -n uncomparable_cnt=$4
    local -n ofiles_subset=$5
    local -n ofiles_supset=$6

    local relaxed=0
    [[ $1 =~ _RELAXED ]] && relaxed=1

    local negate=$(( ! relaxed ))

    subset_cnt=0
    supset_cnt=0
    equal_cnt=0
    uncomparable_cnt=0
    for idx in ${!ofiles_subset[@]}; do
        local ofile_subset=${ofiles_subset[$idx]}
        local ofile_supset=${ofiles_supset[$idx]}

        local subset_result
        local supset_result
        get_result ofile_subset ofile_supset subset_result supset_result $negate

        local subset_result_only=$(( subset_result && !supset_result ))
        local supset_result_only=$(( supset_result && !subset_result ))
        local equal_result=$(( subset_result && supset_result ))

        (( relaxed )) && {
            local ofile_subset_strict=${OFILES_SUBSET[$idx]}
            local ofile_supset_strict=${OFILES_SUPSET[$idx]}

            local ifile_subset_strict=${IFILES_SUBSET[$idx]}
            local ifile_supset_strict=${IFILES_SUPSET[$idx]}
            local ifile_subset_relaxed=${IFILES_SUBSET_RELAXED[$idx]}
            local ifile_supset_relaxed=${IFILES_SUPSET_RELAXED[$idx]}

            local subset_result_strict
            local supset_result_strict
            get_result ofile_subset_strict ofile_supset_strict subset_result_strict supset_result_strict $((!negate))

            local subset_result_strict_only=$(( subset_result_strict && !supset_result_strict ))
            local supset_result_strict_only=$(( supset_result_strict && !subset_result_strict ))
            local equal_result_strict=$(( subset_result_strict && supset_result_strict ))

            for res_name in subset supset equal; do
                local res_strict_id=${res_name}_result_strict_only
                local res_id=${res_name}_result_only
                [[ $res_name == equal ]] && {
                    res_strict_id=${res_strict_id%_only}
                    res_id=${res_id%_only}
                }
                local -n res_strict=$res_strict_id
                local -n res=$res_id

                (( !res_strict || $res )) && continue

                printf "Detected strict $res_name but non-relaxed $res_name: %d !=> %d\n" $res_strict $res >&2
                printf "relaxed subset:%d supset:%d\n" $subset_result $supset_result
                cp -v $ifile_subset_strict tmp.ifile_subset_strict.smt2 >&2
                cp -v $ifile_supset_strict tmp.ifile_supset_strict.smt2 >&2
                cp -v $ifile_subset_relaxed tmp.ifile_subset_relaxed.smt2 >&2
                cp -v $ifile_supset_relaxed tmp.ifile_supset_relaxed.smt2 >&2
                cleanup 8
            done
        }

        if (( subset_result_only )); then
            (( ++subset_cnt ))
        elif (( supset_result_only )); then
            (( ++supset_cnt ))
        elif (( equal_result )); then
            (( ++equal_cnt ))
        else
            (( ++uncomparable_cnt ))
        fi
    done
}

count_results SUBSET_CNT SUPSET_CNT EQUAL_CNT UNCOMPARABLE_CNT OFILES_SUBSET OFILES_SUPSET
count_results SUBSET_CNT_RELAXED SUPSET_CNT_RELAXED EQUAL_CNT_RELAXED UNCOMPARABLE_CNT_RELAXED OFILES_SUBSET_RELAXED OFILES_SUPSET_RELAXED

SUBSET_CNT_RELAXED_ONLY=$(( SUBSET_CNT_RELAXED - SUBSET_CNT ))
SUPSET_CNT_RELAXED_ONLY=$(( SUPSET_CNT_RELAXED - SUPSET_CNT ))

printf "Total: %d\n" $cnt
printf "<<: %d =: %d >>: %d | ?: %d\n" $SUBSET_CNT $EQUAL_CNT $SUPSET_CNT $UNCOMPARABLE_CNT
printf "<<: %d <: %d =: %d >: %d >>: %d | ?: %d\n" $SUBSET_CNT $SUBSET_CNT_RELAXED_ONLY $EQUAL_CNT_RELAXED $SUPSET_CNT_RELAXED_ONLY $SUPSET_CNT $UNCOMPARABLE_CNT_RELAXED

cleanup 0
