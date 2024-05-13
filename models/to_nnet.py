
from keras.src.saving import load_model
from NNet.utils.writeNNet import writeNNet


def keras2nnets(kerasFile, input_min=-1000000, input_max=1000000, custom_objects=None):
    # Load the model
    model = load_model(kerasFile, custom_objects=custom_objects)

    # Get a list of the model weights
    model_params = model.get_weights()

    # Split the network parameters into weights and biases, assuming they alternate
    weights = model_params[0:len(model_params):2]
    biases = model_params[1:len(model_params):2]

    # Transpose weight matrices
    weights = [w.T for w in weights]

    num_inputs = model.input_shape[1]
    num_outputs = model.output_shape[1]

    # Min and max values used to bound the inputs
    input_mins = [input_min for _ in range(num_inputs)]
    input_maxes = [input_max for _ in range(num_inputs)]

    # Mean and range values for normalizing the inputs and outputs. All outputs are normalized with the same value
    means = [0 for _ in range(num_inputs+num_outputs)]
    ranges = [1 for _ in range(num_inputs+num_outputs)]

    # Tensorflow pb file to convert to .nnet file
    nnetFile = kerasFile[:-2] + 'nnet'

    # Convert the file
    writeNNet(weights, biases, input_mins, input_maxes, means, ranges, nnetFile)


keras2nnets('heartAttack.h5')