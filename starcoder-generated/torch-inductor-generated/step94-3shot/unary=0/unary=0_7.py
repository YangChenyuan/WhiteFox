
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(5, 16, 1, stride=1, padding=0)
        self.relu = torch.nn.ReLU()
    def forward(self, x51):
        v1 = self.conv(x51)
        v2 = v1 * 0.5
        v3 = v1 * v1
        v4 = v3 * v1
        v5 = v4 * 0.044715
        v6 = v1 + v5
        v7 = v6 * 0.7978845608028654
        v8 = self.relu(v7)
        v9 = v8 + 1
        v10 = v2 * v9
        return v10
# Inputs to the model
x51 = torch.randn(1, 5, 8, 12)