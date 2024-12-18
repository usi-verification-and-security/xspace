#!/bin/bash

ACTION_REGEX='check|count-fixed'

function usage {
    printf "USAGE: %s <action> <psi> <f> [<max_rows>]\n" "$0"
    printf "ACTIONS: %s\n" "$ACTION_REGEX"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 0

ACTION=$1
shift

[[ $ACTION =~ ^($ACTION_REGEX)$ ]] || {
    printf "Expected an action, got: %s\n" "$ACTION" >&2
    usage 1
}

PSI_FILE="$1"
shift

[[ $PSI_FILE =~ \.smt2$ ]] || {
    printf "Expected an SMT-LIB psi file, got: %s\n" "$PSI_FILE" >&2
    usage 1
}

case $ACTION in
check)
    [[ $PSI_FILE =~ _c[0-9][^0-9] ]] || {
        printf "Expected class-related psi file, got: %s\n" "$PSI_FILE" >&2
        usage 1
    }
    ;;
count-fixed)
    [[ $PSI_FILE =~ _d ]] || {
        printf "Expected domain psi file, got: %s\n" "$PSI_FILE" >&2
        usage 1
    }
    ;;
esac

[[ -r $PSI_FILE ]] || {
    printf "Not readable psi file: %s\n" "$PSI_FILE" >&2
    usage 1
}

FILE="$1"
shift

if [[ $FILE =~ \.phi\.txt$ ]]; then
    PHI_FILE="$FILE"
    STATS_FILE="${FILE/.phi./.stats.}"
elif [[ $FILE =~ \.stats\.txt$ ]]; then
    STATS_FILE="$FILE"
    PHI_FILE="${FILE/.stats./.phi.}"
else
    STATS_FILE="${FILE%.}.stats.txt"
    PHI_FILE="${FILE%.}.phi.txt"
fi

[[ -r $STATS_FILE ]] || {
    printf "Not readable stats data file: %s\n" "$STATS_FILE" >&2
    usage 1
}

[[ -r $PHI_FILE ]] || {
    printf "Not readable formula data file: %s\n" "$PHI_FILE" >&2
    usage 1
}

MAX_LINES=$1

N_LINES=$(wc -l <"$PHI_FILE")

[[ -n $MAX_LINES ]] && {
    (( $MAX_LINES > $N_LINES )) && {
        printf "The entered max. no. lines is greater than the actual no. lines: %d > %d\n" $MAX_LINES $N_LINES >&2
        exit 2
    }
}

SOLVER=cvc5
# SOLVER=z3
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

[[ $ACTION == count-fixed ]] && {
    VARIABLES=($(sed -rn '/declare-fun/s/.*declare-fun ([^ ]*) .*/\1/p' <$PSI_FILE))
}

case $ACTION in
check)
    ARGS=(dummy)
    ;;
count-fixed)
    ARGS=(${VARIABLES[@]})
    ;;
esac

cnt=0
while read line; do
    [[ -z ${line// } ]] && continue

    case $ACTION in
    check)
        while read oline <&3; do
            [[ $oline =~ "computed output:" ]] && break
        done
        out_class=${oline##*: }
        [[ $out_class =~ ^[0-9]+$ ]] || {
            printf "Unexpected output class from row '%s' in %s: %s\n" "$oline" "$STATS_FILE" "$out_class" >&2
            cleanup 3
        }
        psi_file="${PSI_FILE/_c[0-9]/_c$out_class}"
        ;;
    count-fixed)
        psi_file="$PSI_FILE"
        ;;
    esac

    for arg in ${ARGS[@]}; do
        ifile=$(mktemp --suffix=.smt2)
        ofile=${ifile}.out
        IFILES+=($ifile)
        OFILES+=($ofile)

        cp "$psi_file" $ifile

        case $ACTION in
        check)
            printf "(assert %s)" "$line" >>$ifile
            ;;
        count-fixed)
            var=$arg
            var2=${var}-2
            sed_str="s/${var}([^a-zA-Z0-9\-_])/${var2}\1/g"
            line2=$(sed -r "$sed_str" <<<"$line")

            printf "(declare-fun %s () Real)\n" $var2 >>$ifile
            printf "(declare-fun C1 () Real)\n" >>$ifile
            printf "(declare-fun C2 () Real)\n" >>$ifile

            sed -n '/assert/,$p' <"$psi_file" | sed -r "${sed_str}" >>$ifile

            printf "(assert %s)\n(assert %s)\n" "$line" "$line2" >>$ifile
            printf "(assert (and (= %s C1) (= %s C2) (not (= C1 C2))))\n" $var $var2 >>$ifile
            ;;
        esac

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
done <"$PHI_FILE" 3<"$STATS_FILE"

wait || {
    printf "Error when waiting on the rest of processes.\n" >&2
    cleanup 4
}

cnt=0
for idx in ${!IFILES[@]}; do
    ifile=${IFILES[$idx]}
    ofile=${OFILES[$idx]}
    result=$(cat $ofile)
    if [[ $result == unsat ]]; then
        (( ++cnt ))
        case $ACTION in
        check)
            ;;
        count-fixed)
            ;;
        esac
    elif [[ $result == sat ]]; then
        case $ACTION in
        check)
            printf "NOT space explanation [%d]:\n" $idx >&2
            sed -n '$p' $ifile >&2
            cp -v $ifile $(basename $ifile) >&2
            cleanup 3
            ;;
        count-fixed)
            ;;
        esac
    else
        printf "Unexpected output of query:\n%s\n" "$result" >&2
        less $ofile
        cleanup 3
    fi
done

case $ACTION in
check)
    printf "OK!\n"
    ;;
count-fixed)
    printf "avg #fixed features: %.1f%%\n" $(bc -l <<<"($cnt / ${#IFILES[@]})*100")
    ;;
esac

cleanup 0
