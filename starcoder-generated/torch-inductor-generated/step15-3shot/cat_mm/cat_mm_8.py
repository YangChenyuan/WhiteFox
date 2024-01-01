
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
    def forward(self, x1, x2):
        v1 = torch.mm(x1, x2)
        v2 = torch.cat([v1, v1, v1, v1, v1], 1)
        v3 = torch.mm(x1, x2)
        return torch.cat([v2, v3], 1)
# Inputs to the model
x1 = torch.randn(5, 2)
x2 = torch.randn(5, 1)