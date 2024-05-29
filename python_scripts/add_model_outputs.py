import pandas as pd
import tensorflow as tf
# from keras.src.models import Model
from keras.src.saving import load_model

model = tf.keras.models.load_model("../models/heartAttack.h5")
# model.summary()

# LOAD data from data/heartAttack.csv
data = pd.read_csv("../data/heartAttack.csv")
# PREPARE data
X = data.drop(columns=["output"])
y = data["output"]
# PREDICT
y_pred = model.predict(X)

if model.layers[-1].activation.__name__ == 'sigmoid':
    # Get the configuration of the last layer
    last_layer_config = model.layers[-1].get_config()

    # Change the activation function to 'linear'
    last_layer_config['activation'] = 'linear'

    # Create a new layer with the modified configuration
    new_last_layer = tf.keras.layers.Dense.from_config(last_layer_config)

    # Create a new model with all layers of the original model except the last one
    new_model = tf.keras.Sequential(model.layers[:-1])
    new_model.add(new_last_layer)
    print("Model has sigmoid layer")
    model = new_model

y_pred = model.predict(X)
# flatten the y_pred
y_pred = y_pred.flatten()
X['target'] = y
X["model_output"] = y_pred
# SAVE data to data/heartAttack.csv
X.to_csv("../data/heartAttack2.csv", index=False)
print(y_pred)



