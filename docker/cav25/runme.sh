#!/bin/bash

ACTIONS=(run get-tables verify)
MODES=(quick full)

function usage {
    printf "USAGE: %s <action> <mode>\n" $(basename "$0")
    printf "ACTIONS: %s\n" "${ACTIONS[*]}"
    printf "MODES: %s\n" "${MODES[*]}"

    [[ -n $1 ]] && exit $1
}

cd $(dirname "$0")/data

ROOT_DIR=../
SCRIPTS_DIR=./scripts
EXPLANATIONS_DIR=./explanations

set -e

[[ -z $1 ]] && usage 0
ACTION="$1"
shift

[[ -z $1 ]] && usage 0
MODE="$1"
shift

found=0
for a in ${ACTIONS[@]}; do
    [[ $a == $ACTION ]] && found=1
done
(( $found )) || {
    printf "Unrecognized action: %s\n" "$ACTION" >&2
    usage 1 >&2
}

found=0
for a in ${MODES[@]}; do
    [[ $a == $MODE ]] && found=1
done
(( $found )) || {
    printf "Unrecognized mode: %s\n" "$MODE" >&2
    usage 1 >&2
}

MODELS=(heart_attack obesity mnist)
SPECS=(base itp itp)

SUITES_QUICK=(quick quick quick)
SUITES_FULL=(full short short)

case $MODE in
    quick)
        declare -n SUITES=SUITES_QUICK
        TIMEOUT=8m
        ;;
    full)
        declare -n SUITES=SUITES_FULL
        TIMEOUT=2h
        ;;
esac

case $ACTION in
    run)
        for idx in ${!MODELS[@]}; do
            model=${MODELS[$idx]}
            spec=${SPECS[$idx]}
            suite=${SUITES[$idx]}

            CMD="$ROOT_DIR/build-marabou/xspace" TIMEOUT=$TIMEOUT "$SCRIPTS_DIR/run-xspace-all.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec
            TIMEOUT=$TIMEOUT "$SCRIPTS_DIR/run-xspace-all.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec consecutive
        done
        ;;

    get-tables)
        for table in table-{1,2,3,5}; do
            for idx in ${!MODELS[@]}; do
                model=${MODELS[$idx]}
                spec=${SPECS[$idx]}
                suite=${SUITES[$idx]}

                ofile="$table-$model-$MODE.txt"

                case $table in
                    table-1)
                        "$SCRIPTS_DIR/collect_stats.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec ^itp_a >"$ofile"
                        ;;
                    table-2)
                        "$SCRIPTS_DIR/collect_stats.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec itp_a >"$ofile"
                        ;;
                    table-3)
                        [[ $model != heart_attack ]] && continue

                        "$SCRIPTS_DIR/collect_stats.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec +consecutive '^(trial|itp_aweak_)' >"$ofile"
                        ;;
                    table-5)
                        [[ $model == mnist ]] && continue

                        "$SCRIPTS_DIR/collect_stats.sh" "$EXPLANATIONS_DIR/$model/$suite" $spec 'itp_(aweak|vars)_' >"$ofile"
                        ;;
                esac

                ofile=$(realpath --relative-base="$ROOT_DIR" $ofile)
                printf "Table stored in file %s\n" "$ofile"
            done
        done

        ;;

    verify)
        for idx in ${!MODELS[@]}; do
            model=${MODELS[$idx]}
            spec=${SPECS[$idx]}
            suite=${SUITES[$idx]}

            [[ $model == mnist ]] && continue

            "$SCRIPTS_DIR/analyze-all.sh" check "$EXPLANATIONS_DIR/$model/$suite" $spec
        done
        ;;
esac

printf "\Success.\n"
