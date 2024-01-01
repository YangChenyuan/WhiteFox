
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.in_features = 128
        self.linear = torch.nn.Linear(self.in_features, 6)
 
    def forward(self, x1):
        v1 = self.linear(x1)
        v2 = torch.tanh(v1)
        return v2

# Initializing the model
m = Model()

# Inputs to the model
x1 = torch.randn(1, 128)