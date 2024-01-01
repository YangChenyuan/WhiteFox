
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = torch.nn.Conv2d(3, 64, 3, stride=1, padding=1)
        self.conv2 = torch.nn.Conv2d(64, 64, 3, stride=2, padding=1)
        self.conv3 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv4 = torch.nn.Conv2d(64, 64, 3, stride=2, padding=1)
        self.conv5 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv6 = torch.nn.Conv2d(64, 64, 3, stride=2, padding=1)
        self.conv7 = torch.nn.Conv2d(64, 64, 1, stride=1, padding=0)
        self.conv8 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv9 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv10 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv11 = torch.nn.Conv2d(64, 64, 3, stride=1, padding=1)
        self.conv12 = torch.nn.Conv2d(64, 32, 3, stride=1, padding=1)
        self.conv13 = torch.nn.Conv2d(32, 32, 1, stride=1, padding=0)
        self.conv14 = torch.nn.Conv2d(32, 32, 3, stride=1, padding=1)
        self.conv15 = torch.nn.Conv2d(32, 32, 3, stride=1, padding=1)
        self.conv16 = torch.nn.Conv2d(32, 16, 3, stride=1, padding=1)
        self.conv17 = torch.nn.Conv2d(16, 16, 3, stride=2, padding=1)
    def forward(self, x1):
        v1 = self.conv1(x1)
        v2 = torch.relu(v1)
        v3 = self.conv2(v2)
        v4 = torch.relu(v3)
        v5 = self.conv3(v4)
        v6 = torch.relu(v5)
        v7 = self.conv4(v6)
        v8 = torch.relu(v7)
        v9 = self.conv5(v8)
        v10 = torch.relu(v9)
        v11 = self.conv6(v10)
        v12 = torch.relu(v11)
        v13 = self.conv7(v12)
        v14 = torch.relu(v13)
        v15 = self.conv8(v14)
        v16 = torch.relu(v15)
        v17 = self.conv9(v16)
        v18 = torch.relu(v17)
        v19 = self.conv10(v18)
        v20 = torch.relu(v19)
        v21 = self.conv11(v20)
        v22 = torch.relu(v21)
        v23 = self.conv12(v22)
        v24 = torch.relu(v23)
        v25 = self.conv13(v24)
        v26 = torch.relu(v25)
        v27 = self.conv14(v26)
        v28 = torch.relu(v27)
        v29 = self.conv15(v28)
        v30 = torch.relu(v29)
        v31 = self.conv16(v30)
        v32 = torch.relu(v31)
        v33 = self.conv17(v32)
        v34 = torch.relu(v33)
        return v34
# Inputs to the model
x1 = torch.randn(1, 3, 256, 256)