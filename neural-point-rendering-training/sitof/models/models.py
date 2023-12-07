""" Full assembly of the parts to form the complete network """

from multiprocessing.connection import wait
import torch.nn.functional as F
from torch.nn import init 
import torch.nn as nn
from sitof.utils.tensor_utils import *
from sitof.utils.color_conversion import *
#######################################################################################################
# INOVIS - Instant Novel View Synthesis
# combine Neural Point-Based Graphics of Aliev et al. (reconstruction network) with Neural Supersampling of Xiao et al. (general)
# Neural Point-BasedGraphics Implementation from https://github.com/alievk/npbg/blob/master/npbg/models/unet.py

################################################################
# Feature extraction network
class INOVIS_Feature_Extraction(nn.Module):
    def __init__(
        self,
        in_channels = 4,
        out_channels = 12,
        conv_block = 'basic',#'gated' # 'basic'
        filter_base = 32
        ):
        super().__init__()
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # convolutions
        print("Feature Extraction: filter_base is ", filter_base)
        assert in_channels < out_channels
        if(conv_block == 'basic'):
            self.conv_inx32 = nn.Conv2d(in_channels,filter_base,3,padding=1)
            self.conv_32x32 = nn.Conv2d(filter_base,filter_base,3,padding=1)
            self.conv_32xout = nn.Conv2d(filter_base,out_channels-in_channels,3,padding=1)
        elif conv_block == 'gated':
            self.conv_inx32 = GatedBlock(in_channels,filter_base)
            self.conv_32x32 = GatedBlock(filter_base,filter_base)
            self.conv_32xout = GatedBlock(filter_base,out_channels-in_channels)
        else:
            print("ERROR: Feature Extraction got invalid convolution block!")
        #---------------------------------------------------------------

    def forward(self, input):
        ################################################################
        # network
        x = self.conv_inx32(input)
        x = F.relu(x)
        x = self.conv_32x32(x)
        x = F.relu(x)
        x = self.conv_32xout(x)
        x = F.relu(x)
        return torch.concat((input,x),1)
        #---------------------------------------------------------------

################################################################
# Feature extraction network including downsampling for gt images
class INOVIS_Feature_Extraction2(nn.Module):
    def __init__(
        self,
        in_channels = 4,
        out_channels = 16,
        depth = 0, # how much the input is downscaled. 0 == no downscale, 1 == times 2, 2 == times 4, 3 == times 8...
        conv_block = 'gated',#'gated' # 'basic'
        filter_base = 16
        
        ):
        super().__init__()
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # general parameters
        assert depth < 4 and depth >= 0
        self.depth = depth
        self.down = nn.AvgPool2d(2, 2)
        #---------------------------------------------------------------
        # convolutions
        print("Feature Extraction2: filter_base is ", filter_base)
        if(conv_block == 'basic'):
            self.conv_inx16 = nn.Conv2d(in_channels,filter_base,3,padding=1)
            self.conv_16x16 = nn.Conv2d(filter_base,filter_base,3,padding=1)
            if self.depth > 0:
                self.conv_16x32 = nn.Conv2d(filter_base,filter_base*2,3,padding=1)
                self.conv_32x32 = nn.Conv2d(filter_base*2,filter_base*2,3,padding=1)
                if self.depth > 1:
                    self.conv_32x64 = nn.Conv2d(filter_base*2,filter_base*4,3,padding=1)
                    self.conv_64x64 = nn.Conv2d(filter_base*4,filter_base*4,3,padding=1)
                    if self.depth > 2:
                        self.conv_64x128 = nn.Conv2d(filter_base*4,filter_base*8,3,padding=1)
                        self.conv_128x128 = nn.Conv2d(filter_base*8,filter_base*8,3,padding=1)
            channels_end = filter_base * 2**depth
            self.conv_out = nn.Conv2d(channels_end,out_channels,3,padding=1)
        elif conv_block == 'gated':
            self.conv_inx16 = GatedBlock(in_channels,filter_base)
            self.conv_16x16 = GatedBlock(filter_base,filter_base)
            if self.depth > 0:
                self.conv_16x32 = GatedBlock(filter_base,filter_base*2)
                self.conv_32x32 = GatedBlock(filter_base*2,filter_base*2)
                if self.depth > 1:
                    self.conv_32x64 = GatedBlock(filter_base*2,filter_base*4)
                    self.conv_64x64 = GatedBlock(filter_base*4,filter_base*4)
                    if self.depth > 2:
                        self.conv_64x128 = GatedBlock(filter_base*4,filter_base*8)
                        self.conv_128x128 = GatedBlock(filter_base*8,filter_base*8)
            channels_end = filter_base * 2**depth
            self.conv_out = GatedBlock(channels_end,out_channels)
        else:
            print("ERROR: Feature Extraction got invalid convolution block!")
        #---------------------------------------------------------------

    def forward(self, input):
        ################################################################
        # feature extraction
        #---------------------------------------------------------------
        # convolutions without downsampling
        x = self.conv_inx16(input)
        x = F.relu(x)
        x = self.conv_16x16(x)
        x = F.relu(x)
        #---------------------------------------------------------------
        # (Optional) convolutions with downsampling
        if self.depth > 0:
            x = self.down(x)
            x = self.conv_16x32(x)
            x = F.relu(x)
            x = self.conv_32x32(x)
            x = F.relu(x)
            if self.depth > 1:
                x = self.down(x)
                x = self.conv_32x64(x)
                x = F.relu(x)
                x = self.conv_64x64(x)
                x = F.relu(x)
                if self.depth > 2:
                    x = self.down(x)
                    x = self.conv_64x128(x)
                    x = F.relu(x)
                    x = self.conv_128x128(x)
                    x = F.relu(x)
        #---------------------------------------------------------------
        # final convolutions to create wanted output channels
        x = self.conv_out(x)
        x = F.relu(x)
        return x
        #---------------------------------------------------------------

################################################################
# Reweighting network that uses only the rgbd of gt frames to create a reweighting map
class INOVIS_Feature_Reweighting(nn.Module):
    def __init__(self,
        in_channels_current         = 4,       # channels of input for the current frame. only rgbd needed by default (current frame is not reweighted)
        in_channels_gt              = 4,       # channels of input for previous frames. 4 for rgbd by default. These channels are used for creating the reweighting. 
                                               # The amount of features in total do not matter since they are all reweighted with the same weights resulting from the network
        previous_frame_amount       = 3,       # number of previous frames feeded to the network
        conv_block                  = "basic",
        disable                     = False    # disables the module and passes through the original features
        ):
        super().__init__()
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # input variables
        assert previous_frame_amount >=1, f'declared less than 1 previous frames. nothing to do here'
        self.previous_frame_amount = previous_frame_amount
        self.in_channels_current = in_channels_current
        self.in_channels_gt = in_channels_gt
        self.output_map = False
        self.warp_counter = 0
        self.disable = disable
        #---------------------------------------------------------------
        # convolutions variables
        if(conv_block == 'basic'):
            self.conv_inx32 = nn.Conv2d(in_channels_current+in_channels_gt*previous_frame_amount,32,3, padding=1)
            self.conv_32x32 = nn.Conv2d(32,32,3,padding=1)
            self.conv_32xpreviousframes = nn.Conv2d(32,previous_frame_amount,3,padding=1)
        if(conv_block == 'gated'):
            self.conv_inx32 = GatedBlock(in_channels_current+in_channels_gt*previous_frame_amount,32)
            self.conv_32x32 = GatedBlock(32,32)
            self.conv_32xpreviousframes = GatedBlock(32,previous_frame_amount)
        #---------------------------------------------------------------

    def forward(self,
        input_current,          # input tensor of the current frame. should have in_channels_current channels
        *input_previous         # input tensors of the previous frames. should be list with previous_frame_amount entries and more than in_channels_gt each.
        ):
        ################################################################
        # reweighting
        #---------------------------------------------------------------
        # assert correct inputs
        assert input_current.shape[1] == self.in_channels_current, f'got current frame with {input_current.shape[1]} channels, but declared {self.in_channels_current} channels'
        assert len(input_previous) == self.previous_frame_amount, f'got {len(input_previous)} frames, but expected {self.previous_frame_amount}'
        for i in range(self.previous_frame_amount):
            assert input_previous[i].shape[1] >= self.in_channels_gt, f'got previous frame {i} with {input_previous[i].shape[1]} channels, but declared a minimum of {self.in_channels_gt} channels for input for the network + an arbitrary amopunt of additional features to be reweighted'
        
        #---------------------------------------------------------------
        # skip if reweighting is disabled
        if not self.disable:
            #---------------------------------------------------------------
            # create network input from current and previous frames
            concatenated = input_current
            for i in range(self.previous_frame_amount):
                concatenated = torch.cat((concatenated,input_previous[i][:,0:4,:,:]),1)
            #---------------------------------------------------------------
            # network / convolutions
            x = self.conv_inx32(concatenated)
            x = F.relu(x)
            x = self.conv_32x32(x)
            x = F.relu(x)
            x = self.conv_32xpreviousframes(x)
            x = torch.tanh(x)
            #---------------------------------------------------------------
            # save map if output is wanted
            if self.output_map:
                for image in range(x.shape[0]):
                    write_tensor_as_image((1+x[image,0:1,:,:])*0.5, "./output_warped/" + str(x.shape[2]) + "_" + str(self.warp_counter) + "_" + str(image) + "_" + "4map.png", input_range=[0,1])
            #---------------------------------------------------------------
            # Right now, x is tensor with previous_frame_amount channels, with each entry in range [-1:1] 
            # -> rescale to [0:10] as mentioned in paper -> add 1 and mul 5 
            x = torch.mul(torch.add(x,1),5)
            #---------------------------------------------------------------
            # Actual reweighting of the whole previous frame tensors
            reweighted = torch.mul(input_previous[0],x[:,0:1,:,:])
            for i in range(1,self.previous_frame_amount):
                reweighted = torch.cat((reweighted,torch.mul(input_previous[i],x[:,i:i+1,:,:])),1)
            return reweighted
        # pass through original features if reweighting is disabled
        else:
            reweighted = input_previous[0]
            for i in range(1,self.previous_frame_amount):
                reweighted = torch.cat((reweighted,input_previous[i]),1)
            return reweighted
        #---------------------------------------------------------------

################################################################
# Reweighting network that uses all features of the gt frames to create a reweighting map
class INOVIS_Feature_Reweighting2(nn.Module):
    def __init__(self,
        in_channels_current         = 4,       # channels of input for the current frame. only rgbd needed by default (current frame is not reweighted)
        in_channels_gt              = 4,       # channels of input for previous frames. 4 for rgbd by default. These channels are used for creating the reweighting. 
                                               # The amount of features in total do not matter since they are all reweighted with the same weights resulting from the network
        previous_frame_amount       = 3,       # number of previous frames feeded to the network
        conv_block                  = "basic",
        disable                     = False    # disables the module and passes through the original features
        ):
        super().__init__()
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # input variables
        assert previous_frame_amount >=1, f'declared less than 1 previous frames. nothing to do here'
        self.previous_frame_amount = previous_frame_amount
        self.in_channels_current = in_channels_current
        self.in_channels_gt = in_channels_gt
        self.disable = disable
        #---------------------------------------------------------------
        # convolutions variables
        if(conv_block == 'basic'):
            self.conv_inx32 = nn.Conv2d(in_channels_current+in_channels_gt*previous_frame_amount,32,3, padding=1)
            self.conv_32x32 = nn.Conv2d(32,32,3,padding=1)
            self.conv_32xpreviousframes = nn.Conv2d(32,previous_frame_amount,3,padding=1)
        if(conv_block == 'gated'):
            self.conv_inx32 = GatedBlock(in_channels_current+in_channels_gt*previous_frame_amount,32)
            self.conv_32x32 = GatedBlock(32,32)
            self.conv_32xpreviousframes = GatedBlock(32,previous_frame_amount)
        #---------------------------------------------------------------

    def forward(self,
        input_current,          # input tensor of the current frame. should have in_channels_current channels
        *input_previous         # input tensors of the previous frames. should be list with previous_frame_amount entries and more than in_channels_gt each.
        ):
        ################################################################
        # reweighting
        #---------------------------------------------------------------
        # assert correct inputs
        assert input_current.shape[1] == self.in_channels_current, f'got current frame with {input_current.shape[1]} channels, but declared {self.in_channels_current} channels'
        assert len(input_previous) == self.previous_frame_amount, f'got {len(input_previous)} frames, but expected {self.previous_frame_amount}'
        for i in range(self.previous_frame_amount):
            assert input_previous[i].shape[1] >= self.in_channels_gt, f'got previous frame {i} with {input_previous[i].shape[1]} channels, but declared a minimum of {self.in_channels_gt} channels for input for the network + an arbitrary amopunt of additional features to be reweighted'
        #---------------------------------------------------------------
        # skip if reweighting is disabled
        if not self.disable:
            #---------------------------------------------------------------
            # create network input from current and previous frames
            concatenated = input_current
            for i in range(self.previous_frame_amount):
                concatenated = torch.cat((concatenated,input_previous[i][:,:,:,:]),1)
            #---------------------------------------------------------------
            # network / convolutions
            x = self.conv_inx32(concatenated)
            x = F.relu(x)
            x = self.conv_32x32(x)
            x = F.relu(x)
            x = self.conv_32xpreviousframes(x)
            x = torch.tanh(x)
            #---------------------------------------------------------------
            # Right now, x is tensor with previous_frame_amount channels, with each entry in range [-1:1] 
            # -> rescale to [0:10] as mentioned in paper -> add 1 and mul 5 
            x = torch.mul(torch.add(x,1),5)
            #---------------------------------------------------------------
            # Actual reweighting of the whole previous frame tensors
            reweighted = torch.mul(input_previous[0],x[:,0:1,:,:])
            for i in range(1,self.previous_frame_amount):
                reweighted = torch.cat((reweighted,torch.mul(input_previous[i],x[:,i:i+1,:,:])),1)
            return reweighted
        # pass through original features if reweighting is disabled
        else:
            reweighted = input_previous[0]
            for i in range(1,self.previous_frame_amount):
                reweighted = torch.cat((reweighted,input_previous[i]),1)
            return reweighted
        #---------------------------------------------------------------

################################################################
# Reconstruction network taking input images at different resolutions to create an image in a UNet
class INOVIS_Reconstruction(nn.Module):
    '''
    Unet from Neural Point-Based Graphics by Aliev et al. Used in INOVIS for the Reconstruction step
    base for Implementation from https://github.com/alievk/npbg/blob/master/npbg/models/unet.py
    '''
    r""" Rendering network with UNet architecture and multi-scale input.
    Args:
        num_input_channels:     Number of channels in the input tensor or list of tensors. An integer or a list of integers for each input tensor.
        num_output_channels:    Number of output channels.
        additional_channels:    Additional channels for the network
        filter_sizes:                Feature tensor sizes of the UNet
        feature_scale:          Division factor of number of convolutional channels. The bigger the less parameters in the model.
        more_layers:            Additional down/up-sample layers.
        upsample_mode:          One of 'deconv', 'bilinear' or 'nearest' for ConvTranspose, Bilinear or Nearest upsampling.
        pooling_mode:           One of 'avg' or 'max'
        norm_layer:             [unused] One of 'bn', 'in' or 'none' for BatchNorm, InstanceNorm or no normalization. Default: 'bn'.
        last_act:               Last layer activation. One of 'sigmoid', 'tanh' or None.
        conv_block:             Type of convolutional block, like Convolution-Normalization-Activation. One of 'basic', 'partial' or 'gated'.
    """
    def __init__(
        self,
        num_input_channels=3, 
        num_output_channels=3,
        additional_channels=[0,0,0,0,0],
        filter_sizes = [64, 128, 256, 512, 1024] ,
        feature_scale=4,
        more_layers=0,
        upsample_mode='deconv', #'bilinear',
        pooling_mode='avg', #'bilinear',
        norm_layer='bn',
        last_act='sigmoid',
        conv_block= 'gated' #'gated' #'partial'
    ):
        super().__init__()
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # input variables
        self.feature_scale = feature_scale
        self.more_layers = more_layers
        if isinstance(num_input_channels, int):
            num_input_channels = [num_input_channels]
        if len(num_input_channels) < 5:
            num_input_channels += [0] * (5 - len(num_input_channels))
        self.num_input_channels = num_input_channels[:5]
        #---------------------------------------------------------------
        # specify convolution block
        if conv_block == 'basic':
            self.conv_block = BasicBlock
        elif conv_block == 'gated':
            self.conv_block = GatedBlock
        else:
            raise ValueError('bad conv block {}'.format(conv_block))
        #---------------------------------------------------------------
        # compute filter channels
        up_filters = filter_sizes.copy()
        down_filters = filter_sizes.copy()
        #---------------------------------------------------------------
        # add additional custom filters
        print("Original Down Filter Dimensions", down_filters)
        for i in range(len(additional_channels)):
            down_filters[i] += additional_channels[i]
        print("Additional Down Filter Dimensions", additional_channels)
        print("New Down Filter Dimensions", down_filters)
        print("New Up Filter Dimensions", up_filters)
        #---------------------------------------------------------------
        # configure Downsampling steps
        self.start = self.conv_block(self.num_input_channels[0], down_filters[0])
        self.down1 = DownsampleBlock(down_filters[0], down_filters[1] - self.num_input_channels[1], conv_block=self.conv_block, pooling_mode=pooling_mode)
        self.down2 = DownsampleBlock(down_filters[1], down_filters[2] - self.num_input_channels[2], conv_block=self.conv_block, pooling_mode=pooling_mode)
        self.down3 = DownsampleBlock(down_filters[2], down_filters[3] - self.num_input_channels[3], conv_block=self.conv_block, pooling_mode=pooling_mode)
        self.down4 = DownsampleBlock(down_filters[3], down_filters[4] - self.num_input_channels[4], conv_block=self.conv_block, pooling_mode=pooling_mode)
        #---------------------------------------------------------------
        # (Optional) configure additional layers if specified
        if self.more_layers > 0:
            self.more_downs = [
                DownsampleBlock(down_filters[4], up_filters[4], conv_block=self.conv_block, pooling_mode=pooling_mode) for i in range(self.more_layers)]
            self.more_ups = [UpsampleBlock2(up_filters[4], upsample_mode, same_num_filt =True, conv_block=self.conv_block) for i in range(self.more_layers)]

            self.more_downs = ListModule(*self.more_downs)
            self.more_ups   = ListModule(*self.more_ups)
        #---------------------------------------------------------------
        # configure Upsampling steps
        print("up4 ",  down_filters[3], ", ", up_filters[4], ", " , up_filters[3])
        print("up3 ",  down_filters[2], ", ", up_filters[3], ", " , up_filters[2])
        print("up2 ",  down_filters[1], ", ", up_filters[2], ", " , up_filters[1])
        print("up1 ",  down_filters[0], ", ", up_filters[1], ", " , up_filters[0])

        self.up4 = UpsampleBlock2(down_filters[3], up_filters[4], up_filters[3], upsample_mode, conv_block=self.conv_block)
        self.up3 = UpsampleBlock2(down_filters[2], up_filters[3], up_filters[2], upsample_mode, conv_block=self.conv_block)
        self.up2 = UpsampleBlock2(down_filters[1], up_filters[2], up_filters[1], upsample_mode, conv_block=self.conv_block)
        self.up1 = UpsampleBlock2(down_filters[0], up_filters[1], up_filters[0], upsample_mode, conv_block=self.conv_block)
        self.final = nn.Sequential(
            nn.Conv2d(up_filters[0], num_output_channels, 1)
        )
        #---------------------------------------------------------------
        # configure last activation function
        if last_act == 'sigmoid':
            self.final = nn.Sequential(self.final, nn.Sigmoid())
        elif last_act == 'tanh':
            self.final = nn.Sequential(self.final, nn.Tanh())
        #---------------------------------------------------------------

    def forward(self, *inputs, **kwargs):
        ################################################################
        # reconstruction
        #---------------------------------------------------------------
        # parse input
        inputs = list(inputs)
        #---------------------------------------------------------------
        # make sure enough input was provided 
        n_input = len(inputs)
        n_declared = np.count_nonzero(self.num_input_channels)
        assert n_input == n_declared, f'got {n_input} input scales but declared {n_declared}'
        #---------------------------------------------------------------
        # start with downsamplings. concat input on each level provided
        in64 = self.start(inputs[0])
        down1 = self.down1(in64)      
        #print("down1: ", down1.shape)
        if self.num_input_channels[1]:
            down1 = torch.cat([down1, inputs[1]], 1)
        down2 = self.down2(down1)
        #print("down2: ", down2.shape)
        if self.num_input_channels[2]:
            down2 = torch.cat([down2, inputs[2]], 1)
        down3 = self.down3(down2)
        #print("down3: ", down3.shape)
        if self.num_input_channels[3]:
            down3 = torch.cat([down3, inputs[3]], 1)
        down4 = self.down4(down3)
        #print("down4: ", down4.shape)
        if self.num_input_channels[4]:
            down4 = torch.cat([down4, inputs[4]], 1)
        #---------------------------------------------------------------
        # additional steps if specified
        if self.more_layers > 0:
            prevs = [down4]
            for kk, d in enumerate(self.more_downs):
                out = d(prevs[-1])
                prevs.append(out)

            up_ = self.more_ups[-1](prevs[-1], prevs[-2])
            for idx in range(self.more_layers - 1):
                l = self.more_ups[self.more - idx - 2]
                up_= l(up_, prevs[self.more - idx - 2])
        else:
            up_= down4
        #---------------------------------------------------------------
        # upsampling steps
        up4 = self.up4(up_, down3)
        #print("up4: ", up4.shape)
        up3 = self.up3(up4, down2)
        #print("up3: ", up3.shape)
        up2 = self.up2(up3, down1)
        #print("up2: ", up2.shape)
        up1 = self.up1(up2, in64)
        #print("up1: ", up1.shape)
        return self.final(up1)
        #---------------------------------------------------------------

################################################################
# backward warping helper function for pixelwise warping
def warp_back(feature, movec):
    b, c, h, w = feature.size()
    # create a grid that contains the view space coorinates of the corresponding pixels
    gridx, gridy = torch.meshgrid(torch.linspace(-1,1,h), #grid coordinates
                                  torch.linspace(-1,1,w), #grid coordinates
                                  indexing='ij')
    # do with gpuif available
    if torch.cuda.is_available():
        gridx = gridx.cuda()
        gridy = gridy.cuda()
    neutral = torch.stack((gridy, gridx), 2)
    neutral = torch.unsqueeze(neutral,0) # according to documentatiuon of grid sample, the grid needs a batch dimension -> (b,h,w,2)
    # change channels around for adding operation. neutral : (1,h,w,2), movec: (b,2,h,w) -> (b,h,w,2) -> put the actual movecs in the back
    movec_permutation = movec.clone().permute(0,2,3,1)
    # movecs are pixel wise -> a movement over the whole image means +- res -> division by res means movement over whole image is +-1
    # but grid_sample(->neutral) takes [-1,1] -> movement over whole image is +-2 -> multiply movecs by 2
    movec_permutation[:,:,:,0:1] = torch.mul(movec_permutation[:,:,:,0:1],  2.0 / (w-1))           # w-1 to comprehend for opengl pixel positions being at 0.5: e.g. for w=32 gl has positions 0.5, 1.5, 2.5, ... , 31.5, while linspace is 0, 1, 2, ..., 31 -> movecs relate to space of 1 pixel more
    # mul with -1 because y axis of gl and python are flipped
    movec_permutation[:,:,:,1:2] = torch.mul(movec_permutation[:,:,:,1:2],  -2.0  / (h-1))    # h-1 to comprehend for opengl pixel positions being at 0.5: e.g. for h=32 gl has positions 0.5, 1.5, 2.5, ... , 31.5, while linspace is 0, 1, 2, ..., 31 -> movecs relate to space of 1 pixel more
    movedgrid = neutral - movec_permutation # applieas neutral grid to all 6 batches as well

    feature_moved = F.grid_sample(feature, movedgrid, mode = 'nearest', padding_mode = 'zeros') # nearest und bilinear testen 
    return feature_moved

################################################################
# backward warping helper function for warping using a texture corrdinate as motion vector
def warp_back_tc(feature, movec):
    movec_permutation = movec.clone().permute(0,2,3,1)
    movec_permutation[:,:,:,1:2] = (movec_permutation[:,:,:,1:2] * (-1))
    feature_moved = F.grid_sample(feature, movec_permutation, mode = 'nearest', padding_mode = 'zeros') # nearest und bilinear testen 
    return feature_moved
    
################################################################
# general network that chains up all parts
class INOVIS(nn.Module):
    '''
    INOVIS network: Implementation of the Neural Point Rendering Pipeline 
    '''
    r""" Point Rendering network with UNet architecture, multi-scale and multi-view input and feature reweighting.
    Args:
        in_channels_current:        number of channels the current frame input contains.      Can be int or list of ints (for multi resolution input)
        in_channels_gt:             number of channels the previous frames input contains.
        in_gt_frame_amount:         number of ground truth frames feeded to the network
        feature_amount_current:     number of additional features created by the feature extraction network for the current frame
        feature_amount_gt:          number of additional features created by the feature extraction network for the gt frames per frame
        feature_extraction_depth:   DEPRECATED. warping and thus feature extraction on lower resolutions is not supported anymore
        out_channels:               number of output channels
        output_warped:              specify if warped images should be written to disk
        mask_warped:                specify if warped image should be masked by multiplying with 3rd channel of the movec 
        warp_rgbd:                  specify if rgbd of gt should be warped and appended to the reweighting network
        extract_lowres_features:    specify if features should be extracted from lower res point renderings
        reweight_with_low_res  :    specify if the reweighting network gets the low resolution point rendering on top
        conv_block_extraction:      specify the convolution blocks for feature extraction: "gated" or "basic"
        conv_block_reweighting:     specify the convolution blocks for feature reweighting: "gated" or "basic"
        conv_block_reconstruction:  specify the convolution blocks for feature reconstruction: "gated" or "basic"
        upsample_mode:              specify which upsampling type is used: "deconv" or "bilinear"
        pooling_mode:               specify which pooling type is used: "avg" or "max"
        feature_scale:              scale factor to modify the filter sizes: 4 corresponds to 16,32,64,128,256
        feature_sizes:              specify filter_sizes of the reconstruction network: e.g. [64, 128, 256, 512, 1024] 
        fe_current_filter_base:     specify the filter base of the feature extraction network of current frames 
        fe_gt_filter_base:          specify the filter base of the feature extraction network of gt images 
        disable_reweighting:        specify if the reweighting should be disabled 
    """
    def __init__(
        self,
        in_channels_current      = [4,4,4,4],   # number of channels the current frame input contains.      Can be int or list of ints (for multi resolution input)
        in_channels_gt           = 4,           # number of channels the previous frames input contains.
        in_gt_frame_amount       = 3,           # number of ground truth frames feeded to the network
        feature_amount_current   = 8,           # number of additional features created by the feature extraction network for the current frame
        feature_amount_gt        = 12,          # number of additional features created by the feature extraction network for the gt frames per frame
        feature_extraction_depth = 0,           # DEPRECATED. warping and thus feature extraction on lower resolutions is not supported anymore
        out_channels             = 3,           # number of output channels
        output_warped            = False,       # specify if warped images should be written to disk
        mask_warped              = False,       # specify if warped image should be masked by multiplying with 3rd channel of the movec 
        warp_rgbd                = True,        # specify if rgbd of gt should be warped and appended to the reweighting network
        extract_lowres_features  = True,        # specify if features should be extracted from lower res point renderings
        reweight_with_low_res    = False,       # specify if the reweighting network gets the low resolution point rendering on top
        conv_block_extraction_pr = "gated",     # specify the convolution blocks for feature extraction point rendering: "gated" or "basic"
        conv_block_extraction_gt = "gated",     # specify the convolution blocks for feature extraction ground truth: "gated" or "basic"
        conv_block_reweighting   = "basic",     # specify the convolution blocks for feature reweighting: "gated" or "basic"
        conv_block_reconstruction= "gated",     # specify the convolution blocks for feature reconstruction: "gated" or "basic"
        upsample_mode            = "deconv",    # specify which upsampling type is used: "deconv" or "bilinear"
        pooling_mode             = "avg",       # specify which pooling type is used: "avg" or "max"

        feature_scale            = 4,           # scale factor to modify the filter sizes: 4 corresponds to 16,32,64,128,256. Only used if no custom filter size is given
        filter_sizes             = [],          # specify filter_sizes of the reconstruction network: e.g. [64, 128, 256, 512, 1024] (default)
                                                # note that the level where gt images are used are doubled. [] -> default
        fe_current_filter_base  = 32,           # specify the filter base of the feature extraction network of current frames 
        fe_gt_filter_base  = 16,                # specify the filter base of the feature extraction network of gt images 

        disable_reweighting      = False        # specify if the reweighting should be disabled 

        
        ):
        super().__init__()
        ################################################################
        # Init network
        ################################################################
        # set network variables
        #---------------------------------------------------------------
        # input variables
        self.in_gt_frame_amount = in_gt_frame_amount
        self.feature_extraction_depth = feature_extraction_depth
        self.feature_amount_gt = feature_amount_gt
        self.warp_rgbd = warp_rgbd
        self.extract_lowres_features = extract_lowres_features
        self.feature_scale = feature_scale
        self.output_warped = output_warped
        if output_warped:
            print("WARNING: output_warped is active! That is a debug feature. disable for serious training!!!")
        self.warp_counter = 0
        self.mask_warped = mask_warped
        #---------------------------------------------------------------
        
        ################################################################
        # parse in_channels
        #---------------------------------------------------------------
        # transform in_channels to an input list with 5 entries
        # if in_channels is int, transfrom to list with one entry
        if isinstance(in_channels_current, int):
            in_channels_current = [in_channels_current]
        # add more channels for lower resolutions
        if len(in_channels_current) < 5:
            in_channels_current += [0] * (5 - len(in_channels_current))
        self.in_channels_current = in_channels_current[:5]
        #---------------------------------------------------------------
        
        ################################################################
        # init feature extraction networks
        #---------------------------------------------------------------
        # use two different Feature Extraction Networks. one for the point renderings, one for the gt images
        self.feature_extraction_current_module      = INOVIS_Feature_Extraction(self.in_channels_current[0],self.in_channels_current[0]+feature_amount_current, conv_block_extraction_pr, fe_current_filter_base) #Network for the current Frame
        self.feature_extraction_previous_module     = INOVIS_Feature_Extraction2(in_channels_gt,feature_amount_gt,feature_extraction_depth, conv_block_extraction_gt, fe_gt_filter_base)               #Network for the previous Frames
        #---------------------------------------------------------------

        ################################################################
        # init feature reweighting network
        #---------------------------------------------------------------
        # use different feature reweighting, depending on if rgbd data is warped
        self.reweight_with_low_res = reweight_with_low_res
        reweight_current_channels = self.in_channels_current[self.feature_extraction_depth]
        # if the lowest resolution point rendering should be feeded to the reweighting, add the amount of channels for the config
        if reweight_with_low_res: 
            self.lowest_pr_index = -1
            for i in range(len(self.in_channels_current)-1, -1,-1):
                if self.in_channels_current[i] > 0:
                    self.lowest_pr_index = i
                    break
            reweight_current_channels += self.in_channels_current[self.lowest_pr_index]
        if warp_rgbd: # gets rgbd content and computes reweighting based on all rgbd inputs
            self.feature_reweighting_module             = INOVIS_Feature_Reweighting(reweight_current_channels, in_channels_gt, in_gt_frame_amount, conv_block_reweighting, disable_reweighting)
        else:         # directly use the features for weight calculation -> omit rgbd data for reweighting 
            self.feature_reweighting_module             = INOVIS_Feature_Reweighting2(reweight_current_channels, feature_amount_gt, in_gt_frame_amount, conv_block_reweighting, disable_reweighting)
        #---------------------------------------------------------------

        ################################################################
        # init reconstruction network
        #---------------------------------------------------------------
        # compute the input channels for the reconstruction
        reconstruction_input_channels = self.in_channels_current.copy()        
        # if lowres input should be feature extracted, add the needed features to the rgbd input
        if self.extract_lowres_features: 
            for i in range(len(reconstruction_input_channels)):
                if reconstruction_input_channels[i] > 0:
                    reconstruction_input_channels[i] = self.in_channels_current[i]+feature_amount_current
            # add the gt features to the corresponding level
            reconstruction_input_channels[self.feature_extraction_depth] = reconstruction_input_channels[self.feature_extraction_depth] + feature_amount_gt * in_gt_frame_amount
        # if lowres input should not be feature extracted, only add input features to highest level and gt features at corrsponding res
        else:
            reconstruction_input_channels[0] = self.in_channels_current[0]+feature_amount_current
            reconstruction_input_channels[self.feature_extraction_depth] = in_channels_current[self.feature_extraction_depth] + feature_amount_gt * in_gt_frame_amount
        print("Reconstruction Input Channels: ", reconstruction_input_channels)
        #---------------------------------------------------------------
        # additional feature channels for the level of gt images
        # do same feature size calculation as in reconstruction network
        if filter_sizes: # if custom filter size given 
            filters = filter_sizes  # set filter sizes
            self.feature_scale = 1  # set feature_scale to not affect filter sizes
            self.additional_channels = [0,0,0,0,0] # do not use the additional features
        else: # default filter sizes: can be manipulated with feature_scale. double the the filter size on level of gt images
            filters = [64, 128, 256, 512, 1024] # default filter
            filters = [x // self.feature_scale for x in filters] # divide by scale
            # double feature size at feature extraction depth level resolution to comprehend for additional features
            self.additional_channels = [0,0,0,0,0]
            self.additional_channels[self.feature_extraction_depth] = filters[self.feature_extraction_depth]
        #---------------------------------------------------------------
        # init reconstruction
        self.reconstruction_module                  = INOVIS_Reconstruction(num_input_channels=reconstruction_input_channels , num_output_channels=out_channels, feature_scale=self.feature_scale, filter_sizes=filters, additional_channels=self.additional_channels, upsample_mode=upsample_mode, conv_block = conv_block_reconstruction, pooling_mode=pooling_mode)
        #---------------------------------------------------------------
        
    def forward(self,
        input_current,          # input tensor list of the current frame.  
        input_previous,         # input tensors of the auxiliary frames.
        input_movecs            # input tensors of the motion vectors.
        ):
        
        ################################################################
        # Feature Extraction
        #---------------------------------------------------------------
        # Extract Features of Highest Resolution Point Rendering 
        features_current = self.feature_extraction_current_module(input_current[0])
        #---------------------------------------------------------------
        # Extract Featues of Nearest Groundtruth Frames
        # (Optional) Concat RGBD Data if should be warped
        features_previous = [None]*self.in_gt_frame_amount
        if self.warp_rgbd: # if rgbd should be warped as well, scale down and concat rgbd upfront
            for i in range(self.in_gt_frame_amount):
                scale = 1.0 / (2**self.feature_extraction_depth)
                rgbd = F.interpolate(input_previous[i], scale_factor=scale, mode='bilinear')
                features_previous[i]=torch.cat((rgbd, self.feature_extraction_previous_module(input_previous[i])),1)
        else:   # if rgbd should not be warped, do standard stuff
            for i in range(self.in_gt_frame_amount):
                features_previous[i]=self.feature_extraction_previous_module(input_previous[i])
        #---------------------------------------------------------------
        
        ################################################################
        # Backward Warping
        #---------------------------------------------------------------
        # Warp previous frame into the current view.
        warped_previous = [None]*self.in_gt_frame_amount
        for i in range(self.in_gt_frame_amount):
            warped_previous[i] = warp_back_tc(features_previous[i],input_movecs[i][:,0:2,:,:])
        #---------------------------------------------------------------
        # (Optional) Mask warped data with the third component of movecs
        if self.mask_warped:
            for i in range(len(warped_previous)):
                warped_previous[i] = torch.mul(warped_previous[i], input_movecs[i][:,2:3,:,:])
        #---------------------------------------------------------------
        # Uncomment to create a network that outputs the first warped image
        # return warped_previous[0][:,0:3,:,:]#F.interpolate(warped_previous[0][:,0:3,:,:], scale_factor=2.0, mode='bilinear')
        #---------------------------------------------------------------

        ################################################################
        # Reweighting
        #---------------------------------------------------------------
        # (Optional) configure the reweigting network if images should be saved
        if self.output_warped:
            self.feature_reweighting_module.output_map = True
            self.feature_reweighting_module.warp_counter = self.warp_counter
        #---------------------------------------------------------------
        # Reweight features
        reweight_curr_in = input_current[self.feature_extraction_depth]
        # concat lowres point rendering if wanted
        if self.reweight_with_low_res:
            low_pr_up = F.interpolate(input_current[self.lowest_pr_index], scale_factor=2**self.lowest_pr_index, mode='bilinear') 
            reweight_curr_in = torch.cat((reweight_curr_in, low_pr_up),1)
        reweighted_features = self.feature_reweighting_module(reweight_curr_in,*warped_previous)
        #---------------------------------------------------------------
        # (Optional) Save results of warping 
        if(self.output_warped):
            self.feature_reweighting_module.output_map = False
            for image in range(features_current.shape[0]):
                downsampled_current = input_current[self.feature_extraction_depth][image,0:3,:,:]#F.interpolate(features_current, scale_factor=1/(2**self.feature_extraction_depth), mode='bilinear')
                write_tensor_as_image(downsampled_current, "./output_warped/"  + str(features_current.shape[2]) + "_" + str(self.warp_counter)  + "_" + str(image) + "_0pr.png", input_range=[0,1])
                write_tensor_as_image(features_previous[0][image,0:3,:,:], "./output_warped/"  + str(features_current.shape[2]) + "_" + str(self.warp_counter)  + "_" + str(image) + "_1nearest.png", input_range=[0,1])
                i=0
                write_tensor_as_image(warped_previous[i][image,0:3,:,:], "./output_warped/" + str(features_current.shape[2]) + "_" + str(self.warp_counter) + "_" + str(image) + "_" + "2warped.png", input_range=[0,1])
                write_tensor_as_image(reweighted_features[image,0:3,:,:] * 0.1, "./output_warped/" + str(features_current.shape[2]) + "_" + str(self.warp_counter) + "_" + str(image) + "_" + "3reweighted.png", input_range=[0,1])
            self.warp_counter += 1
        #---------------------------------------------------------------
        # (Optional) Save results of warping 
        # discard RGBD of the warped data
        if self.warp_rgbd: # if rgbd was warped as well, delete it after reweighing from the features
            new_reweighted_features = reweighted_features[:,4:4+self.feature_amount_gt,:,:]
            for i in range(1,self.in_gt_frame_amount):
                base_index = i*(4+self.feature_amount_gt)
                new_reweighted_features = torch.cat((new_reweighted_features, reweighted_features[:, base_index + 4 : base_index + 4 + self.feature_amount_gt ,:,:]),1)
            reweighted_features = new_reweighted_features
        #---------------------------------------------------------------

        ################################################################
        # Reconstruction
        #---------------------------------------------------------------
        # Reconstruct the final image 
        reconstr_input = [None]*len(input_current)
        reconstr_input[0] = features_current
        #---------------------------------------------------------------
        # (Optional) Extract Features from lower resolution point renderings and use them as input for reconstruction
        # alternatively use point rendering as input directly
        if self.extract_lowres_features: # if lowres input should be feature extracted, extract here
            for i in range(1,len(input_current)): # for each lower resolution, copy input of the curtrent frame
                reconstr_input[i] = self.feature_extraction_current_module(input_current[i])
        else:
            for i in range(1,len(input_current)):
                reconstr_input[i] = input_current[i] #self.feature_extraction_current_module(input_current[i])
        #---------------------------------------------------------------
        # Concatenate point rendered features and gt features and set the corresponding level of input
        concatenated_features = reconstr_input[self.feature_extraction_depth]#input_current[self.feature_extraction_depth]
        concatenated_features = torch.cat((concatenated_features,reweighted_features),1)
        reconstr_input[self.feature_extraction_depth] = concatenated_features
        #---------------------------------------------------------------
        # Reconstruct with the UNet
        reconstr = self.reconstruction_module(*reconstr_input)
        #---------------------------------------------------------------
        return reconstr
        #---------------------------------------------------------------

#######################################################################################################
# Unet from Neural Point-Based Graphics by Aliev et al.
# Implementation from https://github.com/alievk/npbg/blob/master/npbg/models/unet.py

from sitof.external.conv import PartialConv2d

_assert_if_size_mismatch = True

class Identity(nn.Module):
    def __init__(self, *args, **kwargs):
        super(Identity, self).__init__()

    def forward(self, x):
        return x

class ListModule(nn.Module):
    def __init__(self, *args):
        super(ListModule, self).__init__()
        idx = 0
        for module in args:
            self.add_module(str(idx), module)
            idx += 1

    def __getitem__(self, idx):
        if idx >= len(self._modules):
            raise IndexError('index {} is out of range'.format(idx))
        if idx < 0: 
            idx = len(self) + idx

        it = iter(self._modules.values())
        for i in range(idx):
            next(it)
        return next(it)

    def __iter__(self):
        return iter(self._modules.values())

    def __len__(self):
        return len(self._modules)

class BasicBlock(nn.Module):
    def __init__(self, in_channels, out_channels, kernel_size=3, normalization=nn.BatchNorm2d):
        super().__init__()

        self.conv1 = nn.Sequential(nn.Conv2d(in_channels, out_channels, kernel_size, padding=1),
                                    normalization(out_channels),
                                    nn.ReLU())

        self.conv2 = nn.Sequential(nn.Conv2d(out_channels, out_channels, kernel_size, padding=1),
                                    normalization(out_channels),
                                    nn.ReLU())

    def forward(self, inputs, **kwargs):
        outputs = self.conv1(inputs)
        outputs = self.conv2(outputs)
        return outputs

class PartialBlock(nn.Module):
    def __init__(self, in_channels, out_channels, kernel_size=3, normalization=nn.BatchNorm2d):
        super().__init__()

        self.conv1 = PartialConv2d(
            in_channels, out_channels, kernel_size, padding=1)

        self.conv2 = nn.Sequential(
            normalization(out_channels),
            nn.ReLU(),   
            nn.Conv2d(out_channels, out_channels, kernel_size, padding=1),
            normalization(out_channels),
            nn.ReLU()
        )
                                       
    def forward(self, inputs, mask=None):
        outputs = self.conv1(inputs, mask)
        outputs = self.conv2(outputs)
        return outputs

class GatedBlock(nn.Module):
    def __init__(self, in_channels, out_channels, kernel_size=3, stride=1, dilation=1, padding_mode='reflect', act_fun=nn.ELU, normalization=nn.BatchNorm2d):
        super().__init__()
        self.pad_mode = padding_mode
        self.filter_size = kernel_size
        self.stride = stride
        self.dilation = dilation

        n_pad_pxl = int(self.dilation * (self.filter_size - 1) / 2)

        # this is for backward campatibility with older model checkpoints
        self.block = nn.ModuleDict(
            {
                'conv_f': nn.Conv2d(in_channels, out_channels, kernel_size, stride=stride, dilation=dilation, padding=n_pad_pxl),
                'act_f': act_fun(),
                'conv_m': nn.Conv2d(in_channels, out_channels, kernel_size, stride=stride, dilation=dilation, padding=n_pad_pxl),
                'act_m': nn.Sigmoid(),
                'norm': normalization(out_channels)
            }
        )

    def forward(self, x, *args, **kwargs):
        features = self.block.act_f(self.block.conv_f(x))
        mask = self.block.act_m(self.block.conv_m(x))
        output = features * mask
        output = self.block.norm(output)

        return output

class DownsampleBlock(nn.Module):
    def __init__(self, in_channels, out_channels, conv_block=BasicBlock, pooling_mode = "avg"):
        super().__init__()

        self.conv = conv_block(in_channels, out_channels)
        if pooling_mode == "avg":
            self.down = nn.AvgPool2d(2, 2)
        elif pooling_mode == "max":
            self.down = nn.MaxPool2d(2, 2)

    def forward(self, inputs, mask=None):
        outputs = self.down(inputs)
        outputs = self.conv(outputs, mask=mask)
        return outputs

class UpsampleBlock(nn.Module):
    def __init__(self, out_channels, upsample_mode, same_num_filt=False, conv_block=BasicBlock):
        super().__init__()

        num_filt = out_channels if same_num_filt else out_channels * 2
        if upsample_mode == 'deconv':
            self.up = nn.ConvTranspose2d(num_filt, out_channels, 4, stride=2, padding=1)
            self.conv = conv_block(out_channels * 2, out_channels, normalization=Identity)
        elif upsample_mode=='bilinear' or upsample_mode=='nearest':
            self.up = nn.Sequential(nn.Upsample(scale_factor=2, mode=upsample_mode),
                                    # Before refactoring, it was a nn.Sequential with only one module.
                                    # Need this for backward compatibility with model checkpoints.
                                    nn.Sequential(
                                        nn.Conv2d(num_filt, out_channels, 3, padding=1)
                                        )
                                    )
            self.conv = conv_block(out_channels * 2, out_channels, normalization=Identity)
        else:
            assert False

    def forward(self, inputs1, inputs2):
        in1_up = self.up(inputs1)

        if (inputs2.size(2) != in1_up.size(2)) or (inputs2.size(3) != in1_up.size(3)):
            if _assert_if_size_mismatch:
                raise ValueError(f'input2 size ({inputs2.shape[2:]}) does not match upscaled inputs1 size ({in1_up.shape[2:]}')
            diff2 = (inputs2.size(2) - in1_up.size(2)) // 2
            diff3 = (inputs2.size(3) - in1_up.size(3)) // 2
            inputs2_ = inputs2[:, :, diff2 : diff2 + in1_up.size(2), diff3 : diff3 + in1_up.size(3)]
        else:
            inputs2_ = inputs2

        output= self.conv(torch.cat([in1_up, inputs2_], 1))

        return output

class UpsampleBlock2(nn.Module):
    def __init__(self, in_channels, in_channels_lower, out_channels, upsample_mode, conv_block=BasicBlock):
        super().__init__()

        
        if upsample_mode == 'deconv':
            self.up = nn.ConvTranspose2d(in_channels_lower, out_channels, 4, stride=2, padding=1)
            self.conv = conv_block(out_channels + in_channels, out_channels, normalization=Identity)
        elif upsample_mode=='bilinear' or upsample_mode=='nearest':
            self.up = nn.Sequential(nn.Upsample(scale_factor=2, mode=upsample_mode),
                                    # Before refactoring, it was a nn.Sequential with only one module.
                                    # Need this for backward compatibility with model checkpoints.
                                    nn.Sequential(
                                        nn.Conv2d(in_channels_lower, out_channels, 3, padding=1)
                                        )
                                    )
            self.conv = conv_block(out_channels + in_channels, out_channels, normalization=Identity)
        else:
            assert False

    def forward(self, inputs1, inputs2):
        in1_up = self.up(inputs1)

        if (inputs2.size(2) != in1_up.size(2)) or (inputs2.size(3) != in1_up.size(3)):
            if _assert_if_size_mismatch:
                raise ValueError(f'input2 size ({inputs2.shape[2:]}) does not match upscaled inputs1 size ({in1_up.shape[2:]}')
            diff2 = (inputs2.size(2) - in1_up.size(2)) // 2
            diff3 = (inputs2.size(3) - in1_up.size(3)) // 2
            inputs2_ = inputs2[:, :, diff2 : diff2 + in1_up.size(2), diff3 : diff3 + in1_up.size(3)]
        else:
            inputs2_ = inputs2

        output= self.conv(torch.cat([in1_up, inputs2_], 1))

        return output

###############################################################################
# Helper Functions
###############################################################################
def get_current_learning_rate(optimizer):
            # get current learing rate
        cur_lr = []
        for param_group in optimizer.param_groups:
            cur_lr.append(param_group['lr'])
        return cur_lr

# from https://github.com/junyanz/pytorch-CycleGAN-and-pix2pix/blob/f13aab8148bd5f15b9eb47b690496df8dadbab0c/models/networks.py#L119
def init_weights(net, init_type='normal', init_gain=0.02):
    """Initialize network weights.
    Parameters:
        net (network)   -- network to be initialized
        init_type (str) -- the name of an initialization method: normal | xavier | kaiming | orthogonal
        init_gain (float)    -- scaling factor for normal, xavier and orthogonal.
    We use 'normal' in the original pix2pix and CycleGAN paper. But xavier and kaiming might
    work better for some applications. Feel free to try yourself.
    """
    def init_func(m):  # define the initialization function
        classname = m.__class__.__name__
        if hasattr(m, 'weight') and (classname.find('Conv') != -1 or classname.find('Linear') != -1):
            if init_type == 'normal':
                init.normal_(m.weight.data, 0.0, init_gain)
            elif init_type == 'xavier':
                init.xavier_normal_(m.weight.data, gain=init_gain)
            elif init_type == 'kaiming':
                init.kaiming_normal_(m.weight.data, a=0, mode='fan_in')
            elif init_type == 'orthogonal':
                init.orthogonal_(m.weight.data, gain=init_gain)
            else:
                raise NotImplementedError('initialization method [%s] is not implemented' % init_type)
            if hasattr(m, 'bias') and m.bias is not None:
                init.constant_(m.bias.data, 0.0)
        elif classname.find('BatchNorm2d') != -1:  # BatchNorm Layer's weight is not a matrix; only normal distribution applies.
            init.normal_(m.weight.data, 1.0, init_gain)
            init.constant_(m.bias.data, 0.0)

    print('initialize network with %s' % init_type)
    net.apply(init_func)  # apply the initialization function <init_func>
