
class Model(torch.nn.Module):
    def __init__(self, min, max):
        super().__init__()
        self.pointwise1 = torch.nn.Conv2d(464, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise2 = torch.nn.Conv2d(528, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise3 = torch.nn.Conv2d(153, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise4 = torch.nn.Conv2d(144, 512, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.maxpooling1 = torch.nn.MaxPool2d(kernel_size=(1, 3), stride=(1, 1), padding=(0, 0))
        self.maxpooling2 = torch.nn.MaxPool2d(kernel_size=(3, 1), stride=(1, 1), padding=(0, 0))
        self.maxpooling3 = torch.nn.MaxPool2d(kernel_size=(3, 1), stride=(1, 1), padding=(0, 0))
        self.maxpooling4 = torch.nn.MaxPool2d(kernel_size=(14, 1), stride=(1, 1), padding=(0, 0))
        self.pointwise5 = torch.nn.Conv2d(528, 928, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise6 = torch.nn.Conv2d(1536, 972, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise7 = torch.nn.Conv2d(236, 3136, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise8 = torch.nn.Conv2d(228, 2048, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise9 = torch.nn.Conv2d(928, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise10 = torch.nn.Conv2d(972, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise11 = torch.nn.Conv2d(3136, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise12 = torch.nn.Conv2d(2048, 640, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise13 = torch.nn.Conv2d(228, 144, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise14 = torch.nn.Conv2d(1152, 320, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise15 = torch.nn.Conv2d(228, 144, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise16 = torch.nn.Conv2d(1152, 320, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise17 = torch.nn.Conv2d(228, 144, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise18 = torch.nn.Conv2d(1152, 320, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise19 = torch.nn.Conv2d(228, 144, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise20 = torch.nn.Conv2d(1152, 320, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.minmaxpooling1 = torch.nn.AdaptiveAvgPool2d(output_size=(1, 1))
        self.pointwise21 = torch.nn.Conv2d(528, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise22 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise23 = torch.nn.Conv2d(236, 1296, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise24 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise25 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise26 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise27 = torch.nn.Conv2d(236, 1296, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise28 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise29 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise30 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise31 = torch.nn.Conv2d(236, 1296, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise32 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise33 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise34 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise35 = torch.nn.Conv2d(236, 1296, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise36 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise37 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise38 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise39 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise40 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise41 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise42 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise43 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise44 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise45 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise46 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise47 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise48 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise49 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise50 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise51 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise52 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise53 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise54 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise55 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise56 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise57 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise58 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise59 = torch.nn.Conv2d(236, 528, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise60 = torch.nn.Conv2d(228, 1024, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise61 = torch.nn.Conv2d(972, 2, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise62 = torch.nn.Conv2d(1536, 16, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise63 = torch.nn.Conv2d(236, 768, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise64 = torch.nn.Conv2d(228, 64, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise65 = torch.nn.Conv2d(1384, 192, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise66 = torch.nn.Conv2d(228, 64, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise67 = torch.nn.Conv2d(1384, 192, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise68 = torch.nn.Conv2d(22, 90, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise69 = torch.nn.Conv2d(228, 64, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise70 = torch.nn.Conv2d(1384, 192, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise71 = torch.nn.Conv2d(228, 8, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.pointwise72 = torch.nn.Conv2d(8, 256, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=False)
        self.flatten1 = torch.nn.Flatten(start_dim=1)
        self.pointwise73 = torch.nn.Conv2d(1984, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise74 = torch.nn.Conv2d(3600, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise75 = torch.nn.Conv2d(1984, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise76 = torch.nn.Conv2d(3600, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise77 = torch.nn.Conv2d(1984, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise78 = torch.nn.Conv2d(3600, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise79 = torch.nn.Conv2d(1984, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise80 = torch.nn.Conv2d(3600, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise81 = torch.nn.Conv2d(1984, 1, kernel_size=(1, 1), stride=(1, 1), padding=(0, 0), bias=True)
        self.pointwise82 = tor