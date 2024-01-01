
class Model(torch.nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = torch.nn.Conv2d(961, 924, 3, stride=1)
        self.conv2 = torch.nn.Conv2d(924, 874, 3, stride=2)
        self.conv3 = torch.nn.Conv2d(874, 841, 3, stride=2)
        self.conv4 = torch.nn.Conv2d(841, 818, 3, stride=2)
        self.conv5 = torch.nn.Conv2d(818, 582, 3, stride=2)
        self.conv6 = torch.nn.Conv2d(582, 550, 3, stride=2)
        self.conv7 = torch.nn.Conv2d(550, 304, 3, stride=2)
        self.conv8 = torch.nn.Conv2d(304, 278, 3, stride=2)
        self.conv9 = torch.nn.Conv2d(278, 162, 3, stride=2)
        self.conv10 = torch.nn.Conv2d(162, 134, 3, stride=2)
        self.conv11 = torch.nn.Conv2d(134, 82, 3, stride=2)
        self.conv12 = torch.nn.Conv2d(82, 62, 3, stride=2)
        self.conv13 = torch.nn.Conv2d(62, 32, 3, stride=2)
        self.conv14 = torch.nn.Conv2d(32, 20, 3, stride=2)
        self.conv15 = torch.nn.Conv2d(20, 10, 3, stride=2)
        self.conv16 = torch.nn.Conv2d(10, 8, 3, stride=2)
        self.conv17 = torch.nn.Conv2d(8, 6, 3, stride=2)
        self.conv18 = torch.nn.Conv2d(6, 4, 3, stride=2)
        self.conv19 = torch.nn.Conv2d(4, 4, 5, stride=2, padding=1)
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
        v35 = self.conv18(v34)
        v36 = torch.relu(v35)
        v0 = torch.nn.functional.max_pool2d(v36, kernel_size=[5, 5], stride=3, padding=0, ceil_mode=False)
        v37 = self.conv19(v0)
        return v37
# Inputs to the model
x1 = torch.randn(1, 961, 224, 224)