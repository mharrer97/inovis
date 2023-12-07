import imgaug.augmenters as iaa
import imgaug as ia
from os.path import isfile

from sitof.utils.dataset import HDF5_Base_Dataset
from sitof.utils.dataloader_factory import DataloaderFactory
from sitof.utils.tensor_utils import *
from sitof.models.models import INOVIS

import config
from data_transforms import *
################################################################
# Set input data for the dataset + image augmentations
def build_factories_for_config(config):
    ################################################################
    # init factories
    init_params = {'DatasetType':config.dataset_type, 
        'dataset_param':{'cache_path':config.cache_output, 'test_indices':config.test_indices}, 'batch_size':config.batch_size, 'max_images':config.max_images_per_drawelement, 
        'split_in_train_val_test':True
    }
    factory = DataloaderFactory(**init_params)

    ################################################################
    # create target image dimensions for each resultution level
    input_eval_sizes = []
    for i in range(5):
        if isinstance(config.input_image_width_and_height,int): # input sizes is one dimensional -> just divide
            input_eval_sizes.append(config.input_image_width_and_height//(2**i)) 
        else: # input size is two dimensional -> divide both parts
            input_eval_sizes.append([config.input_image_width_and_height[0]//(2**i),config.input_image_width_and_height[1]//(2**i)]) 
    input_train_sizes= []
    for i in range(5):
        if isinstance(config.input_image_width_and_height,int): # input sizes is one dimensional -> just divide
            input_train_sizes.append(config.train_input_image_width_and_height//(2**i)) 
        else: # input size is two dimensional -> divide both parts
            input_train_sizes.append([config.train_input_image_width_and_height[0]//(2**i),config.train_input_image_width_and_height[1]//(2**i)]) 
    ################################################################
    # image augmentations
    #---------------------------------------------------------------
    # init transformations (executed on dataset.init)
    # These will be applied to the image after the first read. Results will be baked into the hdf5 dataset!
    # When you change them, the hdf5 dataset needs to be rebuild (happens automatically).
    print("input_eval_sizes: ", input_eval_sizes)
    gt_init_transform = INOVIS_GTRGB_InitTransform(input_eval_sizes[0])
    rgb_init_transform = INVOIS_RGB_InitTransform(input_eval_sizes[0])
    rgb_init_transform1 = INVOIS_RGB_InitTransform(input_eval_sizes[1])
    rgb_init_transform2 = INVOIS_RGB_InitTransform(input_eval_sizes[2])
    rgb_init_transform3 = INVOIS_RGB_InitTransform(input_eval_sizes[3])
    rgb_init_transform4 = INVOIS_RGB_InitTransform(input_eval_sizes[4])
    no_init_transform = INOVIS_NoRGB_InitTransform()
    #---------------------------------------------------------------
    # load transformations (executed on dataset.__getitem__)
    # These will always be applied when sampling a new batch. 
    # They usually need to include the ToTensor transform, but do NOT copy to the device!
    #---------------------------------------------------------------
    # for training
    train_onload_transform = INOVIS_Train_OnloadTransform(input_train_sizes[0])
    train_onload_transform1 = INOVIS_Train_OnloadTransform(input_train_sizes[1])
    train_onload_transform2 = INOVIS_Train_OnloadTransform(input_train_sizes[2])
    train_onload_transform3 = INOVIS_Train_OnloadTransform(input_train_sizes[3])
    train_onload_transform4 = INOVIS_Train_OnloadTransform(input_train_sizes[4])
    no_train_onload_transform = INOVIS_NoTrain_OnloadTransform()
    #---------------------------------------------------------------
    # for evaluation
    val_onload_transform = INOVIS_Val_OnloadTransform(input_eval_sizes[0])
    val_onload_transform1 = INOVIS_Val_OnloadTransform(input_eval_sizes[1])
    val_onload_transform2 = INOVIS_Val_OnloadTransform(input_eval_sizes[2])
    val_onload_transform3 = INOVIS_Val_OnloadTransform(input_eval_sizes[3])
    val_onload_transform4 = INOVIS_Val_OnloadTransform(input_eval_sizes[4])
    no_val_onload_transform = INOVIS_NoVal_OnloadTransform()
    ################################################################
    # add image features
    #---------------------------------------------------------------
    # gt: init_transform does a center crop exclusively  
    factory.add_img_feature('groundtruth', [config.data_root + setD + '/groundtruth/' for setD in config.set_description], init_transform=gt_init_transform, train_transform=train_onload_transform, val_transform=val_onload_transform)
    #---------------------------------------------------------------
    # point rendered input images: rgb: i_0/i_lx; d: d_0/d_lx
    # transforms are chosen according to resolution and are matching w.r.t. the area that is i.e. cropped in different resolutions
    factory.add_img_feature('i_0', [config.data_root + setD + '/input_0_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform, train_transform=train_onload_transform, val_transform=val_onload_transform)
    factory.add_img_feature('d_0', [config.data_root + setD + '/depth_0_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform, train_transform=train_onload_transform, val_transform=val_onload_transform)
    factory.add_img_feature('i_l1', [config.data_root + setD + '/input_1_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform1, train_transform=train_onload_transform1, val_transform=val_onload_transform1)
    factory.add_img_feature('d_l1', [config.data_root + setD + '/depth_1_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform1, train_transform=train_onload_transform1, val_transform=val_onload_transform1)
    factory.add_img_feature('i_l2', [config.data_root + setD + '/input_2_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform2, train_transform=train_onload_transform2, val_transform=val_onload_transform2)
    factory.add_img_feature('d_l2', [config.data_root + setD + '/depth_2_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform2, train_transform=train_onload_transform2, val_transform=val_onload_transform2)
    factory.add_img_feature('i_l3', [config.data_root + setD + '/input_3_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform3, train_transform=train_onload_transform3, val_transform=val_onload_transform3)
    factory.add_img_feature('d_l3', [config.data_root + setD + '/depth_3_' + config.lod_description +  '/' for setD in config.set_description], init_transform=rgb_init_transform3, train_transform=train_onload_transform3, val_transform=val_onload_transform3)
    #---------------------------------------------------------------
    # nearest ground truth auxiliary images: rgb: i_x; d: d_x; movecs: m_x
    # auxiliary images are always used at full size, as movecs can reference the whole image.
    # movecs are cropped similar to the point renderings
    gt_amount = config.network_args['in_gt_frame_amount']
    for gt_i in range(1,gt_amount+1):
        if config.network_type == INOVIS:
            factory.add_img_feature('i_' + str(gt_i), [config.data_root + setD + '/nearest' + str(gt_i) + '_groundtruth_0/' for setD in config.set_description], init_transform=no_init_transform, train_transform=no_train_onload_transform, val_transform=no_val_onload_transform)
            factory.add_img_feature('d_' + str(gt_i), [config.data_root + setD + '/nearest' + str(gt_i) + '_depth_0_' + config.lod_description +  '/' for setD in config.set_description], init_transform=no_init_transform, train_transform=no_train_onload_transform, val_transform=no_val_onload_transform)
            factory.add_img_feature('m_' + str(gt_i), [config.data_root + setD + '/nearest' + str(gt_i) + '_motion_0_' + config.lod_description + '/' for setD in config.set_description], init_transform=rgb_init_transform, train_transform=train_onload_transform, val_transform=val_onload_transform)
    return factory.produce('train', 'val'), factory
    #---------------------------------------------------------------

