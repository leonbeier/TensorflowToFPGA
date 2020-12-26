https://colab.research.google.com/drive/19hGVe8AiKUIbEqd_AU58iOyXtlEMP3E2

Add this after your training to get the weights and biases:

import numpy as np
from google.colab import files

layer = 0;
for i in model.layers:
  print("layer " + str(layer))
  if len(i.get_weights()) == 2:
    print("Weights: " + str(len(i.get_weights()[0])))
    np.savetxt("W"+str(layer)+".csv", i.get_weights()[0], delimiter=",")
    files.download("W"+str(layer)+".csv")
    print("Biases: " + str(len(i.get_weights()[1])))
    np.savetxt("B"+str(layer)+".csv", i.get_weights()[1], delimiter=",")
    files.download("B"+str(layer)+".csv")
  layer += 1

Weights and Biases of the DNN from this example that recognizes handwritten numbers:

Values: weights and biases are signed float between -1 and 1
		neuron values are unsigned float between 0 and 1

layer 0
Values : 784 (28x28 image)

layer 1
Neurons: 128
Weights: 784x128
Biases : 128

layer 2
Neurons: 128
Weights: 128x128
Biases : 128

layer 3
Values : 10
Weights: 128x10
Biases : 10

Values to store: 784 + 784*128 + 128 + 128*128 + 128 + 128*10 + 10 = 119066 = 1,9Mbit