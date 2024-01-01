
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = torch.nn.Conv2d(1, 2, 1, stride=1, padding=0)
        self.conv2 = torch.nn.Conv2d(2, 3, 1, stride=1, padding=0)
        self.conv3 = torch.nn.Conv2d(3, 4, (3, 4), stride=1, padding=0)
        self.conv4 = torch.nn.Conv2d(4, 5, (4, 3), stride=1, padding=0)
    def forward(self, x1):
        v1 = self.conv1(x1)
        v2 = v1 * 0.5
        v3 = v1 * 0.7071067811865476
        v4 = torch.erf(v3)
        v5 = v4 + 1
        v6 = v2 * v5
        v7 = self.conv2(v6)
        v8 = v7 * 0.5
        v9 = v7 * 0.7071067811865476
        v10 = torch.erf(v9)
        v11 = v10 + 1
        v12 = v8 * v11
        v13 = self.conv3(v12)
        v14 = self.conv4(v13)
        return v14
# Inputs to the model
x1 = torch.randn(1, 1, 64, 128)