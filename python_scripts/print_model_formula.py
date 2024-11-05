import numpy as np
import tensorflow as tf
# from keras.src.models import Model
# from keras.src.saving import load_model

# model = tf.keras.models.load_model("../models/heartAttack.h5")
import matplotlib.pyplot as plt
import tensorflow as tf

def plot_heatmap(model_path):
    # Load the model
    model = tf.keras.models.load_model(model_path)

    # Extract weights for the first hidden layer
    weights = model.layers[0].get_weights()[0]

    # Plotting the heatmap
    fig, ax = plt.subplots(figsize=(10, 8))
    cax = ax.imshow(weights, cmap='viridis', aspect='auto')
    plt.colorbar(cax)
    plt.title('Heatmap of Weights in the First Hidden Layer')
    plt.ylabel('Input Features')
    plt.xlabel('Neurons in the Hidden Layer')

    # Annotate each cell with the numeric value
    for i in range(weights.shape[0]):
        for j in range(weights.shape[1]):
            ax.text(j, i, f"{weights[i, j]:.3f}", ha="center", va="center", color="w")

    plt.draw()  # Render the plot fully before saving
    plt.savefig('obesity_level_10-20-10_Layer1.png')
    plt.show()  # Display the plot after saving


def load_and_parse_model(model_path):
    # Load the model from the .h5 file
    model = tf.keras.models.load_model(model_path)

    # Extract weights and biases for the first hidden layer
    weights, biases = model.layers[0].get_weights()

    # Round weights and biases to 4 decimal places
    weights = np.round(weights, 4)
    biases = np.round(biases, 4)

    # Number of inputs to the model
    num_inputs = weights.shape[0]

    # Generate and print the formula for each neuron in the first hidden layer
    for i, (weight, bias) in enumerate(zip(weights.T, biases)):
        # Construct the formula as a string
        formula_parts = [f"{weight[j]:.3f} * x{j}" for j in range(num_inputs)]
        formula = " + ".join(formula_parts)
        formula = f"N{i} = {formula} + {bias:.3f}"

        print(formula)

# Example usage
model_path = '../models/obesity/obesity_level_10-20-10.h5'
plot_heatmap(model_path)
load_and_parse_model(model_path)
