
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = torch.nn.Linear(10, 4)
 
    def forward(self, x1):
        v1 = self.linear(x1)
        v2 = v1 - 3
        return v2

# Initializing the model
m = Model()
v = torch.randn(1, 10)