# #332 Space Explanations of Neural Network Classification - Artifact

This Docker-based artifact image reproduces data of the tables
presented in the paper #332
"Space Explanations of Neural Network Classification"
submitted to CAV 2025.

The software computes explanations of neural network classification
for 3 models presented in the paper.
The resulting observed data is runtime and some properties of the explanations.

## Contents

* `README.md`: this read-me file
* `LICENSE`: the MIT license
* `xspace-cav25.tar`: the Docker image

The Docker image contains:
* `runme.sh`: the run script (you should not need to use anything else)
* `data`: contains scripts, NN models, datasets and the resulting explanations
* `src`: source codes of the framework
* `build*`: already built binaries (the main executable of the tool is `xspace`)

## Replicated experiments

This artifact reproduces the data presented in the tables 1-5 in the paper.

The artifact does *not* replicate the presented plots.
We have not automated these steps in a user-friendly way yet.

## Instructions

### Docker image

In order to run the Docker image, take the following steps:

* Load the image with: `docker load -i xspace-cav25.tar`
* Run the image with: `docker run -it xspace-cav25:latest`
* Follow the instructions below (this `README.md` file is also present in the image)  
  Each step uses the main script `runme.sh` and should print `Success` at the end.

### Run experiments

To reproduce the experiments, run `./runme.sh` and follow the help message.
Use the `run` action.

For a quick run, use the `quick` mode.
All experiments are reproduced with the `full` mode.

The script automatically runs the experiments in parallel using 60% of available CPUs of the host machine.
The experiments do not require more than 4GB of memory if run on 16 CPUs.
Using 16 CPUs, experiments should finish:
* within ~ 30 minutes in the case of `quick`,
* within ~ 12 hours in the case of `full`.

If needed, the `TIMEOUT` variable in script `runme.sh` can be adjusted manually.
It represents the timeout per job ~ a strategy computing all sample points included in the mode.

We used a Lenovo ThinkPad machine with Intel(R) Core i9 CPU with 32 cores and 64 GBs memory running on Linux 6.6.28.

### Getting the data

After running the experiments, run again `./runme.sh` script
with `get-tables` command for the respective mode.
This will produce text files per each table and each dataset.

### Verifying the explanations

In addition,
it is possible to verify that the produced explanations indeed
guarantee that the classification cannot change.
This is verified with the `verify` command of the `runme.sh` script
using an external SMT solver (CVC5).
Note that this may take a long time, especially for the `full` mode.

## Claimed badges

### Available

The package is published at Zenodo.

### Functional

* The package is well documented and especially, highly automated and should be easy to use.
* The experimental data presented in the paper (tables) is replicated with the exception of the plots.
* The data is consistent with the paper.
* The artifact allows to verify the corresctness of the produced explanations. 
