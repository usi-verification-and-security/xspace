#!/bin/bash

MODELS=(models/Heart_attack/heartAttack50.nnet models/obesity/obesity-10-20-10.nnet)
DATASETS=(data/heartAttack.csv data/obesity_test_data_noWeight.csv)
OUTPUT_DIRS=(output/heart_attack output/obesity)

[[ -z $CMD ]] && CMD=build/xspace

for idx in ${!MODELS[@]}; do
    [[ -n $MAX_MODELS ]] && (( idx >= MAX_MODELS )) && break

    model="${MODELS[$idx]}"
    dataset="${DATASETS[$idx]}"
    output_dir="${OUTPUT_DIRS[$idx]}"

    # { time ${CMD} "$model" "$dataset" opensmt 'abductive' -sv >"${output_dir}/ucore_sample_min.err.txt" 2>"${output_dir}/ucore_sample_min.out.txt" ; } 2>"${output_dir}/ucore_sample_min.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'abductive' -rsv >"${output_dir}/ucore_sample_min_reverse.err.txt" 2>"${output_dir}/ucore_sample_min_reverse.out.txt" ; } 2>"${output_dir}/ucore_sample_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample' -sv >"${output_dir}/ucore_sample_min.err.txt" 2>"${output_dir}/ucore_sample_min.out.txt" ; } 2>"${output_dir}/ucore_sample_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample' -rsv >"${output_dir}/ucore_sample_min_reverse.err.txt" 2>"${output_dir}/ucore_sample_min_reverse.out.txt" ; } 2>"${output_dir}/ucore_sample_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval' -sv >"${output_dir}/ucore_interval_min.err.txt" 2>"${output_dir}/ucore_interval_min.out.txt" ; } 2>"${output_dir}/ucore_interval_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval' -rsv >"${output_dir}/ucore_interval_min_reverse.err.txt" 2>"${output_dir}/ucore_interval_min_reverse.out.txt" ; } 2>"${output_dir}/ucore_interval_min_reverse.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'abductive,trial' -sv >"${output_dir}/trial_sample_min_3.err.txt" 2>"${output_dir}/trial_sample_min_3.out.txt" ; } 2>"${output_dir}/trial_sample_min_3.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'abductive,trial' -rsv >"${output_dir}/trial_sample_min_reverse_3.err.txt" 2>"${output_dir}/trial_sample_min_reverse_3.out.txt" ; } 2>"${output_dir}/trial_sample_min_reverse_3.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore,trial' -sv >"${output_dir}/trial_sample_min_3.err.txt" 2>"${output_dir}/trial_sample_min_3.out.txt" ; } 2>"${output_dir}/trial_sample_min_3.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore,trial' -rsv >"${output_dir}/trial_sample_min_reverse_3.err.txt" 2>"${output_dir}/trial_sample_min_reverse_3.out.txt" ; } 2>"${output_dir}/trial_sample_min_reverse_3.time.txt" &
done
