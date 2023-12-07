import torch
from torch.utils.data import DataLoader
import dill
from .dataset import *

class DataloaderFactory:    
    def __init__(self, DatasetType:type, dataset_param, batch_size, num_workers:int=4, max_images:int=0, 
            split_in_train_val_test:bool=True, split_seed:int = 42, split_ratio:float=0.1, batch_size_val:int=1,
            pin_memory:bool=False):
        self.dataset_type = DatasetType

        #split
        self.split = split_in_train_val_test
        self.split_seed = split_seed
        self.split_val_n_test_ratio = split_ratio

        # data
        self.max_images = max_images
        self.dataset_param = dataset_param

        # 
        self.input_feature_structs = {}
        self.train_input_structs = {}
        self.val_input_structs = {} 
        self.test_input_structs = {}

        ## loader params
        self.batch_size = batch_size
        self.val_batch_size = batch_size_val
        self.pin_memory = pin_memory
        self.num_workers = num_workers

        self.produced = False

    def add_img_feature(self, name, dir_path, init_transform, train_transform=None, val_transform=None, test_transform=None):
        if name in self.input_feature_structs:
            print ("add img_feature: ", name, " exists ")
        else:
            print ("add img_feature: ", name)
        '''add some feature to the factory'''
        if self.produced:
            print("WARNING: You shall NOT alter factory parameters after you produced your first dataset! This may lead to undefined behavior regarding the saved/loaded state!!")
        if self.split:
            if not train_transform or not val_transform:
                print("for splitted datasets you need to provide a train and an val/test transform!")
                exit()

        # transform can be given as TransformMonitor or as plain transform! Do not wrap it twice!
        if not type(init_transform) == DataTransformsMonitor: init_transform = DataTransformsMonitor(init_transform)
        if not type(train_transform) == DataTransformsMonitor: train_transform = DataTransformsMonitor(train_transform)
        if not type(val_transform) == DataTransformsMonitor: val_transform = DataTransformsMonitor(val_transform)
        if not type(test_transform) == DataTransformsMonitor: test_transform = DataTransformsMonitor(test_transform)

        self.input_feature_structs[name] =  InputFeatureStructTemplate(
                name, dir_path, init_transform, train_transform, val_transform, test_transform, self.max_images)     

        if self.split:
            self.train_input_structs[name], self.val_input_structs[name], self.test_input_structs[name] = self.input_feature_structs[name].init_split( 
                self.split_seed, self.split_val_n_test_ratio,verbose=False, test_indices = self.dataset_param['test_indices'])

    # does the same as add_image_feratures, but takes multiple paths as input to be fused
    def add_img_features(self, name, dir_path, init_transform, train_transform=None, val_transform=None, test_transform=None):
        '''add some feature to the factory'''
        if self.produced:
            print("WARNING: You shall NOT alter factory parameters after you produced your first dataset! This may lead to undefined behavior regarding the saved/loaded state!!")
        if self.split:
            if not train_transform or not val_transform:
                print("for splitted datasets you need to provide a train and an val/test transform!")
                exit()

        # transform can be given as TransformMonitor or as plain transform! Do not wrap it twice!
        if not type(init_transform) == DataTransformsMonitor: init_transform = DataTransformsMonitor(init_transform)
        if not type(train_transform) == DataTransformsMonitor: train_transform = DataTransformsMonitor(train_transform)
        if not type(val_transform) == DataTransformsMonitor: val_transform = DataTransformsMonitor(val_transform)
        if not type(test_transform) == DataTransformsMonitor: test_transform = DataTransformsMonitor(test_transform)

        self.input_feature_structs[name] =  InputFeatureStructTemplate(
                name, dir_path, init_transform, train_transform, val_transform, test_transform, self.max_images)     

        if self.split:
            self.train_input_structs[name], self.val_input_structs[name], self.test_input_structs[name] = self.input_feature_structs[name].init_split( 
                self.split_seed, self.split_val_n_test_ratio,verbose=False, test_indices = self.dataset_param['test_indices'])
            
    def produce(self, *set_type:str):
        if len(self.input_feature_structs) == 0:
            raise ValueError ('No features added, nothing to produce...')

        loaders = {}

        if self.split:
            sets = {}
            if 'train' in set_type:
                sets['train'] = self.dataset_type(train=True, input_structs=self.train_input_structs, dataset_params=self.dataset_param, verbose=False)
            if 'val' in set_type:
                sets['val'] = self.dataset_type(train=False, input_structs=self.val_input_structs, dataset_params=self.dataset_param, verbose=False)
            if 'test' in set_type:
                sets['test'] = self.dataset_type(train=False, input_structs=self.test_input_structs, dataset_params=self.dataset_param, verbose=False)

            if len(sets) < len(set_type):
                print("Invalid set type detected!")
                exit(0)

            #creates h5 dataset
            loaders = {k: DataLoader(sets[k],
                                batch_size=self.batch_size if k == 'train' else self.val_batch_size, 
                                shuffle=(k=='train'), 
                                num_workers=self.num_workers, 
                                pin_memory=self.pin_memory,
                                collate_fn=self.dataset_param['collate_fn'] if 'collate_fn' in self.dataset_param.keys() else None                         
                                ) 
                            for k in sets}

        else:
            sets = {}
            for k in set_type:
                for _, struct in self.input_feature_structs.items():
                    struct.init(k)
                
                sets[k] = self.dataset_type(train=('train' == k), input_structs=self.input_feature_structs, dataset_params=self.dataset_param)
            
            #loaders[k] = DataLoader(ds,
            #        batch_size=self.batch_size if k =='train' else self.val_batch_size,
            #        shuffle=(k =='train'),
            #        num_workers=self.batch_size if k =='train' else self.val_batch_size,
            #        pin_memory=self.pin_memory)
            loaders = {k: DataLoader(sets[k],
                                batch_size=self.batch_size if k == 'train' else self.val_batch_size, 
                                shuffle=(k=='train'), 
                                num_workers=self.num_workers, 
                                pin_memory=self.pin_memory,
                                collate_fn=self.dataset_param['collate_fn'] if 'collate_fn' in self.dataset_param.keys() else None                         
                                ) 
                            for k in sets}
            
        return loaders

    def has_feature(self, name):
        return name in self.input_feature_structs
        
    def get_input_feature_init_transform(self, name):
        return self.input_feature_structs[name].init_transform.transform
    def get_input_feature_val_onload_transform(self, name):
        return self.input_feature_structs[name].val_onload_transform.transform
    def get_input_feature_train_onload_transform(self, name):
        return self.input_feature_structs[name].train_onload_transform.transform
    def get_input_feature_test_onload_transform(self, name):
        return self.input_feature_structs[name].test_onload_transform.transform

    def set_input_feature_test_onload_transform(self, name, transform):
        if not type(transform) == DataTransformsMonitor: transform = DataTransformsMonitor(transform)
        self.input_feature_structs[name].test_onload_transform = transform
        self.test_input_structs[name].test_onload_transform = transform
        self.input_feature_structs[name].onload_transform = transform
        self.test_input_structs[name].onload_transform = transform

    def set_input_feature_val_onload_transform(self, name, transform):
        if not type(transform) == DataTransformsMonitor: transform = DataTransformsMonitor(transform)
        self.input_feature_structs[name].val_onload_transform = transform
        self.val_input_structs[name].val_onload_transform = transform
        self.input_feature_structs[name].onload_transform = transform
        self.val_input_structs[name].onload_transform = transform

    def set_input_feature_train_onload_transform(self, name, transform):
        if not type(transform) == DataTransformsMonitor: transform = DataTransformsMonitor(transform)
        self.input_feature_structs[name].train_onload_transform = transform
        self.train_input_structs[name].train_onload_transform = transform
        self.input_feature_structs[name].onload_transform = transform
        self.train_input_structs[name].onload_transform = transform

    def save(self, filename):
        with open(filename, 'wb') as f:
            dill.dump(self, f)

    @classmethod
    def load(cls,filename):
        with open(filename, 'rb') as f:
            factory = dill.load(f)
            return factory

   