
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = torch.nn.Linear(2, 2)
    def forward(self, x1):
        v1 = torch.nn.functional.linear(x1, self.linear.weight, self.linear.bias)
        v2 = v1.type_as(x1)
        return v2.permute(0, 2, 1)
# Inputs to the model
x1 = torch.randn(1, 2, 2)