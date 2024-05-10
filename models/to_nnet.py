import h5py
import numpy as np
import json
from tensorflow.keras.models import load_model

import numpy as np

def convert_to_nnet(model, nnet_file):
    # Extract the necessary information from the model
    num_layers = len(model.layers)
    num_inputs = model.input_shape[1]
    num_outputs = model.output_shape[1]
    max_layer_size = 40

    # Write the header and network architecture information
    with open(nnet_file, 'w') as f:
        f.write("// Neural Network File Format\n")
        f.write(f"{num_layers},{num_inputs},{num_outputs},{max_layer_size},\n")

        # Write the layer sizes
        layer_sizes = [num_inputs] + [layer.units for layer in model.layers]
        f.write(','.join(map(str, layer_sizes)) + ',\n')

        # Write the unused flag
        f.write("0,\n")

        # Write the minimum and maximum values of inputs
        f.write("-100000.0," * num_inputs + "\n")
        f.write("100000.0," * num_inputs + "\n")

        # Write the mean and range values of inputs and outputs for normalization
        f.write("0.0," * (num_inputs + 1) + "\n")
        f.write("1.0," * (num_inputs + 1) + "\n")

        # Write the weights and biases for each layer
        for layer in model.layers:
            weights, biases = layer.get_weights()
            for w in weights:
                f.write(','.join(map(str, w)) + ',\n')
            f.write(','.join(map(str, biases)) + ',\n')


# Usage
model = load_model('heartAttack_1hidden.h5')
convert_to_nnet(model, 'heartAttack_1hidden.nnet')