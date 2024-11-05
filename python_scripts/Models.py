import torch.nn as nn


class FF_10_20_10(nn.Module):
    def __init__(self, input_size, num_classes):
        super(FF_10_20_10, self).__init__()
        self.layer1 = nn.Linear(input_size, 10)
        self.layer2 = nn.Linear(10, 20)
        self.layer3 = nn.Linear(20, 10)
        self.output_layer = nn.Linear(10, num_classes)
        self.relu = nn.ReLU()

    def forward(self, x):
        x = self.relu(self.layer1(x))
        x = self.relu(self.layer2(x))
        x = self.relu(self.layer3(x))
        x = self.output_layer(x)
        return x
