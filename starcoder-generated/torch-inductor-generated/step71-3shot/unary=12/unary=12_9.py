
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 64, 3, stride=1, padding=1)
        self.pool = torch.nn.AdaptiveAvgPool2d((1,1))
    def forward(self, x1):
        v1 = self.conv(x1)
        v2 = self.pool(v1)
        return v2
# Inputs to the model
x1 = torch.randn(1, 3, 256, 256)