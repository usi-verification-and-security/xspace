#!/bin/bash

[[ -z $1 ]] && {
    printf "USAGE: %s <psi_d> <f1> <f2> [<max_comparisons>]\n" "$0"
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

N_LINES1=$(wc -l <"$FILE1")
N_LINES2=$(wc -l <"$FILE2")
(( $N_LINES1 != $N_LINES2 )) && {
    printf "The number of lines of the data files do not match: %d != %d\n" $N_LINES1 $N_LINES2 >&2
    exit 2
}
N_LINES=$N_LINES1

[[ -n $MAX_LINES ]] && {
    (( $MAX_LINES > $N_LINES )) && {
        printf "The entered max. no. lines is greater than the actual no. lines: %d > %d\n" $MAX_LINES $N_LINES >&2
        exit 2
    }
}

SOLVER=z3
# SOLVER=/home/tomaqa/Data/Prog/C++/opensmt/build/opensmt

command -v $SOLVER &>/dev/null || {
    printf "Solver %s is not executable: %s\n" $SOLVER >&2
    exit 2
}

IFILES_SUBSET=()
IFILES_SUPSET=()

OFILES_SUBSET=()
OFILES_SUPSET=()

# set -e

function cleanup {
    local code=$1

    for idx in ${!IFILES_SUBSET[@]}; do
        local ifile_subset=${IFILES_SUBSET[$idx]}
        local ofile_subset=${OFILES_SUBSET[$idx]}
        local ifile_supset=${IFILES_SUPSET[$idx]}
        local ofile_supset=${OFILES_SUPSET[$idx]}
        rm $ifile_subset
        rm $ofile_subset
        rm $ifile_supset
        rm $ofile_supset
    done

    [[ -n $code ]] && exit $code
}

## Not parallelized yet, seems fast enough

cnt=0
while read line1 && read line2 <&3; do
    [[ -z ${line1// } ]] && continue

    ifile_subset=$(mktemp).smt2
    ofile_subset=${ifile_subset}.out
    IFILES_SUBSET+=($ifile_subset)
    OFILES_SUBSET+=($ofile_subset)
    ifile_supset=$(mktemp).smt2
    ofile_supset=${ifile_supset}.out
    IFILES_SUPSET+=($ifile_supset)
    OFILES_SUPSET+=($ofile_supset)

    cp "$PSI_D_FILE" $ifile_subset
    cp "$PSI_D_FILE" $ifile_supset
    printf "(assert (and %s (not %s)))\n(check-sat)\n" "$line1" "$line2" >>$ifile_subset
    printf "(assert (and (not %s) %s))\n(check-sat)\n" "$line1" "$line2" >>$ifile_supset

    $SOLVER $ifile_subset &>$ofile_subset
    $SOLVER $ifile_supset &>$ofile_supset

    [[ -z $MAX_LINES ]] && continue
    (( ++cnt ))
    (( $cnt >= $MAX_LINES )) && break
done <"$FILE1" 3<"$FILE2"

SUBSET_CNT=0
SUPSET_CNT=0
EQUAL_CNT=0
DISTINCT_CNT=0
for idx in ${!IFILES_SUBSET[@]}; do
    ofile_subset=${OFILES_SUBSET[$idx]}
    ofile_supset=${OFILES_SUPSET[$idx]}

    subset_result=$(cat $ofile_subset)
    supset_result=$(cat $ofile_supset)

    if [[ $subset_result == unsat ]]; then
        if [[ $supset_result == unsat ]]; then
            (( ++EQUAL_CNT ))
        elif [[ $supset_result == sat ]]; then
            (( ++SUBSET_CNT ))
        else
            printf "Unexpected output of supset query: %s\n" $supset_result >&2
            less $ofile_supset
            cleanup 3
        fi
    elif [[ $subset_result == sat ]]; then
        if [[ $supset_result == unsat ]]; then
            (( ++SUPSET_CNT ))
        elif [[ $supset_result == sat ]]; then
            (( ++DISTINCT_CNT ))
        else
            printf "Unexpected output of supset query: %s\n" $supset_result >&2
            less $ofile_supset
            cleanup 3
        fi
    else
        printf "Unexpected output of subset query: %s\n" $subset_result >&2
        less $ofile_subset
        cleanup 3
    fi
done

printf "<: %d =: %d >: %d | ?: %d\n" $SUBSET_CNT $EQUAL_CNT $SUPSET_CNT $DISTINCT_CNT

cleanup 0
