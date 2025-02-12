#!/bin/bash

## git-diff experimental outputs ignoring times and missing contents
## > we can check the results even if the experiments have not finished yet

cd explanations
git diff | grep '^+' | grep -Ev '^\+(real|user|sys)' | less
