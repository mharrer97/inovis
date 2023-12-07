from collections import namedtuple
import torch
from torchvision import models



# https://github.com/pytorch/examples/blob/master/fast_neural_style/neural_style

def gram_matrix(y):
    (b, ch, h, w) = y.size()
    features = y.view(b, ch, w * h)
    features_t = features.transpose(1, 2)
    gram = features.bmm(features_t) / (ch * h * w)
    return gram

def normalize_batch(batch):
    # normalize using imagenet mean and std
    mean = batch.new_tensor([0.485, 0.456, 0.406]).view(-1, 1, 1)
    std = batch.new_tensor([0.229, 0.224, 0.225]).view(-1, 1, 1)
    # batch = batch.div_(255.0)
    return (batch - mean) / std

class Vgg16(torch.nn.Module):
    def __init__(self, requires_grad=False):
        super(Vgg16, self).__init__()
        vgg_pretrained_features = models.vgg16(weights=models.VGG16_Weights.DEFAULT).features
        self.slice1 = torch.nn.Sequential()
        self.slice2 = torch.nn.Sequential()
        self.slice3 = torch.nn.Sequential()
        self.slice4 = torch.nn.Sequential()
        for x in range(4):
            self.slice1.add_module(str(x), vgg_pretrained_features[x])
        for x in range(4, 9):
            self.slice2.add_module(str(x), vgg_pretrained_features[x])
        for x in range(9, 16):
            self.slice3.add_module(str(x), vgg_pretrained_features[x])
        for x in range(16, 23):
            self.slice4.add_module(str(x), vgg_pretrained_features[x])
        if not requires_grad:
            for param in self.parameters():
                param.requires_grad = False

    def forward(self, X):
        h = self.slice1(X)
        h_relu1_2 = h
        h = self.slice2(h)
        h_relu2_2 = h
        h = self.slice3(h)
        h_relu3_3 = h
        h = self.slice4(h)
        h_relu4_3 = h
        vgg_outputs = namedtuple("VggOutputs", ['relu1_2', 'relu2_2', 'relu3_3', 'relu4_3'])
        out = vgg_outputs(h_relu1_2, h_relu2_2, h_relu3_3, h_relu4_3)
        return out

class VGG_loss:
    def __init__(self, device):
        self.vgg = Vgg16().to(device)
        self.mse_loss = torch.nn.functional.mse_loss
        self.content_weight = 0.25
        self.style_weight = 3.0


    def __call__(self, tensor, groundtruth):
        features_groundtruth = self.vgg(normalize_batch(groundtruth))
        features_tensor = self.vgg(normalize_batch(tensor))

        content_loss = self.content_weight * self.mse_loss(features_groundtruth.relu2_2, features_tensor.relu2_2)

        gram_style = [gram_matrix(y) for y in features_groundtruth]
        style_loss = 0.
        for ft_y, gm_s in zip(features_tensor, gram_style):
            gm_y = gram_matrix(ft_y)
            style_loss += self.mse_loss(gm_y, gm_s)
        style_loss *= self.style_weight
        return style_loss + content_loss




#################################3
    