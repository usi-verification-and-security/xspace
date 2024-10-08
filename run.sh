#!/bin/bash

BIN=$1

[[ -x $BIN ]] || {
  printf "Not executable: %s\n" $BIN >&2
  exit 1
}

VARIANT=${BIN#build/XAI-SMT_}
if [[ $VARIANT =~ .*-.* ]]; then
  DATASET=${VARIANT#*-}
  VARIANT=${VARIANT%-*}
else
  DATASET=heart_attack
fi

[[ -z $VARIANT ]] && {
  echo "Expected a variant." >&2
  exit 2
}

[[ -z $DATASET ]] && {
  echo "Expected a dataset." >&2
  exit 2
}

DIR=output/$DATASET

[[ -d $DIR ]] || {
  printf "Directory '%s' does not exist.\n" "$DIR" >&2
  exit 2
}

{ time $BIN >$DIR/${VARIANT}.out.txt 2>$DIR/${VARIANT}.err.txt; } 2>$DIR/${VARIANT}.time.txt &
