import logging
import datetime
import os
from os.path import splitext
from os.path import isfile
from os import listdir
from glob import glob
import io
import numpy as np
import matplotlib.pyplot as plt
import random
import h5py
# Conda's libtiff package is the C library, not the Python library. 
# The easiest way to get it would be to use conda to install the dependencies (like libtiff, numpy), 
# and then use pip to install libtiff (pip install libtiff).
from libtiff import TIFF
from PIL import Image
import hashlib
import atexit
from multiprocessing.pool import ThreadPool
import torch
from torch.utils.data import Dataset
import imgaug as ia
import imgaug.augmenters as iaa
from tqdm import tqdm

from .read_image_from_binary_blob import read_blob
from .tensor_utils import *
from .data_transforms_monitor import *

###########################################################
# various helpers

def read_image(base_dir, file_stem, file_extension, channels = 3):
    ''' Returns a numpy array with: 
            dtype = np.float32, 
            value range = [0,1]
    '''
    file_path = base_dir+file_stem+file_extension
    if '.tif' in file_extension.lower():
        return TIFF.open(file_path).read_image().astype(dtype=np.float32)[:,:,:3]/np.iinfo(np.uint16).max
    elif file_extension.lower() == '.jpg' or file_extension.lower() == '.png' :
        return np.array(Image.open(file_path))[:,:,:channels].astype(dtype=np.float32)/255.0
    elif file_extension.lower() == '.bin':
        return read_blob(file_path)[:,:,:channels]
    else:
        print("WARNING:", file_extension, "is an untested file format! (Using PIL...)")
        return np.array(Image.open(file_path))[:,:,:channels]

def process_filelists(file_dir, max_images):
    file_list = listdir(file_dir)
    assert len(file_list) > 0, f"{file_dir} is empty!"
    file_extension = splitext(listdir(file_dir)[0])[1]
    
    filenames = [splitext(file)[0] for file in listdir(file_dir)
            if not file.startswith('.')]
    filenames.sort()
    total_files_in_dir = len(filenames)

    # discard images that exceed limit
    if max_images > 0 and max_images < len(filenames):
        filenames = filenames[:max_images]

    return filenames, file_extension, total_files_in_dir

def split_filename_list_in_train_val_testset(filename_list, seed=42, val_n_test_ratio = 0.1, verbose = True, test_indices = []):
    if verbose: print(filename_list)
    filename_list.sort()
    # calc length of sets
    num_files = len(filename_list)
    num_val_files = int(val_n_test_ratio*num_files)+1
    if len(test_indices) == 0: num_test_files = num_val_files
    else: 
        num_test_files = len(test_indices)
    num_train_files = num_files - (num_val_files+num_test_files)
    if verbose: print("num [train,test,val]: [", num_train_files, ", ", num_test_files, ", ", num_val_files, "]")

    if len(test_indices) == 0:
        # shuffle and split
        random.seed(seed)
        random.shuffle(filename_list)
        val_filenames_lists = filename_list[:num_val_files] 
        test_filenames_lists = filename_list[num_val_files: num_val_files+ num_test_files] 
        train_filenames_lists = filename_list[num_val_files+ num_test_files:] 
        assert len(train_filenames_lists) == num_train_files, train_filenames_lists
    else : # specific test indices were passed
        test_filenames_lists = []
        for i in test_indices:
            if i >= len(filename_list):
                continue
            test_filenames_lists.append(filename_list[i])
        new_filename_list = list(set(filename_list)-set(test_filenames_lists))
        new_filename_list.sort()
        random.seed(seed)
        random.shuffle(new_filename_list)
        val_filenames_lists = new_filename_list[:num_val_files] 
        train_filenames_lists = new_filename_list[num_val_files:] 
        #assert len(train_filenames_lists) == num_train_files, train_filenames_lists

    # sort
    train_filenames_lists.sort()
    val_filenames_lists.sort()
    test_filenames_lists.sort()
    
    if verbose:
        print("num [train,test,val]: [", len(train_filenames_lists), ", ", len(test_filenames_lists), ", ", len(val_filenames_lists), "]")
        print('========================================= \n', 'train')
        print(train_filenames_lists)
        print('========================================= \n', 'val')
        print(val_filenames_lists)
        print('========================================= \n', 'test')
        print(test_filenames_lists)


    return {'train': train_filenames_lists, 
            'val' :  val_filenames_lists,
            'test' : test_filenames_lists }


class InputFeatureStructTemplate:

    def __init__(self, featurename, directory, init_transform, train_onload_transform, val_onload_transform, test_onload_transform, max_images):
        self.featurename = featurename
        self.directory = directory
        self.size_per_directory = []
        self.init_transform = init_transform
        self.max_images = max_images

        self.train_onload_transform = train_onload_transform
        self.val_onload_transform = val_onload_transform
        self.test_onload_transform = test_onload_transform
        self.onload_transform = None

        ## init in read_files
        self.filename_list = None
        self.extension = None
        self.total_files_in_directory = None
        if self.filename_list == None:
            self.read_filenames()

    def read_filenames(self):
        # print("read files in ", self.directory )
        if isinstance(self.directory, str): # single directory
            self.filename_list, self.extension, self.total_files_in_directory = process_filelists(self.directory, self.max_images)
            self.size_per_directory.append(self.total_files_in_directory)
        else: # multiple directories in a list
            
            self.total_files_in_directory = 0
            print(self.directory)
            for single_dir in self.directory:
                splitted_dir = single_dir.split('/')
                base_dir = splitted_dir[-3] + '/' + splitted_dir[-2]
                # print(splitted_dir)
                # print(base_dir)
                self.total_files_in_directory
                single_filename_list, self.extension, single_total_files_in_directory = process_filelists(single_dir, self.max_images)
                print(single_dir)
                new_filename_list = []
                for name in single_filename_list:
                    new_filename_list += [base_dir + '/' + name]
                if self.filename_list == None:
                    self.filename_list = new_filename_list
                else:
                    self.filename_list += new_filename_list
                self.total_files_in_directory += single_total_files_in_directory
                # print(self.total_files_in_directory)
                self.size_per_directory.append(single_total_files_in_directory)
            
            new_directory = ""
            for e in self.directory[0].split('/')[:-3]:
                new_directory += e + '/'
            self.directory = new_directory
            # print(self.directory)

        

    def __iter__(self):        
        assert len(self.filename_list) > 0, 'of '+ self.featurename + ' ' + self.directory
        return iter(self.filename_list)


    def get_path_list(self):
        return [self.directory+f+self.extension for f in self.filename_list]

    def init_split(self, seed=42, val_n_test_ratio = 0.1, verbose=True, test_indices = []):
        ''' splitting depends on the seed and val_n_test_ratio. If you want to do k-fold cross validation provide a new seed for every training viea the config file or so
        ''' 
        if verbose: print('========================================= \n split files of', self.featurename)

        # sort... just to be sure
        self.filename_list.sort()
        # copy self
        train_part = self.copy()
        train_part.set_type('train')
        val_part = self.copy()
        val_part.set_type('val')
        test_part = self.copy()
        test_part.set_type('test')

        #construct test_indices for all datasets -> test_indicesa re to be left out from every dataset.
        test_indices_new = []
        base_i = 0
        print ("set sizes: ", self.size_per_directory)
        print(self.size_per_directory)
        for dir_i in range(len(self.size_per_directory)):
            print(dir_i)
            for i in test_indices:
                if i < self.size_per_directory[dir_i]:
                    test_indices_new.append(base_i + i) 
            base_i += self.size_per_directory[dir_i]

        print ("old test_indices: ", test_indices)
        print ("constructed test_indices: ", test_indices_new)
        #split list
        split_lists = split_filename_list_in_train_val_testset(self.filename_list, verbose=verbose, test_indices = test_indices_new)

        # unpack from dict
        train_part.filename_list = split_lists['train']
        val_part.filename_list = split_lists['val']
        test_part.filename_list = split_lists['test']

        return (train_part, val_part, test_part)

    def init(self, dataset_type):
        '''you need to call either init or init split'''
        self.set_type(dataset_type)

    @staticmethod
    def check_filename_correspondence(input_structs):
        lists = [struct.filename_list for _, struct in input_structs.items()]
        #print(lists)
        #for i in lists:
        #    print(len(i))
        len0 = len(lists[0])
        same = True
        for l in lists:
            same = same and len(l) == len0
            assert same, len0 + ' and '+ len(l)+ ' are not equal!'

        for t in zip(lists):
            same = True
            for name in t:
                same = same and t[0] == name
            assert same, print(t, 'are not equal!')

    def copy(self):
        new = InputFeatureStructTemplate(self.featurename, self.directory, self.init_transform, self.train_onload_transform, self.val_onload_transform, self.test_onload_transform, self.max_images)
        new.extension, new.total_files_in_directory = self.extension, self.total_files_in_directory
        return new

    def set_type(self, dataset_type):
        if dataset_type == 'train':
            self.onload_transform = self.train_onload_transform
        elif dataset_type == 'val':
            self.onload_transform = self.val_onload_transform
        elif dataset_type == 'test':
            self.onload_transform = self.test_onload_transform
        elif dataset_type == 'render':
            self.onload_transform = self.val_onload_transform
        else:
            print("Invalid dataset type!")
            exit(0)

# -----------------------------------------------------------
# build memory mapped data set (hdf5)

pool = ThreadPool(os.cpu_count())
atexit.register(pool.join)
atexit.register(pool.close)


# We need to close and reopen the hdf5 dataset file, as we can't be sure that all dataloader workers inherit the same file pointer! --> race conditions!!
# see e.g. https://stackoverflow.com/questions/46045512/h5py-hdf5-database-randomly-returning-nans-and-near-very-small-data-with-multi/52438133#52438133
def make_multifeature_dataset(output_dir, input_structs):
    '''
        Creates hdf5 dataset that is regenerated depending on filelist and the use preprocessing augmentation function!
        WARNING: Dataset generation will break if the output dimensions of augmentation_func or the all the image theirself are not equally sized!! 
    '''
    path_lists = [(input_structs[feature].init_transform, input_structs[feature].get_path_list()) for feature in input_structs]

    transforms_code = []
    shape = (0,0,0,0)
    last_shape = None
    path_lists = ()
    feature = ''
    directories = [] 
    c = 0
    lo_hi_map = {}
    for feature in input_structs:
        # load single image to determine shape
        tmp = input_structs[feature].init_transform(read_image(input_structs[feature].directory, input_structs[feature].filename_list[0], input_structs[feature].extension))
        #if only one color channel exists, add an empty channel dimension 
        while len(tmp.shape)< 3:
            tmp = np.expand_dims(tmp,len(tmp.shape))
        
        # remember where we put the channels in the dataset
        lo = c
        hi = c+tmp.shape[2]
        #print(tmp.shape)
        lo_hi_map[feature] = (lo, hi)
        c = hi

        shape = (len(input_structs[feature].filename_list), tmp.shape[-3], tmp.shape[-2], shape[3]+tmp.shape[-1])

        assert (last_shape == None or last_shape == shape), f"Transform of {feature} has mismatching dimensions to prev transform! Please check!" 

        # get source code of transforms
        transforms_code.append(input_structs[feature].init_transform.get_code_of_transform())
        # get all  the filenames
        path_lists = path_lists+ ([(input_structs[feature].init_transform, input_structs[feature].directory, file_stem, input_structs[feature].extension) for file_stem in input_structs[feature]],)
        # and directories
        directories.append(input_structs[feature].directory)

    #print('hdf5 shape is', shape)

    # determine some unique input combination thingy
    filenames = str(input_structs[feature].filename_list)
    dataset_filename = hashlib.md5(f'{directories}{filenames}{transforms_code}{shape}'.encode()).hexdigest()
    dataset_filename = f"{output_dir}/{dataset_filename}.h5"

    # zip
    path_lists = [tupl for tupl in zip(*path_lists)]
    if not isfile(dataset_filename):
        print(f'Building dataset {[i_struct for i_struct in input_structs]} \n -> {dataset_filename} (this may take a while)...')
        with h5py.File(dataset_filename, 'w') as f:
            dset = f.create_dataset('data', shape=shape, dtype=tmp.dtype, chunks=(1, shape[1], shape[2],shape[3]))
            tq = tqdm(total=len(path_lists))
            def helper(dset, idx, path_tuple):
                img_list = []
                for transform, base_dir, file_path, ext in path_tuple:
                    f = read_image(base_dir, file_path, ext)
                    #if only one color channel exists, add an empty channel dimension 
                    tmp = transform(f)
                    while len(tmp.shape)< 3:
                        tmp = np.expand_dims(tmp,len(tmp.shape))
                    img_list.append(tmp)
                    # print("AFTER:", np.max(img_list[-1]))
                dset[idx]  = np.concatenate(img_list, axis=-1)
                tq.update(1)
            pool.starmap(lambda i, path_tuple: helper(dset, i, path_tuple), enumerate(path_lists))
            tq.close()
    else:
        print('Load', dataset_filename)
    return dataset_filename, lo_hi_map


            
class HDF5_Base_Dataset(torch.utils.data.Dataset):
    def __init__(self, train, input_structs, dataset_params={}, verbose=True):
        if verbose: print(input_structs)
        # assert that all for all features the filenames correspond
        InputFeatureStructTemplate.check_filename_correspondence(input_structs)
        
        # init members
        self.input_structs = input_structs
        self.total_frames = len([*input_structs.items()][0][1].filename_list)
        self.train = train

        # create hdf5 dataset for all features
        cache_path = dataset_params['cache_path'] if 'cache_path' in dataset_params.keys() else None 
        self.dataset_filename, self.lo_hi_map = make_multifeature_dataset(cache_path, input_structs)




    def __len__(self):
        return self.total_frames


    def __getitem__(self, i):
        # We need to close and reopen the hdf5 dataset file, as we can't be sure that all dataloader workers inherit the same file pointer! --> race conditions!!
        # see e.g. https://stackoverflow.com/questions/46045512/h5py-hdf5-database-randomly-returning-nans-and-near-very-small-data-with-multi/52438133#52438133
        with h5py.File(self.dataset_filename, 'r') as f:
            data = f['data']             # in shape index x w x h x c
            stack = (data[i])   # in shape w x h x c
            item_dict = {}

           # the random seed needs to be set for each worker, as the numpy random generator (which iaa also uses) 
           # is duplicated for each worker thread
           # only torch random number generator is initialized differently for each worker
           # (see https://pytorch.org/docs/stable/data.html , see "Randomness in multi-process data loading")
           # torch.utils.data.get_worker_info().seed is guarantied to be randomish for each worker and epoch
           # and torch.rand is initialized with this seed
            threadlocal_seed = get_threadlocal_seed()

            for feature in self.input_structs:
                #we want each image to be randomly transformed (cropped, etc) the same way, thus each needs the same random seed
                ia.random.seed(threadlocal_seed)
                #print("\ndataset getitem feature:" + str(feature))
                transform = self.input_structs[feature].onload_transform
                
                lo, hi = self.lo_hi_map[feature]
                feat = stack[:,:,lo:hi]
                #print("\nimg size before crop: " + str(feat.shape) + "\n")
                img = transform(feat)
                #print("\nimg size after crop: " + str(img.shape) + "\n")

                
                # write_tensor_as_image(img, 'debug/'+self.input_structs['groundtruth'].filename_list[i]+feature+str(i)+'.png')
                item_dict[feature] = img

            #print(item_dict)
            return i, self.input_structs['groundtruth'].filename_list[i], item_dict


# custom dataset to manage input features of different sizes
class HDF5_NPR_Dataset(torch.utils.data.Dataset):
    def __init__(self, train, input_structs, dataset_params={}, verbose=True):
        if verbose: print(input_structs)
        # assert that all for all features the filenames correspond
        InputFeatureStructTemplate.check_filename_correspondence(input_structs)
        #print("input structs for dataset :")
        #print(input_structs)
        # init members
        self.input_structs = input_structs
        self.total_frames = len([*input_structs.items()][0][1].filename_list)
        self.train = train

        # create hdf5 dataset for all features
        cache_path = dataset_params['cache_path'] if 'cache_path' in dataset_params.keys() else None 

        #create multiple datasets for different input sizes -> sort features according to dimension
        self.splitted_input_structs = []    #split the input_structs into multiple dicts
        self.splitted_input_dimensions = [] #save the dimensions of each of the splitted input dicts
        for feature in input_structs:
            # load single image to determine shape
            tmp = input_structs[feature].init_transform(read_image(input_structs[feature].directory, input_structs[feature].filename_list[0], input_structs[feature].extension))
            #print("Feature " + str(feature) + " has dim " + str(tmp.shape))
            if tmp.shape not in self.splitted_input_dimensions:
                # new dimesnion: add dimension and store input_struct in new dict
                #print(" shape is new: add another h5 file\n")
                self.splitted_input_dimensions.append(tmp.shape)
                self.splitted_input_structs.append({feature:input_structs[feature]})
            else:
                #print(" shape is already present: add to existing h5 file\n")
                #existing dimension -> get existing dict entry and append
                for i in range(len(self.splitted_input_dimensions)):
                    if self.splitted_input_dimensions[i] == tmp.shape:
                        self.splitted_input_structs[i][feature] = input_structs[feature]
                        break
        #for split_dict in self.splitted_input_structs:
        #    print(split_dict)
        print(self.splitted_input_dimensions)
        self.dataset_filenames = []
        self.lo_hi_maps = []
        for split_input_structs in self.splitted_input_structs:
            dataset_filename, lo_hi_map = make_multifeature_dataset(cache_path, split_input_structs)
            self.dataset_filenames.append(dataset_filename)
            self.lo_hi_maps.append(lo_hi_map)




    def __len__(self):
        return self.total_frames


    def __getitem__(self, i):
        # the random seed needs to be set for each worker, as the numpy random generator (which iaa also uses) 
        # is duplicated for each worker thread
        # only torch random number generator is initialized differently for each worker
        # (see https://pytorch.org/docs/stable/data.html , see "Randomness in multi-process data loading")
        # torch.utils.data.get_worker_info().seed is guarantied to be randomish for each worker and epoch
        # and torch.rand is initialized with this seed
        threadlocal_seed = get_threadlocal_seed()
        #determine, which dataset to use, i.e. which dimension
        #go through the datasets
        item_dict = {}
        for d in range(len(self.dataset_filenames)):
            # We need to close and reopen the hdf5 dataset file, as we can't be sure that all dataloader workers inherit the same file pointer! --> race conditions!!
            # see e.g. https://stackoverflow.com/questions/46045512/h5py-hdf5-database-randomly-returning-nans-and-near-very-small-data-with-multi/52438133#52438133
            with h5py.File(self.dataset_filenames[d], 'r') as f:
                data = f['data']             # in shape index x w x h x c
                stack = (data[i])   # in shape w x h x c
                

                for feature in self.splitted_input_structs[d]:
                    #we want each image to be randomly transformed (cropped, etc) the same way, thus each needs the same random seed
                    ia.random.seed(threadlocal_seed)
                    #print("\ndataset getitem feature:" + str(feature))
                    transform = self.input_structs[feature].onload_transform
                    
                    lo, hi = self.lo_hi_maps[d][feature]
                    feat = stack[:,:,lo:hi]
                    #print("\nimg size before crop: " + str(feat.shape) + "\n")
                    img = transform(feat)
                    #print("\nimg size after crop: " + str(img.shape) + "\n")

                    
                    # write_tensor_as_image(img, 'debug/'+self.input_structs['groundtruth'].filename_list[i]+feature+str(i)+'.png')
                    item_dict[feature] = img

        #print(item_dict)
        return i, self.input_structs['groundtruth'].filename_list[i], item_dict

# maybe interesting helpers

#     def batch_to_stack(self, item_dict):
#         if self.lo_hi == None:
#             self.lo_hi = {}
#             c = 0
#             for elem_key in item_dict:
#                 low = c
#                 hi = c + item_dict[elem_key].shape[2]
#                 self.lo_hi[elem_key] = (low, hi)
#                 c = hi

#         return np.dstack([item_dict[k] for k in item_dict])

#     def stack_to_batch(self, item_dict, stack):
#         for elem_key in item_dict:
#             lo, hi = self.lo_hi[elem_key]
#             item_dict[elem_key] = stack[lo : hi]
#         return item_dict
