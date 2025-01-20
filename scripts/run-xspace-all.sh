#!/bin/bash

source "$(dirname "$0")/experiments"

OUTPUT_DIRS=(output/heart_attack output/obesity)

## Models and datasets that correspond to the output directories
MODELS=(models/Heart_attack/heartAttack50.nnet models/obesity/obesity-10-20-10.nnet)
DATASETS=(data/heartAttack.csv data/obesity_test_data_noWeight.csv)

function usage {
    printf "USAGE: %s <output_dir> [short] [<filter_regex>]\n" "$0"

    [[ -n $1 ]] && exit $1
}

[[ -z $1 ]] && usage 1 >&2

OUTPUT_DIR="$1"
shift

OUTPUT_DIR="${OUTPUT_DIR%%/}"
for odir_idx in ${!OUTPUT_DIRS[@]}; do
    odir="${OUTPUT_DIRS[$odir_idx]}"
    [[ $odir == $OUTPUT_DIR ]] || continue
    OUTPUT_DIR_IDX=$odir_idx
    break
done

[[ -z $OUTPUT_DIR_IDX ]] && {
    printf "Unrecognized output directory: %s\n" "$OUTPUT_DIR" >&2
    printf "Expected one of: %s\n" "${OUTPUT_DIRS[*]}" >&2
    exit 1
}

[[ -d $OUTPUT_DIR && -w $OUTPUT_DIR ]] || {
    printf "Output directory %s is not writable directory.\n" "$OUTPUT_DIR" >&2
    exit 2
}

MODEL="${MODELS[$OUTPUT_DIR_IDX]}"
DATASET="${DATASETS[$OUTPUT_DIR_IDX]}"

[[ -r $MODEL ]] || {
    printf "Model file %s is not readable.\n" "$MODEL" >&2
    exit 2
}

[[ -r $DATASET ]] || {
    printf "Dataset file %s is not readable.\n" "$DATASET" >&2
    exit 2
}

[[ -n $1 && $1 =~ ^(short|[0-9]+)$ ]] && {
    MAX_SAMPLES_SHORT=20
    MAX_SAMPLES=$1
    shift

    if [[ $MAX_SAMPLES == short ]]; then
        MAX_SAMPLES=$MAX_SAMPLES_SHORT
    elif ! [[ $MAX_SAMPLES =~ ^[1-9][0-9]*$ ]]; then
        printf "Expected 'short' or a concrete number of shuffled samples to process, got: %s\n" "$MAX_SAMPLES" >&2
        exit 1
    fi
}

[[ -n $1 ]] && {
    FILTER="$1"
    shift
}

[[ -z $CMD ]] && CMD=build/xspace

printf "Output directory:  %s\n" "$OUTPUT_DIR"
printf "Model:  %s\n" "$MODEL"
printf "Dataset:  %s\n" "$DATASET"
printf "\n"

_output_dir="$OUTPUT_DIR"

for exp_idx in ${!EXPERIMENTS[@]}; do
    experiment=${EXPERIMENTS[$exp_idx]}
    [[ -n $FILTER && ! $experiment =~ $FILTER ]] && {
        printf "Skipping %s ...\n" $experiment
        continue
    }

    printf "Running %s ...\n" $experiment

    experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"

    for do_reverse in 0 1; do
        output_dir="$_output_dir"
        opts=(-sv)

        [[ -n $MAX_SAMPLES ]] && {
            if (( $MAX_SAMPLES != $MAX_SAMPLES_SHORT )); then
                output_dir+=/Sn$MAX_SAMPLES
            else
                output_dir+=/short
            fi
            opts+=(-Sn$MAX_SAMPLES)
        }

        (( $do_reverse )) && {
            output_dir+=/reverse
            opts+=(-r)
        }

        mkdir -p "$output_dir" >/dev/null || exit $?
        { time ${CMD} "$MODEL" "$DATASET" "$experiment_strategies" ${opts[@]} >"${output_dir}/${experiment}.phi.txt" 2>"${output_dir}/${experiment}.stats.txt" ; } 2>"${output_dir}/${experiment}.time.txt" &
    done
done

printf "\nDone.\n"
exit 0
