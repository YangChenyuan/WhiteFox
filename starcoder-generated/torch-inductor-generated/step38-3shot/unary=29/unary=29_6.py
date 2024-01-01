
class Model(torch.nn.Module):
    def __init__(self, min_value=-0.2609, max_value=-0.1837):
        super().__init__()
        self.conv1 = torch.zeros([1,1,41,40], dtype=torch.float32)
        self.conv1[0, :, [0,1,4,5], :]=[[[0,.1,1,4],[1,3,1,3],[0,1,3,1],[5,1,5,1]]]
        self.conv1[0, :, [2,3,6,7], :]=[[[2,-5,-1,6],[-2,-2,-1,-4],[-5,2,2,-3],[6,-4,-1,-6]]]
        self.conv1[0, :, [8,9,12,13], :]=[[[-1,-3,5,-2],[-1,-5,1,4],[0,-4,-5,-3],[-1,-4,-2,6]]]
        self.conv1[0, :, [10,11,14,15], :]=[[[0,-1,-3,4],[6,6,2,5],[-4,-6,-1,0],[-1,2,1,0]]]
        self.conv1[0, :, [16,17,20,21], :]=[[[1,4,1,6],[0,-3,1,0],[-4,4,-5,2],[-4,3,-4,-4]]]
        self.conv1[0, :, [18,19,22,23], :]=[[[5,-6,-4,1],[1,2,3,-2],[1,-6,-5,-3]],[[-2,4,-1,-1],[0,-2,2,3],[-4,5,-3,1],[0,0,-4,4]]]
        self.conv1[0, :, [24,25,28,29], :]=[[[0,3,5,-4],[-1,1,-3,-1],[2,-1,2,5],[-3,6,2,3]]]
        self.conv1[0, :, [26,27,30,31], :]=[[[-5,2,3,-6],[-2,-2,-6,-6],[-5,-4,5,5],[3,-6,-3,5]]]
        self.conv1[0, :, [32,33,36,37], :]=[[[0,0,4,-4],[3,6,5,2],[-3,2,-2,-4],[-5,-3,3,3]]]
        self.conv1[0, :, [34,35,38,39], :]=[[[0,-1,-2,3],[-1,0,-5,-2],[4,6,-1,-2],[0,-1,4,-2]]]
        self.conv2 = torch.zeros([1,1,41,40], dtype=torch.float32)
        self.conv2[0, :, [0,1,4,5], :]=[[[-2,-1,-1,0],[0,1,0,-1],[-6,3,-5,5],[-4,0,-2,-3]]]
        self.conv2[0, :, [2,3,6,7], :]=[[[-1,3,-3,2],[-2,-1,0,1],[-2,4,-2,-6],[-2,0,4,1]]]
        self.conv2[0, :, [8,9,12,13], :]=[[[3,5,4,-1],[-1,6,-3,4],[0,-3,1,0],[-5,-2,-5,1]]]
        self.conv2[0, :, [10,11,14,15], :]=[[[4,-2,5,0],[-3,-2,1,0],[5,3,5,4],[4,-3,0,5]]]
        self.conv2[0, :, [16,17,20,21], :]=[[[-2,-1,1,3],[1,-6,-2,5],[-3,6,-2,-4],[1,0,3,-2]]]
        self.conv2[0, :, [18,19,22,23], :]=[[[-1,-5,-6,4],[-1,-6,-1,-2],[4,4,-6,4],[-5,-3,0,-2]]]
        self.conv2[0, :, [24,25,28,29], :]=[[[1,-6,-5,-5],[-3,-6,2,0],[1,0,3,-1],[-2,-5,4,-4]]]
        self.conv2[0, :, [26,27,30,31], :]=[[[5,1,-1,4],[-1,-4,-3,1],[1,-4,-2,-3],[3,-3,-4,3]]]
        self.conv2[0, :, [32,33,36,37], :]=[[[-2,3,0,-3],[0,-2,2,-6],[-5,5,-4,-5],[4,0,-3,3]]]
        self.conv2[0, :, [34,35,38,39], :]=[[[-6,1,1,3],[-6,-3,6,1],[1,1,1,1],[1,-5,-1,-6]]]
        self.conv_transpose = torch.nn.ConvTranspose2d(2, 3, 3, stride=19, padding=0)
        self.min_value = min_value
        self.max_value = max_value
    def forward(self):
        v1 = self.conv_transpose(self.conv1 + self.conv2)
        v2 = torch.clamp_min(v1, self.min_value)
        v3 = torch.clamp_max(v2, self.max_value)
        return v3
# Inputs to the model