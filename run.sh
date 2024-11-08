#!/bin/bash

BIN=$1

[[ -x $BIN ]] || {
  printf "Not executable: %s\n" $BIN >&2
  exit 1
}

VARIANT=${BIN#build/XAI-SMT_}
DATASET=${VARIANT#*-}
VARIANT=${VARIANT%-*}

[[ -z $VARIANT ]] && {
  echo "Expected a variant." >&2
  exit 2
}

[[ -z $DATASET ]] && DATASET=heart_attack

DIR=output/$DATASET

{ time $BIN >$DIR/${VARIANT}.out.txt 2>$DIR/${VARIANT}.err.txt; } 2>$DIR/${VARIANT}.time.txt &
