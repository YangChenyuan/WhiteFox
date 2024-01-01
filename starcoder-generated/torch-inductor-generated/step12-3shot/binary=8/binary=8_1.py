
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = torch.nn.Conv2d(3, 8, 1, stride=1, padding=1)
 
    def forward(self, x1, other=None):
        if other is None:
            other = torch.randn([1, 8, 64, 64])
        v1 = self.conv(x1)
        v2 = v1 + other
        return v2

# Initializing the model
m = Model()

# Inputs (not the real inputs, just dummy input in the correct order)
x1 = torch.randn(1, 3, 64, 64)
other = torch.randn(1, 8, 64, 64)