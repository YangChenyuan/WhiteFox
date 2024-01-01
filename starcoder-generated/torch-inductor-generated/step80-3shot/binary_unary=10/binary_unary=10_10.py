
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = torch.nn.Linear(5, 5)
 
    def forward(self, x1):
        v1 = torch.ones(5, 5)
        v2 = self.linear(x1)
        v3 = v2 + v1
        v4 = torch.nn.functional.relu(v3)
        return v4
 
# Initializing the model
m = Model()

# Inputs to the model
x1 = torch.randn(5, 5)