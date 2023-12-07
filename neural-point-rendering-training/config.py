import torch
import datetime
from sitof.utils.dataset import HDF5_Base_Dataset, HDF5_NPR_Dataset
from sitof.models.models import INOVIS
from torchvision import transforms
import random
from torch import optim

################################################################
# query standard scheduler args
def get_scheduler_args(scheduler_type):
    if scheduler_type == optim.lr_scheduler.ReduceLROnPlateau:
        return {
            'mode' : 'min',
            'factor' : 0.75, # 0.1
            'patience' : 10, # 10
            'threshold' : 1e-4,
            'threshold_mode' : 'rel', 
            'cooldown' : 25, # 0
            'min_lr' : 0.00005,
            'eps' : 1e-8,
            'verbose' : True
        }
    elif scheduler_type == optim.lr_scheduler.MultiplicativeLR:
        return {
            'lr_lambda' : lambda epoch:0.98628,
            'last_epoch' : -1,
            'verbose' : True
        }
    else:
        return {}
#---------------------------------------------------------------

################################################################
# parameter class
# every parameter describing a run is stored here and the whole program accesses this class to query parameters
class ConfigParams:
    ################################################################
    # base initialization 
    def __init__(self, filename = None):
        if filename:
            self.load(filename)
            return
        ################################################################
        # name of training -> influences how the target folder is named 
        #---------------------------------------------------------------
        # Name of the training.
        self.training_name = datetime.datetime.now().strftime("%Y_%m_%d_%H_%M_%S")
        #---------------------------------------------------------------

        ################################################################
        # checkpoint load config with no checkpoint load dir, no checkpoint is loaded and training is created from scratch
        #---------------------------------------------------------------
        # Specify a folder of a checkpoint to be loaded.
        self.checkpoint_load_dir = None#'C:/Users/to84qufi/MA/ma-neural-point-rendering/neural-point-rendering-training/2023_01_02_18_22_35_Kitty_1-3_GT4_preInitMV/out_checkpoints/BEST__epoch-111'#'/home/woody/iwi9/iwi9009h/ma-neural-point-rendering/neural-point-rendering-training/2022_03_28_15_04_07_debug_scheduler/out_checkpoints/__epoch-41'#None 
        # Specify if the dataloader should be restore to continue training or if a new dataloader should be created from scratch.
        self.restore_dataloader = False
        #---------------------------------------------------------------

        ################################################################
        # dataset
        #---------------------------------------------------------------
        # class type of the dataset. this is not configurable
        self.dataset_type = HDF5_Base_Dataset
        # Names of the dataset folders. Each name corresponds to a folder in the training/data folder. All names should share a target resolution and must share the same ground truth resolution
        self.set_description = ""
        # Specify how many views should be loaded from the datasets. 0 equals to all views. (per dataset)
        self.max_images_per_drawelement = 0 # 0 == use all
        # Batch size for the dataloader. Choose as large as possible w.r.t. the available VRAM and image sizes.
        self.batch_size = 6
        # Test/Holdout indices while training. These Views are not used for training. (per dataset)
        self.test_indices = []# [] # empty list -> random
        # Width and Height of the target images. Should fit the width of the target images (point rendered) and must be divisible by 32.
        self.input_image_width_and_height = [512,512] # 1024
        # Width and Height of the train images. Should fit the width of the target images (point rendered) and must be divisible by 16.
        self.train_input_image_width_and_height = [256,256]
        # DEPRECATED. 'lod0' is used to reference the folders. this matches the C++ application.
        self.lod_description = ""
        #---------------------------------------------------------------

        ################################################################
        # target folder name handling
        #---------------------------------------------------------------
        # directories and output
        self.data_root = './data/'
        self.cache_output= './out_cache/'
        self.base_dir_output = self.training_name+'/' 
        self.train_output = self.base_dir_output+'out_train/'
        self.eval_output = self.base_dir_output+'out_eval/'
        self.results_output = self.base_dir_output+'out_results/'
        self.test_output = self.base_dir_output+'out_test/'
        self.checkpoints_output = self.base_dir_output+'out_checkpoints/'
        #---------------------------------------------------------------

        ################################################################
        # checkpoints saving and output of results
        #---------------------------------------------------------------
        # Specify a folder of a checkpoint to be loaded.
        # if checkpoints should be saved
        self.save_checkpoint = True
        # Specify how often checkpoints should be saved.
        self.save_checkpoint_frequency = 3
        # Specify a cooldwon of saving a new checkpoint if the loss improved compared to the last checkpoint.
        self.save_checkpoint_best_epoch_frequency = 50
        # Specify how often rendered output images are saved.
        self.write_output_nth_epoch = 100
        # Specify if rendered output images are saved.
        self.output_results = False
        #---------------------------------------------------------------

        ################################################################
        # optimizer and scheduler config
        #---------------------------------------------------------------
        # Specify the numer of epochs to be trained.
        self.epochs = 1001
        # Start Learning rate of the optimizer.
        self.learning_rate = 0.0005#0.001 # maybe increase drastically when using scheduler
        # Type of loss function.
        self.loss_function = ""
        self.scheduler_gamma = 0.98
        # 0.1 way to high, 0.01 kind of ok but probably too high, 0.001 slow but ok?
        self.scheduler_type = optim.lr_scheduler.ReduceLROnPlateau
        #self.scheduler_type = optim.lr_scheduler.MultiplicativeLR
        self.scheduler_args = get_scheduler_args(self.scheduler_type)
        #---------------------------------------------------------------

        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

        ################################################################
        # network setup
        #---------------------------------------------------------------
        # Specify a folder of a checkpoint to be loaded.
        # renderer
        self.network_type = INOVIS
        self.network_args = {}
        #---------------------------------------------------------------

        torch.autograd.set_detect_anomaly(True)
        #---------------------------------------------------------------

    ################################################################
    # for better output
    def __str__(self):
        message = ''
        message += '----------------- Config ---------------\n'
        defaultParams = vars(ConfigParams())
        for k, v in vars(self).items():
            comment = ''
            if k in defaultParams:
                default = defaultParams[k]
                if v != default:
                    comment = '\t[default: %s]' % str(default)
            message += '{:>25}: {:<30}{}\n'.format(str(k), str(v), comment)
        message += '----------------- End -------------------'
        return message

    ################################################################
    # DEPRECATED hparams are never used
    def get_hparams(self):
        hparams_dict = {}
        hparams_dict.update({k: v for k, v in vars(self).items() if type(v) in [int, float, str, bool] and ('_output' not in k)})
        hparams_dict['dataset_type'] = str(self.dataset_type)
        hparams_dict['network_type'] = str(self.network_type)

        return hparams_dict
################################################################
# output the default parameters
def defaultParams():
    return ConfigParams()

################################################################
# custom INOVIS parameters: overwrite default parameters
def myParams():
    ################################################################
    # query default parameters
    p = ConfigParams()
    p.batch_size = 2

    p.max_images_per_drawelement = 0 #50 # 0 == use all
    #for different epoch amount
    #p.epochs = 1
    
    ################################################################
    # dataset
    #---------------------------------------------------------------
    p.dataset_type = HDF5_NPR_Dataset # use custom dataset to manage inputs of different dimensions
    p.input_image_width_and_height = [512,512]#[768,512] #512 # 1024
    p.train_input_image_width_and_height = [256,256]#[384,256] #256
    p.set_description = ["bagger2"] #["dataset1", "dataset2"]
    if isinstance(p.set_description, str):
        p.set_description = [p.set_description]
    p.lod_description = "lod0"
    # choose same test indices as reference
    p.test_indices = []#[0,5,10,15,20,25,30,35,40,45,50,55]#[1, 21, 41, 61, 81, 101, 121, 141, 161, 181, 201, 221, 241, 261, 281, 301]#,
                      #381, 401, 421, 441, 461, 481, 501, 521, 541, 561, 581, 601, 621, 641, 661, 681  ]
    #---------------------------------------------------------------

    ################################################################
    # optimizer and scheduler config
    #---------------------------------------------------------------
    p.loss_function = "INOVIS" #'MSE', 'L1', 'VGG', 'SSIM', 'XIAO', 'Spectral'
    #---------------------------------------------------------------

    ################################################################
    # checkpoints saving and output of results
    #---------------------------------------------------------------
    p.output_results = True
    #---------------------------------------------------------------
    
    ################################################################
    # network arguments these match the parameters of the INOVIS network module in models.py
    #---------------------------------------------------------------
    p.network_type = INOVIS 
    if p.network_type == INOVIS:
        p.network_args = {}
        # Number of channels the current frame input contains.
        p.network_args['in_channels_current'] = [4,4,4,4]#[1,1,1,1]#[4,4,4,4]
        # Number of ground truth frames feeded to the network.
        p.network_args['in_gt_frame_amount'] = 4      
        # Number of channels the previous frames input contains.
        p.network_args['in_channels_gt'] = 4
        # Scale factor to modify the filter sizes: 4 corresponds to 16,32,64,128,256.
        p.network_args['feature_scale'] = 4
        # DEPRECATED. warping and thus feature extraction on lower resolutions is not supported anymore.
        p.network_args['feature_extraction_depth'] = 0
        # Specify if rgbd of gt should be warped and appended to the reweighting network.
        p.network_args['warp_rgbd'] = True                  #use rgbd input for reweighting? -> scale and warp
        # Specify if warped images should be written to disk.
        p.network_args['output_warped'] = False             #ouput warped information for backward warping debugging. must be disabled for real training runs
        # Specify if warped image should be masked by multiplying with 3rd channel of the movec.
        p.network_args['mask_warped'] = False
        # Specify if features should be extracted from lower res point renderings.
        p.network_args['extract_lowres_features'] = True
        #---------------------------------------------------------------
        # Specify the convolution blocks for feature extraction: point rendering.
        p.network_args['conv_block_extraction_pr'] = "gated"
        # Specify the convolution blocks for feature extraction: auxiliary images.
        p.network_args['conv_block_extraction_gt'] = "gated"
        # Specify the convolution blocks for feature reweighting.
        p.network_args['conv_block_reweighting'] = "basic"
        # Specify the convolution blocks for feature reconstruction.
        p.network_args['conv_block_reconstruction'] = "gated"
        # Specify which upsampling type is used.
        p.network_args['upsample_mode'] = "bilinear"
        # Specify which pooling type is used.
        p.network_args['pooling_mode'] = "max"
        #---------------------------------------------------------------
        # Specify if the reweighting network gets the low resolution point rendering on top.
        p.network_args['reweight_with_low_res'] = False
        # Specify if the reweighting should be disabled.
        p.network_args['disable_reweighting'] = False
        #---------------------------------------------------------------
        # Specify filter_sizes of the reconstruction network: e.g. [64, 128, 256, 512, 1024].
        p.network_args['filter_sizes'] = [32,32,32,32,32]
        # Specify the filter base of the feature extraction network of current frames.
        p.network_args['fe_current_filter_base'] = 32
        # Specify the filter base of the feature extraction network of gt images.
        p.network_args['fe_gt_filter_base'] = 16
        #---------------------------------------------------------------
    return p
#---------------------------------------------------------------
