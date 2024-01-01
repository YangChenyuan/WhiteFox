
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv_transpose = torch.nn.ConvTranspose2d(1, 16000, 200, stride=100, padding=0, output_padding=0)
    def forward(self, x1):
        v1 = self.conv_transpose(x1)
        v2 = v1 + 3
        v3 = torch.clamp(v2, min=-2)
        v4 = torch.clamp(v3, max=4.5)
        v5 = v1 * v4
        v6 = v5 / 6
        return v6
# Inputs to the model:
x1 = torch.randn(1, 1, 100, 100)