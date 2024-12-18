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

    # { time ${CMD} "$model" "$dataset" opensmt 'abductive' -sv >"${output_dir}/abductive_min.phi.txt" 2>"${output_dir}/abductive_min.stats.txt" ; } 2>"${output_dir}/abductive_min.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'abductive' -rsv >"${output_dir}/abductive_min_reverse.phi.txt" 2>"${output_dir}/abductive_min_reverse.stats.txt" ; } 2>"${output_dir}/abductive_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample' -sv >"${output_dir}/ucore_sample_min.phi.txt" 2>"${output_dir}/ucore_sample_min.stats.txt" ; } 2>"${output_dir}/ucore_sample_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample' -rsv >"${output_dir}/ucore_sample_min_reverse.phi.txt" 2>"${output_dir}/ucore_sample_min_reverse.stats.txt" ; } 2>"${output_dir}/ucore_sample_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval' -sv >"${output_dir}/ucore_interval_min.phi.txt" 2>"${output_dir}/ucore_interval_min.stats.txt" ; } 2>"${output_dir}/ucore_interval_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval' -rsv >"${output_dir}/ucore_interval_min_reverse.phi.txt" 2>"${output_dir}/ucore_interval_min_reverse.stats.txt" ; } 2>"${output_dir}/ucore_interval_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'abductive,trial' -sv >"${output_dir}/trial_sample_min_3.phi.txt" 2>"${output_dir}/trial_sample_min_3.stats.txt" ; } 2>"${output_dir}/trial_sample_min_3.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'abductive,trial' -rsv >"${output_dir}/trial_sample_min_reverse_3.phi.txt" 2>"${output_dir}/trial_sample_min_reverse_3.stats.txt" ; } 2>"${output_dir}/trial_sample_min_reverse_3.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'ucore,trial' -sv >"${output_dir}/trial_ucore_sample_min_3.phi.txt" 2>"${output_dir}/trial_ucore_sample_min_3.stats.txt" ; } 2>"${output_dir}/trial_ucore_sample_min_3.time.txt" &
    # { time ${CMD} "$model" "$dataset" opensmt 'ucore,trial' -rsv >"${output_dir}/trial_ucore_sample_min_reverse_3.phi.txt" 2>"${output_dir}/trial_ucore_sample_min_reverse_3.stats.txt" ; } 2>"${output_dir}/trial_ucore_sample_min_reverse_3.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample,itp' -sv >"${output_dir}/itp_ucore_sample_min.phi.txt" 2>"${output_dir}/itp_ucore_sample_min.stats.txt" ; } 2>"${output_dir}/itp_ucore_sample_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore sample,itp' -rsv >"${output_dir}/itp_ucore_sample_min_reverse.phi.txt" 2>"${output_dir}/itp_ucore_sample_min_reverse.stats.txt" ; } 2>"${output_dir}/itp_ucore_sample_min_reverse.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval,itp' -sv >"${output_dir}/itp_ucore_interval_min.phi.txt" 2>"${output_dir}/itp_ucore_interval_min.stats.txt" ; } 2>"${output_dir}/itp_ucore_interval_min.time.txt" &
    { time ${CMD} "$model" "$dataset" opensmt 'ucore interval,itp' -rsv >"${output_dir}/itp_ucore_interval_min_reverse.phi.txt" 2>"${output_dir}/itp_ucore_interval_min_reverse.stats.txt" ; } 2>"${output_dir}/itp_ucore_interval_min_reverse.time.txt" &
done
