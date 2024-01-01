
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = torch.nn.Linear(24, 16)
 
    def forward(self, x, other):
        v1 = self.linear(x)
        v2 = v1 + other
        return v2


# Initializing the model
m = Model()

# Inputs to the model
x = torch.randn(1, 24)
other = torch.randn(1, 16)