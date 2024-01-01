
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = torch.nn.Linear(64, 10)
 
    def forward(self, x1):
        l1 = self.linear(x1)
        l2 = torch.tanh(l1)
        return l2

# Initializing the model
m = Model()

# Inputs to the model
x1 = torch.randn(8, 64)