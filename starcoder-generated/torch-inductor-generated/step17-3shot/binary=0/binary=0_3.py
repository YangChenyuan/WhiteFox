
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(12, 3, 1, stride=1, padding=1)
    def forward(self, x1, input_tensor, other=None, padding1=None, stride1=None):
        v1 = self.conv(x1)
        if other == None:
            other = torch.randn(v1.shape)
        v2 = v1 + other
        return v2
# Inputs to the model
input_tensor = torch.randn(1, 12, 64, 64)
x1 = torch.randn(1, 3, 64, 64)