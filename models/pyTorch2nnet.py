import torch

from python_scripts.nnet_utils.writeNNet import writeNNet
from python_scripts.Models import *

import torch


import torch


def pytorch2nnet(model, nnetFile, input_min=-1000000, input_max=1000000):
    # Extract weights and biases from the model
    weights = []
    biases = []

    for layer in model.children():
        if isinstance(layer, torch.nn.Linear):
            weights.append(layer.weight.data.numpy())
            biases.append(layer.bias.data.numpy())

    # Determine the number of inputs and outputs
    num_inputs = weights[0].shape[0]
    num_outputs = biases[-1].shape[0]

    # Min and max values used to bound the inputs
    if isinstance(input_min, (int, float)):
        input_mins = [input_min for _ in range(num_inputs)]
    else:
        input_mins = input_min
    if isinstance(input_max, (int, float)):
        input_maxes = [input_max for _ in range(num_inputs)]
    else:
        input_maxes = input_max

    # Mean and range values for normalizing the inputs and outputs
    means = [0 for _ in range(num_inputs + num_outputs)]
    ranges = [1 for _ in range(num_inputs + num_outputs)]

    # Convert the file
    writeNNet(weights, biases, input_mins, input_maxes, means, ranges, nnetFile)


# Initialize the model architecture
model = FF_10_20_10(15,7)

# Load the state_dict into the model
pytorchFile = "models/obesity/obesity-10-20-10-new.pth"
state_dict = torch.load(pytorchFile, map_location=torch.device('cpu'))
model.load_state_dict(state_dict)
model.eval()
pytorch2nnet(model=model, nnetFile="models/obesity/obesity-10-20-10-new.nnet")
