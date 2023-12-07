from torchvision import transforms
import imgaug.augmenters as iaa
import imgaug as ia

from sitof.utils.tensor_utils import *

def map11(x):
    return 2.0*x - 1.0

## !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# don't use random transformations on \init\ transformations! (i.e. Rotate() or cropping with something not tagged as "center-center")
# the transformations will be applied differently on different images (i.e. Groundtruth will be transformed differently then Input)
#
# Onload transformations handle this, there all images are transformed correctly 
# (i.e. GT-image1 and Input-image1 are cropped randomly each epoch, but both the same way) 

################################################################
# standard transformations -- not used

################################################################
# standard color init transformation
# rescale image to match the shorter side and center crop a square
class RGB_InitTransform:
    def __init__(self, input_image_width_and_height):
        self.input_image_width_and_height = input_image_width_and_height

    def __call__(self, x):
        y = iaa.Sequential([
                iaa.Resize({'shorter-side': int(self.input_image_width_and_height),"longer-side": "keep-aspect-ratio"}), 
                          
                iaa.CropToFixedSize(int(self.input_image_width_and_height), int(self.input_image_width_and_height), position='center-center'),
                # iaa.CropToMultiplesOf(height_multiple=32, width_multiple=32),
            ]).augment_image(x)
        #y = map11(y) # maps values between -1 and 1 do for tanh, do not use with sigmoid
        return y[:,:,:3] # discard alpha
#---------------------------------------------------------------

################################################################
# standard data init transformation
# rescale image (nearest neighbour interpolation) to match the shorter side and center crop a square
class Data_InitTransform:
    def __init__(self, input_image_width_and_height):
        self.input_image_width_and_height = input_image_width_and_height

    def __call__(self, x):
        y = iaa.Sequential([
                iaa.Resize({'shorter-side':int(self.input_image_width_and_height),"longer-side": "keep-aspect-ratio"}, interpolation="nearest"), 
                         
                iaa.CropToFixedSize(int(self.input_image_width_and_height),int(self.input_image_width_and_height), position='center-center'),
            ]).augment_image(x)
        return y
#---------------------------------------------------------------

################################################################
# standard train onload transformation
# crop a square -- normal distribution
class Train_OnloadTransform:
    def __init__(self, image_width_and_height):
        self.image_width_and_height = image_width_and_height

    def __call__(self, x):
        y = iaa.Sequential([
                iaa.CropToFixedSize(self.image_width_and_height, self.image_width_and_height, position='normal'),
            ]).augment_image(x)
        return transforms.ToTensor()(y)
#---------------------------------------------------------------

################################################################
# standard val onload transformation
# crop a square -- center
class Val_OnloadTransform:
    def __init__(self, image_width_and_height):
        self.image_width_and_height = image_width_and_height

    def __call__(self, x):
        y = iaa.Sequential([
                iaa.CropToFixedSize(self.image_width_and_height, self.image_width_and_height, position='center-center'),
                iaa.CropToMultiplesOf(height_multiple=32, width_multiple=32),
            ]).augment_image(x)
        return transforms.ToTensor()(y)
#---------------------------------------------------------------

################################################################
# INOVIS transformations

################################################################
# INOVIS RGB init transformation
# ideally, this tranformation does nothing, as the input images are already in correct resolution.
# else: resize, if shorter side is too small and center crop the desired resolution
class INVOIS_RGB_InitTransform:
    def __init__(self, input_image_width_and_height):
        self.input_image_width_and_height = input_image_width_and_height
        if isinstance(input_image_width_and_height, int):
            self.input_image_width = self.input_image_width_and_height
            self.input_image_height = self.input_image_width_and_height
            self.shorter_target = self.input_image_height # doesnt matter, dimensions are the same
        else:
            self.input_image_width = self.input_image_width_and_height[0]
            self.input_image_height = self.input_image_width_and_height[1]
            self.shorter_target = self.input_image_width_and_height[0] if self.input_image_width_and_height[0] < self.input_image_width_and_height[1] else self.input_image_width_and_height[1]

    def __call__(self, x):
        shorter = x.shape[0] if x.shape[0] < x.shape[1] else x.shape[1]
        if shorter < self.shorter_target:
            y = iaa.Sequential([
                    iaa.Resize({'shorter-side': int(self.shorter_target),"longer-side": "keep-aspect-ratio"}),   
                    iaa.CropToFixedSize(int(self.input_image_width), int(self.input_image_height), position='center-center'),
                ]).augment_image(x)
        else:
            y = iaa.CropToFixedSize(int(self.input_image_width), int(self.input_image_height), position='center-center').augment_image(x)
        return y[:,:,:3] # discard alpha
#---------------------------------------------------------------

################################################################
# INOVIS GT init transformation
# center crop the desired resolution
class INOVIS_GTRGB_InitTransform:
    def __init__(self, input_image_width_and_height):
        self.input_image_width_and_height = input_image_width_and_height
        if isinstance(input_image_width_and_height, int):
            self.input_image_width = self.input_image_width_and_height
            self.input_image_height = self.input_image_width_and_height
        else:
            self.input_image_width = self.input_image_width_and_height[0]
            self.input_image_height = self.input_image_width_and_height[1]
    def __call__(self, x):
        y = iaa.CropToFixedSize(int(self.input_image_width), int(self.input_image_height), position='center-center').augment_image(x)
        return y[:,:,:3] # discard alpha
#---------------------------------------------------------------

################################################################
# INOVIS train onload transformation
# crop the desired resolution -- uniform distribution
class INOVIS_Train_OnloadTransform:
    #takes original and crop size as an array
    def __init__(self, image_width_and_height):
        self.input_image_width_and_height = image_width_and_height
        if isinstance(self.input_image_width_and_height, int):
            self.input_image_width = self.input_image_width_and_height
            self.input_image_height = self.input_image_width_and_height
        else:
            self.input_image_width = self.input_image_width_and_height[0]
            self.input_image_height = self.input_image_width_and_height[1]
    def __call__(self, x):
        y = iaa.CropToFixedSize(self.input_image_width, self.input_image_height, position='uniform').augment_image(x)
        return transforms.ToTensor()(y)
#---------------------------------------------------------------

################################################################
# INOVIS eval onload transformation
# center crop the desired resolution
class INOVIS_Val_OnloadTransform:
    def __init__(self, image_width_and_height):
        self.input_image_width_and_height = image_width_and_height
        if isinstance(self.input_image_width_and_height, int):
            self.input_image_width = self.input_image_width_and_height
            self.input_image_height = self.input_image_width_and_height
        else:
            self.input_image_width = self.input_image_width_and_height[0]
            self.input_image_height = self.input_image_width_and_height[1]
    def __call__(self, x):
        y = iaa.CropToFixedSize(self.input_image_width, self.input_image_height, position='center-center').augment_image(x)
        return transforms.ToTensor()(y)
#---------------------------------------------------------------

################################################################
# INOVIS no transformations
# return the input value to do nop transformation: used for auxiliary images

################################################################
# INOVIS no init transformation
class INOVIS_NoRGB_InitTransform:
    def __init__(self):
        return
    def __call__(self, x):
        return x
#---------------------------------------------------------------

################################################################
# INOVIS no train onload transformation
class INOVIS_NoTrain_OnloadTransform:
    def __init__(self):
        return
    def __call__(self, x):
        return transforms.ToTensor()(x)
#---------------------------------------------------------------

################################################################
# INOVIS no val onload transformation
class INOVIS_NoVal_OnloadTransform:
    def __init__(self):
        return
    def __call__(self, x):
        return transforms.ToTensor()(x)
#---------------------------------------------------------------
