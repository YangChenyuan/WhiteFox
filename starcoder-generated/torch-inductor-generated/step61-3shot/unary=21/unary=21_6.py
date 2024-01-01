
class MyModel(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 100, 3, stride=2)
        self.convtranspose = torch.nn.ConvTranspose2d(100, 50, 2, stride=2)
    def forward(self, x):
        v1 = self.conv(x)
        v2 = torch.tanh(v1)
        v3 = self.convtranspose(v2)
        return v3
# Inputs to the model
x = torch.randn(1, 3, 128, 128)