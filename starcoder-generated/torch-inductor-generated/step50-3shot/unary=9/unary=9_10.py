
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 8, 1, stride=1, padding=1)
    def forward(self, x1):
        v1 = torch.clamp_(self.conv(x1), min=0, max=6)
        v1 = v1.add_(3)
        v2 = v1.clamp(min=0, max=6)
        v3 = v2.div_(6)
        return v3
# Inputs to the model
x1 = torch.randn(8, 3, 64, 64)