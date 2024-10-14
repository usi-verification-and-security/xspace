#!/bin/bash

[[ -z $1 ]] && {
    printf "USAGE: %s <psi_d> <f> [<max_rows>]\n" "$0"
    exit 0
}

PSI_D_FILE="$1"
shift

[[ -r $PSI_D_FILE ]] || {
    printf "Not readable psi file: %s\n" "$PSI_D_FILE" >&2
    exit 1
}

FILE="$1"
shift

[[ -r $FILE ]] || {
    printf "Not readable data file: %s\n" "$FILE" >&2
    exit 1
}

MAX_LINES=$1

N_LINES=$(wc -l <"$FILE")

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

IFILES=()
OFILES=()

# set -e

function cleanup {
    local code=$1

    [[ -n $code && $code != 0 ]] && {
        kill 0
        wait
    }

    for idx in ${!IFILES[@]}; do
        local ifile=${IFILES[$idx]}
        local ofile=${OFILES[$idx]}
        rm -f $ifile
        rm -f $ofile
    done

    [[ -n $code ]] && exit $code
}

N_CPU=$(nproc --all)
MAX_PROC=$(( 1 + $N_CPU/2 ))
N_PROC=0

PIDS=()

VARIABLES=($(sed -rn '/declare-fun/s/.*declare-fun ([^ ]*) .*/\1/p' <$PSI_D_FILE))

cnt=0
while read line; do
    [[ -z ${line// } ]] && continue

    for var in ${VARIABLES[@]}; do
        ifile=$(mktemp).smt2
        ofile=${ifile}.out
        IFILES+=($ifile)
        OFILES+=($ofile)

        var2=${var}-2
        sed_str="s/${var}([^a-zA-Z0-9\-_])/${var2}\1/g"
        line2=$(sed -r "$sed_str" <<<"$line")

        cp "$PSI_D_FILE" $ifile
        printf "(declare-fun %s () Real)\n" $var2 >>$ifile
        printf "(declare-fun C1 () Real)\n" >>$ifile
        printf "(declare-fun C2 () Real)\n" >>$ifile

        sed -n '/assert/,$p' <"$PSI_D_FILE" | sed -r "${sed_str}" >>$ifile

        printf "(assert %s)\n(assert %s)\n" "$line" "$line2" >>$ifile
        printf "(assert (and (= %s C1) (= %s C2) (not (= C1 C2))))\n" $var $var2 >>$ifile
        printf "(check-sat)\n" >>$ifile

        while (( $N_PROC >= $MAX_PROC )); do
            # wait -n -p pid || {
            #     printf "Process %d exited with error.\n" $pid >&2
            wait -n || {
                printf "Process exited with error.\n" >&2
                cleanup 4
            }
            (( --N_PROC ))
        done

        $SOLVER $ifile &>$ofile &
        PIDS+=($!)
        (( ++N_PROC ))
    done

    [[ -z $MAX_LINES ]] && continue
    (( ++cnt ))
    (( $cnt >= $MAX_LINES )) && break
done <"$FILE"

wait || {
    printf "Error when waiting on the rest of processes.\n" >&2
    cleanup 4
}

cnt=0
for idx in ${!IFILES[@]}; do
    ofile=${OFILES[$idx]}
    result=$(cat $ofile)
    if [[ $result == unsat ]]; then
        (( ++cnt ))
    elif [[ $result != sat ]]; then
        printf "Unexpected output of query:\n%s\n" "$result" >&2
        less $ofile
        cleanup 3
    fi
done

printf "avg #fixed features: %.1f%%\n" $(bc -l <<<"($cnt / ${#IFILES[@]})*100")

cleanup 0
