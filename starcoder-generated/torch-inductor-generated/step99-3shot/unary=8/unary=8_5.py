
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv_transpose1 = torch.nn.ConvTranspose2d(64, 128, 3, stride=1)
        self.conv_transpose2 = torch.nn.ConvTranspose2d(128, 256, 3, stride=1)
        self.conv_transpose3 = torch.nn.ConvTranspose2d(256, 3, 3, stride=1)
    def forward(self, x1):
        v1 = self.conv_transpose1(x1)
        v2 = v1 + 3
        v3 = torch.clamp(v2, min=0)
        v4 = torch.clamp(v3, max=6)
        v5 = v1 * v4
        v6 = v5 / 6
        return v6
# Inputs to the model
x1 = torch.randn(1, 64, 56, 56)