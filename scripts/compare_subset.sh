#!/bin/bash

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
        rm -f $ifile_subset
        rm -f $ofile_subset
        rm -f $ifile_supset
        rm -f $ofile_supset
    done

    [[ -n $code ]] && exit $code
}

N_CPU=$(nproc --all)
MAX_PROC=$(( 1 + $N_CPU/2 ))
N_PROC=0

PIDS=()

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

    cp "$PSI_D_FILE" $ifile_subset
    cp "$PSI_D_FILE" $ifile_supset
    printf "(assert (and %s (not %s)))\n(check-sat)\n" "$line1" "$line2" >>$ifile_subset
    printf "(assert (and (not %s) %s))\n(check-sat)\n" "$line1" "$line2" >>$ifile_supset

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

SUBSET_CNT=0
SUPSET_CNT=0
EQUAL_CNT=0
UNCOMPARABLE_CNT=0
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
            (( ++UNCOMPARABLE_CNT ))
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

printf "Total: %d\n" $cnt
printf "<: %d =: %d >: %d | ?: %d\n" $SUBSET_CNT $EQUAL_CNT $SUPSET_CNT $UNCOMPARABLE_CNT

cleanup 0
