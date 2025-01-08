#!/bin/bash

MODELS=(models/Heart_attack/heartAttack50.nnet models/obesity/obesity-10-20-10.nnet)
DATASETS=(data/heartAttack.csv data/obesity_test_data_noWeight.csv)
OUTPUT_DIRS=(output/heart_attack output/obesity)

[[ -z $CMD ]] && CMD=build/xspace

## Default for now ..
MAX_MODELS=1

source "$(dirname "$0")/experiments"

for model_idx in ${!MODELS[@]}; do
    [[ -n $MAX_MODELS ]] && (( model_idx >= MAX_MODELS )) && break

    model="${MODELS[$model_idx]}"
    dataset="${DATASETS[$model_idx]}"
    output_dir="${OUTPUT_DIRS[$model_idx]}"

    for exp_idx in ${!EXPERIMENTS[@]}; do
        _experiment=${EXPERIMENTS[$exp_idx]}
        experiment_strategies="${EXPERIMENT_STRATEGIES[$exp_idx]}"

        for do_reverse in 0 1; do
            experiment=$_experiment
            opts=-sv
            (( $do_reverse )) && {
                experiment+=_reverse
                opts+=r
            }

            { time ${CMD} "$model" "$dataset" opensmt "$experiment_strategies" $opts >"${output_dir}/${experiment}.phi.txt" 2>"${output_dir}/${experiment}.stats.txt" ; } 2>"${output_dir}/${experiment}.time.txt" &
        done
    done
done
