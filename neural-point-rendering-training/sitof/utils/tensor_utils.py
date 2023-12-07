from PIL import Image
import numpy as np
import shutil
import os
import datetime

import torch
import torch.nn as nn
from torch import optim
from torchvision import transforms

def create_output_dir(file):
    try:
        shutil.rmtree(file)
    except:
        pass
    os.mkdir(file)


def get_memory_size_of_tensor(t):
   return  (t.nelement()*t.element_size())/(1024*1024)


## https://discuss.pytorch.org/t/gpu-memory-that-model-uses/56822/2
def get_non_backwards_size_of_model(model):
    mem_params = sum([param.nelement()*param.element_size() for param in model.parameters()])
    mem_bufs = sum([buf.nelement()*buf.element_size() for buf in model.buffers()])
    mem = mem_params + mem_bufs # in bytes
    return mem/(1024*1024)

def get_threadlocal_seed():
    # only torch random number generator is initialized differently for each worker
    # (see https://pytorch.org/docs/stable/data.html , "Randomness in multi-process data loading"):
    # torch.utils.data.get_worker_info().seed is guarantied to be randomish for each worker and epoch
    # and torch.rand is initialized with this seed
    return int(torch.randint(0,838349873,(1,))[0])

def get_threadlocal_random_number(lower_bound=0, upper_bound=838349873):
    return int(torch.randint(lower_bound,upper_bound,(1,))[0])



def print_tensor_info(name, tensor):
    if tensor.dtype == torch.float:
        print(name+': dims: ', tensor.size(),', min, max:', tensor.min(), tensor.max(), 'var, mean', torch.var_mean(tensor), 'any nans? ', torch.isnan(tensor).any())  
    else:
        print(name+': dims: ', tensor.size(),', min, max:', tensor.min(), tensor.max(), 'any nans? ', torch.isnan(tensor).any())  

def unnormalize(tensor):
    t1 = transforms.Normalize(mean=[-0.485, -0.456, -0.406],std=[1,1,1])
    t2 = transforms.Normalize(mean=[0,0,0], std=[1.0/0.229, 1.0/0.224, 1.0/0.225])

    if len(tensor.size()) == 4:
        for i in range(0,tensor.size()[0]):
            tensor[i] = t2(tensor[i])
            tensor[i] = t1(tensor[i])
    else:
        tensor = t2(tensor)
        tensor = t1(tensor)
    return tensor


    
def write_map_of_tensors_side_by_side(tensor_map, path, prefix=datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S"), postfix="", split_four_channel_images_into_two=True, fixed_filename=""):
    with torch.no_grad():
        images_written = 0
        #loop over batch
        for b in range(next(iter(tensor_map.values())).shape[0]):
            concatenated_tensors = torch.tensor(()).cpu()
            concatenated_tensors_names = ""

            #loop over features
            for k,v in tensor_map.items():
                feat = ((v[b].clone().detach()).cpu()).float()
                
                tensor_channels = v.shape[1]
                # change multi-channel tensors for displaying
                if tensor_channels != 3:
                    # one channel -> pad to greyscale
                    if tensor_channels == 1:
                        feat = torch.cat([feat,feat,feat],dim=0)
                    #two channels: blue channel is zero
                    if tensor_channels == 2:
                        feat = torch.cat([feat,torch.zeros(1,feat[0].shape[0],feat[0].shape[1])],dim=0)
                    #four channels: split to two images if flag recommends it
                    if v.shape[1] == 4:
                        #make two images: first with only alpha, second with rgb
                        if split_four_channel_images_into_two:
                            x =  feat[3].unsqueeze(0)
                            feat = torch.cat([torch.cat([x,x,x],dim=0), feat[:3]],dim=2)
                            if (len(concatenated_tensors_names)+ len(k) + 3 + len(prefix) + len(postfix)+ len(path) + 6) < 200: ##255 is the max file length for most filesystems, keep under that
                                concatenated_tensors_names += k + "_a-"
                        else:
                            feat = feat[:3]
                if len(concatenated_tensors) == 0:
                    concatenated_tensors = feat
                else:
                    concatenated_tensors = torch.cat([concatenated_tensors, feat], dim=2)

                if (len(concatenated_tensors_names)+ len(k) + 1 + len(prefix) + len(postfix)+ len(path) + 6) < 200: ##255 is the max file length for most filesystems, keep under that
                    concatenated_tensors_names += k + "-"

            #don't use map names as input if desired
            if fixed_filename != "":
                concatenated_tensors_names = fixed_filename

            write_tensor_as_image(concatenated_tensors, path+prefix+'_'+concatenated_tensors_names+'_bi'+str(b)+'_' + postfix, normalize=False)
            images_written += 1

        return images_written


def tensor_to_image(in_tensor, normalize, input_range = [-1,1]):
    with torch.no_grad():
        tensor = torch.tensor([])
        if in_tensor.device == torch.device('cpu'):
            tensor = in_tensor.clone()
        elif in_tensor.device.type == torch.device('cuda:0').type:
            tensor = in_tensor.cpu()
        else:
            print('unknown device ',in_tensor.device.type)
            exit(-1)
        if len(tensor.size()) == 4:
            tensor = tensor.squeeze(dim=0)
        while len(tensor.size()) < 3:
            tensor.unsqueeze_(dim=0)

        if input_range[0] != 0 or input_range[1] != 1:
            tensor -= input_range[0]
            tensor = tensor / (input_range[1]-input_range[0])

        if normalize:
            tensor = unnormalize(tensor)

        tensor = tensor.clamp(0,1)

        return transforms.ToPILImage()(tensor)


def write_tensor_as_image(tensor, file, input_range = [-1,1], normalize=False):
    img = tensor_to_image(tensor, input_range=input_range, normalize=normalize)
    img.save(file)

def write_np_array_as_image(arr_, file, input_range = [-1,1]):
    arr = np.copy(arr_)
    if input_range[0] != 0 or input_range[1] != 1:
        arr -= input_range[0]
        arr = arr / (input_range[1]-input_range[0])

    arr = np.clip(arr, 0, 1)
    arr_uint = (255*arr).astype(dtype=np.uint8)
    
    Image.fromarray(arr_uint).save(file)

def write_input_output_images(input, output, base_dir, fn_suffix):

    num_batches = input[next(iter(input))].shape[0]
    input_shape_single = input[next(iter(input))].shape
    #print("input tensor shape: " + str(input_shape_single) + "\n") 
    input_vertical_resolution = input_shape_single[2]
    #print("vert res was ", input_vertical_resolution)
    for k,v in input.items():
        feat = v[0]
        #print(feat.shape)
        if feat.shape[1] > input_vertical_resolution:
            input_vertical_resolution = feat.shape[1]
    #print("vert res is ", input_vertical_resolution)

    images_written = 0
    for b in range(num_batches):

        concatenated_tensors = torch.tensor(())
        concatenated_tensors_names = ""
        
        for k,v in input.items():

            feat = v[b]
            # pad one-channel only images to 3 channels
            if v.shape[1] == 1:
                feat = torch.cat([v[b],v[b],v[b]],dim=0)
            if v.shape[1] == 2:
                # add artificial blue channel if only 2 channels available
                feat = torch.cat([feat.cpu(), torch.ones(1, feat.shape[1], feat.shape[2])],dim=0)
            if v.shape[1] == 4:
                #make two images: first rgb, second alpha
                x =  feat[3].unsqueeze(0)
                feat = torch.cat([feat[:3], torch.cat([x,x,x],dim=0)],dim=2)
                concatenated_tensors_names += k + "+a-"
                #concatenated_tensors_names += k + "+alpha-"

            feattens = feat.cpu()
            if(v.shape[2] != input_vertical_resolution): 
                feattens = torch.cat([feattens, torch.ones(3, input_vertical_resolution-v.shape[2], v.shape[3])], dim=1)
            if len(concatenated_tensors) == 0:
                concatenated_tensors = feattens
            else:
                concatenated_tensors = torch.cat([concatenated_tensors, feattens], dim=2)
            #concatenated_tensors_names += k + "-"
            if(len(k) > 4):
                concatenated_tensors_names += k[0:4] + "-"
            else:
                concatenated_tensors_names += k + "-"


        if len(concatenated_tensors):
            concatenated_tensors = torch.cat([concatenated_tensors, torch.ones(3, concatenated_tensors.shape[1], 20)], dim=2)

        for k,v in output.items():
            feat = v[b]
            # pad one-channel only images to 3 channels
            if v.shape[1] == 1:
                feat = torch.cat([v[b],v[b],v[b]],dim=0)
            if v.shape[1] == 2:
                # add artificial blue channel if only 2 channels available
                feat = torch.cat([feat.cpu(), torch.ones(1, feat.shape[1], feat.shape[2])],dim=0)
            if v.shape[1] == 4:
                #make two images: first rgb, second alpha
                x =  feat[3].unsqueeze(0)
                feat = torch.cat([feat[:3], torch.cat([x,x,x],dim=0)],dim=2)
                concatenated_tensors_names += k + "+a-"
                #concatenated_tensors_names += k + "+alpha-"
            feattens = feat.cpu()
            if(v.shape[2] != input_vertical_resolution): 
                feattens = torch.cat([feattens, torch.ones(3, input_vertical_resolution-v.shape[2], v.shape[3])], dim=1)
            if len(concatenated_tensors) == 0:
                concatenated_tensors = feattens
            else:
                concatenated_tensors = torch.cat([concatenated_tensors, feattens], dim=2)
            #concatenated_tensors_names += k + "-"
            if(len(k) > 4):
                concatenated_tensors_names += k[0:4] + "-"
            else:
                concatenated_tensors_names += k + "-"


        write_tensor_as_image(concatenated_tensors, base_dir+concatenated_tensors_names+str(b)+fn_suffix, normalize=False, input_range = [0,1])
        images_written += 1

    return images_written

