#!/bin/bash

DIRNAME=$(dirname "$0")

source "$DIRNAME/lib/run-xspace"

function usage {
    printf "USAGE: %s <output_dir> <exp_strategies_spec> <name> [reverse] [short | <max_samples>] <args>...\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

read_output_dir "$1" && shift

[[ -z $1 || $1 =~ ^(reverse|short)$ ]] && usage 1 >&2
STRATEGIES="$1"
shift

[[ -z $1 || $1 =~ ^(reverse|short)$ ]] && usage 1 >&2
EXPERIMENT="$1"
shift

[[ $1 == reverse ]] && {
    REVERSE=1
    shift
}

read_max_samples "$1" && shift

[[ $1 =~ ^(reverse|short)$ ]] && usage 1 >&2

set_cmd

OPTIONS=(-sv)

[[ -n $MAX_SAMPLES ]] && {
    if (( $MAX_SAMPLES != $MAX_SAMPLES_SHORT )); then
        OUTPUT_DIR+=/Sn$MAX_SAMPLES
    else
        OUTPUT_DIR+=/short
    fi
    OPTIONS+=(-Sn$MAX_SAMPLES)
}

[[ -n $REVERSE ]] && {
    OUTPUT_DIR+=/reverse
    OPTIONS+=(-r)
}

mkdir -p "$OUTPUT_DIR" >/dev/null || exit $?

function set_file {
    local file_var=$1
    local experiment="$2"
    local type=$3
    local src_experiment="$4"

    local aux_experiment
    if [[ -z $src_experiment ]]; then
        aux_experiment="$experiment"
    else
        aux_experiment="${experiment}__${src_experiment}"
    fi

    local -n lfile=$file_var
    lfile="${OUTPUT_DIR}/${aux_experiment}.${type}.txt"
}

ARGS=()

## SRC_EXPERIMENT may be empty
for t in phi stats time; do
    set_file ${t}_file "$EXPERIMENT" $t "$SRC_EXPERIMENT"
done

[[ -n $SRC_EXPERIMENT ]] && {
    set_file src_phi_file "$SRC_EXPERIMENT" phi

    ARGS+=(-E "$src_phi_file")
}

{ time ${CMD} "$MODEL" "$DATASET" "$STRATEGIES" ${OPTIONS[@]} "${ARGS[@]}" "$@" >"$phi_file" 2>"$stats_file" ; } 2>"$time_file"
