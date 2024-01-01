
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(1, 64, (1, 7), stride=1, padding=(0, 3))
        self.conv2 = torch.nn.Conv2d(64, 5, (1, 1), stride=1, padding=0)
    def forward(self, x1):
        v1 = self.conv(x1)
        return v1
# Inputs to the model
x1 = torch.randn(1, 1, 576, 512)