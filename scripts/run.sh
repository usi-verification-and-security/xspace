#!/bin/bash

BIN=$1

[[ -x $BIN ]] || {
  printf "Not executable: %s\n" $BIN >&2
  exit 1
}

BUILD_TYPE=${BIN%/*}
if [[ $BUILD_TYPE == build ]]; then
  unset BUILD_TYPE
else
  BUILD_TYPE=${BUILD_TYPE#build-}
  BUILD_PREFIX=${BUILD_TYPE}-
fi

VARIANT=${BIN#build*/XAI-SMT_}
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

PREFIX=$DIR/${BUILD_PREFIX}${VARIANT}

{ time $BIN >${PREFIX}.stats.txt 2>${PREFIX}.phi.txt; } 2>${PREFIX}.time.txt &
