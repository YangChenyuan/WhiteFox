
import math
class Model(torch.nn.Module):
    def __init__(self, other_tensor):
        super().__init__()
        self.linear = torch.nn.Linear(10, 10)
        self.other_tensor = other_tensor
 
    def forward(self, x1):
        v1 = self.linear(x1)
        v2 = v1 + self.other_tensor
        return v2

# Initializing the model
m = Model(torch.randn(10))

# Inputs to the model
x1 = torch.randn(1, 10)