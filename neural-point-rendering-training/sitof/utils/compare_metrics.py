from skimage.metrics import structural_similarity as ssim
from skimage.color import rgb2gray
from .tensor_utils import *

import skimage
import torch
import numpy as np

def ssim_map(inTensor, compareTensor, input_range = [-1,1]):
    with torch.no_grad():
        if len(inTensor.shape)!= 3 and len(compareTensor.shape)!= 3:
            print("input should be one tensor, not a batch; tensor shapes:", inTensor.shape, compareTensor.shape)

        in_np = inTensor.cpu().detach()
        com_np = compareTensor.cpu().detach()
        if input_range[0] != 0 or input_range[1] != 1:
            in_np -= input_range[0]
            in_np = in_np / (input_range[1]-input_range[0])
            com_np -= input_range[0]
            com_np = com_np / (input_range[1]-input_range[0])

        #scimage wants w x h x c structure
        in_np = rgb2gray(in_np.numpy().transpose(1,2,0))
        com_np = rgb2gray(com_np.numpy().transpose(1,2,0))
       
        #returns tupel: mssim (mean ssim) always
        #               gradient image if gradient=True
        #               ssim_image if full=True
        ssim_image = ssim(in_np, com_np, full=True)[1]
        return ssim_image


def difference_map_with_ssim(inTensor, compareTensor, input_range = [-1,1], threshold=0.1):
    with torch.no_grad():
        ssim_image = ssim_map(inTensor, compareTensor, input_range)
        ssim_image[ssim_image>threshold] = 1.0
        ssim_image[ssim_image<threshold] = 0.0
        return ssim_image