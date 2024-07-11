import pandas as pd
import tensorflow as tf
from keras import Model

# from keras.src.models import Model
from keras.src.saving import load_model

from models.keras2nnet import keras2nnets
from models.onnx2nnet import onnx2nnet

target = "NObeyesdad"
model_file = "../models/obesity_level_10-20-10.h5"

model = tf.keras.models.load_model(model_file)
# model.summary()

# LOAD data from data/heartAttack.csv
data = pd.read_csv("../data/obesity_test_data_noWeight.csv")
# PREPARE data
X = data.drop(columns=[target])
y = data[target]
y_pred = model.predict(X)

# Create a new model that includes the input layer and the first hidden layer of the original model
# layer_output_model = Model(inputs=model.input, outputs=model.layers[1].output)
new_model = tf.keras.Sequential(model.layers[:1])
# load the weights of the first layer
new_model.layers[0].set_weights(model.layers[0].get_weights())
# new_model.summary()


first_hidden_layer_output = new_model.predict(X)
# save to data/heartAttack_1st_layer.csv
first_hidden_layer_output = pd.DataFrame(first_hidden_layer_output)
first_hidden_layer_output.to_csv("../data/inner_layers/obesity_noWeight_10_1st_layer.csv", index=False)

# print the max value of each column in the first_hidden_layer_output
print(first_hidden_layer_output.max())
input_max = first_hidden_layer_output.max().tolist()
input_min = [0.0 for _ in range(len(input_max))]

keras2nnets(model_file, input_min=input_min, input_max=input_max, custom_objects=None)