import pandas as pd
import tensorflow as tf
from keras import Model

# from keras.src.models import Model
from keras.src.saving import load_model

model = tf.keras.models.load_model("../models/heart1000.h5")
# model.summary()

# LOAD data from data/heartAttack.csv
data = pd.read_csv("../data/heart.csv")
# PREPARE data
X = data.drop(columns=["target"])
y = data["target"]
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
first_hidden_layer_output.to_csv("../data/inner_layers/heart1000_1st_layer.csv", index=False)

# print the max value of each column in the first_hidden_layer_output
print(first_hidden_layer_output.max())
