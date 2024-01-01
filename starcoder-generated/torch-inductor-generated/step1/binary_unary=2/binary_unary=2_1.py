
class Model(torch.nn.Module):
    def __init__(self, other):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 8, 3, stride=1, padding=1)
        self.other = torch.nn.Parameter(other, requires_grad=False)
 
    def forward(self, x):
        return F.relu(self.conv(x) - self.other)

# Initializing the model
m = Model(other=torch.randn(1, 8, 64, 64))

# Inputs to the model
x = torch.randn(1, 3, 64, 64)