
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 8, 1, stride=1, padding=1)
    def forward(self, x1):
        v1 = self.conv(x1)
        v2 = torch.add(v1, torch.tensor([3.0]).clone().detach())
        v3 = torch.clamp(v2, torch.tensor([0.0]).clone().detach(), torch.tensor([6.0]).clone().detach())
        v4 = v3.div(torch.tensor([6.0]).clone().detach())
        return v4
# Inputs to the model
x1 = torch.randn(1, 3, 64, 64)