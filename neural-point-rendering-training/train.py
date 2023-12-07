# general stuff
import argparse
import logging
import datetime
import os
import io
import shutil
from zipfile import ZipFile
import sys
from PIL import Image
import numpy as np
import math
from sitof.utils.color_conversion import *

# torch 
import torch
import torch.nn as nn
from torch import optim
from tqdm import tqdm
from torchsummary import summary
from torch.utils.tensorboard import SummaryWriter
from torch.utils.data import DataLoader
import torch.nn.functional as F

# framework
import sitof.models.models
from sitof.models.models import get_current_learning_rate, INOVIS
import sitof.models.model_zoo
from sitof.utils.tensor_utils import *
import sitof.utils.pytorch_ssim
from sitof.utils import *
from sitof.utils.vgg_loss import VGG_loss
# project
import config
import build_dataloaders

############
# for logging use:
# $ tensorboard --logdir=./out_runs --bind_all


#################################################
# globals 
state = {}
tensorboard_writer = None
# use tagging convention for tensorboard writer
epoch_losses = {}
best_losses = {}
debug_counter = 0

def print_memory_stats(device):
    memory_stats = torch.cuda.memory_stats(device)
    logging.info(f'''Memory stats for device {device}:
    allocated:     {memory_stats['active_bytes.all.allocated'] / 1000000}
    current:       {memory_stats['active_bytes.all.current'] / 1000000}
    peak:          {memory_stats['active_bytes.all.peak']/ 1000000}
      ''')

    #for name in memory_stats.items():
        #print(name)

    #print(memory_stats['active_bytes.all.allocated'])
    #'active_bytes.all.allocated'

    
def train():
    global state
    global epoch_losses
    ################################################################
    # Start Training
    ################################################################
    # set training state
    #---------------------------------------------------------------
    try:
        #---------------------------------------------------------------
        # init epoch variables
        start_epoch = state['epoch']
        state['best_epoch'] = start_epoch
        last_best_epoch = start_epoch
        last_checkpoint = start_epoch
        #---------------------------------------------------------------
        # loop over number of epochs
        for epoch in range(start_epoch, state['config'].epochs):
            state['epoch'] = epoch
            epoch_losses = {}
            torch.cuda.empty_cache()
            #---------------------------------------------------------------
            # train current epoch
            train_one_epoch(epoch)
            #---------------------------------------------------------------
            # eval current epoch
            eval_one_epoch(epoch)
            #---------------------------------------------------------------
            # print information about the trained epoch        
            for k, l in epoch_losses.items(): tensorboard_writer.add_scalar(k, l, epoch)
            print("best: " , state['best_loss'], " current: ", epoch_losses['Summed Loss/val'], ' last_checkpoint was ' , epoch-last_checkpoint, "epochs ago")
            if state['best_loss'] > epoch_losses['Summed Loss/val']:
                print('New Best Loss')
                state['best_loss'] = epoch_losses['Summed Loss/val']
                state['best_epoch'] = epoch
                best_losses = {**epoch_losses}

                best_losses['best_epoch'] = epoch
            ################################################################
            # Log the last trained epoch
            ################################################################
                #---------------------------------------------------------------
                # save checkpoint if necessary
                #---------------------------------------------------------------
                # if cooldown for BEST checkpoint saving is 
                if state['config'].save_checkpoint and epoch - last_checkpoint >= state['config'].save_checkpoint_best_epoch_frequency:
                    print("write best epoch because last checkpoint was (more than) " , state['config'].save_checkpoint_best_epoch_frequency, " ago and current epoch is best")
                    save_state(state['config'].checkpoints_output + 'BEST')
                    last_checkpoint = epoch
                last_best_epoch = state['best_epoch']
            #---------------------------------------------------------------
            # save checkpoint if necessary
            #---------------------------------------------------------------
            # if frequency for checkpoint saving is 
            if state['config'].save_checkpoint and 0 == epoch % state['config'].save_checkpoint_frequency and epoch !=0 and epoch-last_checkpoint != 0:
                print("write checkpoint because epoch modulo ferequency == 0  with frequency" , state['config'].save_checkpoint_frequency)
                save_state(state['config'].checkpoints_output )
                last_checkpoint = epoch
            tensorboard_writer.flush()
            #---------------------------------------------------------------
    ################################################################
    # Keyboard interrupt handling 
    ################################################################
    except KeyboardInterrupt:
        save_state(state['config'].checkpoints_output+'INTERRUPTED')
        print({k.replace('/', '_h/'): v for k, v in best_losses.items()})
        tensorboard_writer.add_hparams(
            state['config'].get_hparams(),
            metric_dict={k+'_h': v for k, v in best_losses.items()},
            run_name='hyper'
        )
        tensorboard_writer.close()
        try:
            sys.exit(0)
        except SystemExit:
            os._exit(0)
    #---------------------------------------------------------------
    ################################################################
    # hyper parameter?
    ################################################################
    tensorboard_writer.add_hparams(
            state['config'].get_hparams(),
            metric_dict={k+'_h': v for k, v in best_losses.items()},
            run_name='hyper'
        )
    tensorboard_writer.close()

################################################################
# TODO Spectral Loss (Not quite sure if implemented correctly)
def spectral_loss(rendered, groundtruth):
    mse_loss = torch.nn.functional.mse_loss
    mse = mse_loss(rendered,groundtruth)
    fft_ren = torch.fft.fft2(rendered, norm='ortho')
    fft_gt  = torch.fft.fft2(groundtruth, norm='ortho') 
    ampl = torch.mean((fft_ren.abs() - fft_gt.abs())**2)
    return ampl#mse + ampl

################################################################
# Calculate losses between rendered and gt
def calc_losses(rendered, groundtruth):
    global state
    #---------------------------------------------------------------
    # define loss functions
    mse_loss = torch.nn.functional.mse_loss
    l1_loss = torch.nn.functional.l1_loss
    #---------------------------------------------------------------
    # calc losses
    losses = {}
    losses['MSE'] = mse_loss(rendered, groundtruth)
    losses['L1'] =  l1_loss(rendered, groundtruth) 
    losses['VGG'] = state['vgg_loss'](rendered,groundtruth)
    losses['SSIM'] = 1.0 - pytorch_ssim.SSIM()(rendered, groundtruth) 
    losses['Spectral'] = spectral_loss(rendered,groundtruth)
    #---------------------------------------------------------------
    # INOVIS and XIAO are mixtures of SSIM and VGG loss
    losses['XIAO'] = losses['SSIM'] + 0.1*losses['VGG'] #xiao loss
    losses['INOVIS'] = 0.025 * losses['SSIM'] + 0.1*losses['VGG']# + 1 *  losses['Spectral']
    #---------------------------------------------------------------
    # pick the used loss function according to config
    losses['Loss'] = losses[state['config'].loss_function]
    return losses
    #---------------------------------------------------------------
    
################################################################
# Train an epoch
def train_one_epoch(epoch):
    global state
    global epoch_losses
    
    torch.set_grad_enabled(True)
    ################################################################ 
    # init epoch variables
    #---------------------------------------------------------------
    epoch_losses['Summed Loss/train'] = 0
    
    frame_counter = 0
    max_debug_images = 10
    wrote_debug_images = 0
    #---------------------------------------------------------------
    # abbreviations -- query model, optimizer and scheduler
    network = state['model_zoo'].models['renderer']
    r_optim = state['model_zoo'].optimizers['renderer_opt']
    scheduler = state['model_zoo'].schedulers['scheduler']
    #---------------------------------------------------------------
    # print logging
    if state['config'].output_results and (0 == epoch % state['config'].write_output_nth_epoch or epoch == state['config'].epochs -1):
            print("Write Results this epoch")
            network.warp_counter = 0
            #network.output_warped = True
    ################################################################
    # Train
    #---------------------------------------------------------------
    # progress bar      
    with tqdm(total=len(state['dataloaders']['train'].dataset), desc=f"Train Epoch {epoch + 1}/{state['config'].epochs} @ _lr={state['config'].learning_rate:.4f}, rend_lr={get_current_learning_rate(r_optim)[0]:.4f}", unit='img',
                leave=True) as pbar:
        train_epoch_losses = None
        #---------------------------------------------------------------
        # set to train mode
        network.train()
        ################################################################
        # batch loop
        batch_id = 0
        for i,filenames, batch in state['dataloaders']['train']:
            #---------------------------------------------------------------
            # put input tensors to device
            on_device_batch = {}
            for key in batch:
                on_device_batch[key] = batch[key].clone().detach()
                on_device_batch[key] = on_device_batch[key].to(state['config'].device)   
            results = {}      
            ################################################################
            # forward pass
            #---------------------------------------------------------------
            # create network input
            if state['config'].network_type == INOVIS:
                #---------------------------------------------------------------
                # tuple of point rendered input tensors of different sizes: full res; 1/2 res; 1/4 res; 1/8 res.
                current = (torch.cat((on_device_batch['i_0'],on_device_batch['d_0'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l1'],on_device_batch['d_l1'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l2'],on_device_batch['d_l2'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l3'],on_device_batch['d_l3'][:,0:1,:,:]),1) )
                #---------------------------------------------------------------
                # tuple of #aux_images input data. Each with 6-7 channels: 3 channels rgb; 1 channel d, 2-3 channels movecs (depending on if mask information is given)
                movec_c = 3 if state['config'].network_args['mask_warped'] else 2
                previous = []
                movecs = []
                for gt_i in range(1,state['config'].network_args['in_gt_frame_amount']+1):
                    previous += [torch.cat((on_device_batch['i_' + str(gt_i)][:,0:3,:,:],on_device_batch['d_' + str(gt_i)][:,0:1,:,:]),1)]
                    movecs += [on_device_batch['m_' + str(gt_i)][:,0:movec_c,:,:]]
                previous = tuple(previous)
                movecs = tuple(movecs)
                #---------------------------------------------------------------
                # actual forward pass
                rendered = network(current,previous, movecs)
                results['rendered'] = rendered
            else:
                print("ERROR: unknown network type!")
            ################################################################
            # calc loss
            batch_losses = calc_losses(rendered, on_device_batch['groundtruth'])

            ################################################################
            # backward pass
            r_optim.zero_grad()
            batch_losses['Loss'].backward()
            r_optim.step()

            ################################################################
            # output losses and update progress bar
            if train_epoch_losses is None:
                train_epoch_losses = {k:v.item() for k, v in batch_losses.items()}
            else:
                train_epoch_losses = {k: train_epoch_losses[k] + batch_losses[k].item() for k in batch_losses}
            
            frame_counter += state['config'].batch_size
            pbar.set_postfix(**{k: v/ len(state['dataloaders']['train']) for k, v in train_epoch_losses.items()})
            pbar.update(on_device_batch['groundtruth'].size()[0])

            ################################################################
            # debug out -- write images
            if epoch % state['config'].write_output_nth_epoch == 0 and (wrote_debug_images < max_debug_images):
                base_dir = state['config'].train_output
                fn_suffix = f'_{epoch:03}_{batch_id}.png'
                
                wrote_debug_images += write_input_output_images(on_device_batch, results, base_dir, fn_suffix)
            batch_id += 1
        # batch loop END
        #---------------------------------------------------------------
        epoch_losses.update({k+'/train' : v / len(state['dataloaders']['train']) for k, v in train_epoch_losses.items()})
        epoch_losses['Summed Loss/train'] = train_epoch_losses['Loss']
        #---------------------------------------------------------------
    network.output_warped = False
    #---------------------------------------------------------------


################################################################
# Eval an epoch
def eval_one_epoch(epoch):
    global state
    global epoch_losses
    global debug_counter

    with torch.no_grad():
        epoch_losses['Summed Loss/val'] = 0
        ################################################################ 
        # init epoch variables
        #---------------------------------------------------------------
        frame_counter = 0

        #---------------------------------------------------------------
        # abbreviations -- query model, optimizer and scheduler
        network = state['model_zoo'].models['renderer']
        dataloader = state['dataloaders']['val']

        val_epoch_losses = None
        #---------------------------------------------------------------
        # print logging
        if state['config'].output_results and (0 == epoch % state['config'].write_output_nth_epoch or epoch == state['config'].epochs -1):
            print("Write Results this epoch")
            network.warp_counter = 0
            network.output_warped = True
        
        ################################################################
        # Eval
        #---------------------------------------------------------------
        # progress bar      
        with tqdm(total=len(dataloader.dataset), desc=f"Val Epoch {epoch + 1}/{state['config'].epochs}", unit='img',
                    leave=True) as pbar:
            #---------------------------------------------------------------
            # set to eval mode
            network.eval()
            ################################################################
            # batch loop
            batch_id = 0
            for i, filename, batch in dataloader:
                #---------------------------------------------------------------
                # put input tensors to device
                on_device_batch = {}
                for key in batch:
                    on_device_batch[key] = batch[key].clone()
                    on_device_batch[key] = on_device_batch[key].to(state['config'].device) 
                results = {}      
                                
                ################################################################
                # forward pass
                #---------------------------------------------------------------
                if state['config'].network_type == INOVIS:
                    #---------------------------------------------------------------
                    # tuple of point rendered input tensors of different sizes: full res; 1/2 res; 1/4 res; 1/8 res.
                    current = (torch.cat((on_device_batch['i_0'],on_device_batch['d_0'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l1'],on_device_batch['d_l1'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l2'],on_device_batch['d_l2'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l3'],on_device_batch['d_l3'][:,0:1,:,:]),1) )
                    #---------------------------------------------------------------
                    # tuple of #aux_images input data. Each with 6-7 channels: 3 channels rgb; 1 channel d, 2-3 channels movecs (depending on if mask information is given)
                    movec_c = 3 if state['config'].network_args['mask_warped'] else 2
                    previous = []
                    movecs = []

                    for gt_i in range(1,state['config'].network_args['in_gt_frame_amount'] + 1):
                        previous += [torch.cat((on_device_batch['i_' + str(gt_i)][:,0:3,:,:],on_device_batch['d_' + str(gt_i)][:,0:1,:,:]),1)]
                        movecs += [on_device_batch['m_' + str(gt_i)][:,0:movec_c,:,:]]
                    previous = tuple(previous)
                    movecs = tuple(movecs)
                    #---------------------------------------------------------------
                    # actual forward pass
                    rendered = network(current,previous, movecs)
                    results['rendered'] = rendered

                else:
                    print("ERROR: unknown network type!")
                
                ################################################################
                # calc loss
                batch_losses = calc_losses(rendered, on_device_batch['groundtruth'])

                ################################################################
                # output losses and update progress bar
                if val_epoch_losses is None:
                    val_epoch_losses = {k: v.item() for k, v in batch_losses.items()}
                else:
                    val_epoch_losses = {k: val_epoch_losses[k] + batch_losses[k].item() for k in batch_losses}
                
                frame_counter += state['config'].batch_size
                pbar.set_postfix(**{k: v/ len(dataloader) for k, v in val_epoch_losses.items()})
                pbar.update(on_device_batch['groundtruth'].size()[0])
                ################################################################
                # debug out -- write images  
                if 0 == epoch % state['config'].write_output_nth_epoch and batch_id < 4:
                    base_dir = state['config'].eval_output
                    fn_suffix = f'_{epoch:03}_{batch_id}.png'
                    write_input_output_images(on_device_batch, results, base_dir, fn_suffix)
                if state['config'].output_results and (0 == epoch % state['config'].write_output_nth_epoch or epoch == state['config'].epochs -1):
                    base_dir = state['config'].results_output 
                    write_tensor_as_image(torch.cat((results['rendered'][0],on_device_batch['groundtruth'][0]),2), base_dir + "Result" + str(batch_id)+".png", input_range=[0,1])


                batch_id += 1
            # batch loop END
            #---------------------------------------------------------------
            epoch_losses.update({k+'/val' : v / len(dataloader) for k, v in val_epoch_losses.items()})
            epoch_losses['Summed Loss/val'] = val_epoch_losses['Loss']
            #---------------------------------------------------------------
        network.output_warped = False
        #---------------------------------------------------------------

################################################################
# Trace model
def trace_model(dirpath):
    global state
    global epoch_losses
    global debug_counter


    torch.cuda.empty_cache()

    with torch.no_grad():
        ################################################################
        # init epoch variables
        frame_counter = 0

        #---------------------------------------------------------------
        # abbreviations -- query model, optimizer and scheduler
        network = state['model_zoo'].models['renderer']
        dataloader = state['dataloaders']['val']

        #---------------------------------------------------------------
        # progress bar      
        #with tqdm(total=len(dataloader.dataset), desc=f"Val Epoch {epoch + 1}/{state['config'].epochs}", unit='img',
        #            leave=True) as pbar:

        # set to eval mode
        network.eval()

        ################################################################
        # batch loop
        batch_id = 0
        for i, filename, batch in dataloader:
            #---------------------------------------------------------------
            # put input tensors to device
            on_device_batch = {}
            for key in batch:
                on_device_batch[key] = batch[key].clone()
                on_device_batch[key] = on_device_batch[key].to(state['config'].device) 
            results = {}      
                            
            ################################################################
            # forward pass
            #---------------------------------------------------------------
            if state['config'].network_type == INOVIS:
                #---------------------------------------------------------------
                # tuple of point rendered input tensors of different sizes: full res; 1/2 res; 1/4 res; 1/8 res.
                current = (torch.cat((on_device_batch['i_0'],on_device_batch['d_0'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l1'],on_device_batch['d_l1'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l2'],on_device_batch['d_l2'][:,0:1,:,:]),1),
                            torch.cat((on_device_batch['i_l3'],on_device_batch['d_l3'][:,0:1,:,:]),1) )
                #---------------------------------------------------------------
                # tuple of #aux_images input data. Each with 6-7 channels: 3 channels rgb; 1 channel d, 2-3 channels movecs (depending on if mask information is given)
                movec_c = 3 if state['config'].network_args['mask_warped'] else 2
                previous = []
                movecs = []

                for gt_i in range(1,state['config'].network_args['in_gt_frame_amount'] + 1):
                    previous += [torch.cat((on_device_batch['i_' + str(gt_i)][:,0:3,:,:],on_device_batch['d_' + str(gt_i)][:,0:1,:,:]),1)]
                    movecs += [on_device_batch['m_' + str(gt_i)][:,0:movec_c,:,:]]
                previous = tuple(previous)
                movecs = tuple(movecs)
                rendered = network(current,previous, movecs)
                #---------------------------------------------------------------
                # actual forward pass
                results['rendered'] = rendered
                jit_trace_model(network, (current,previous,movecs), dirpath + '_inovis.pt')
            #---------------------------------------------------------------
            # break, because we only need one batch input to trace the model 
            break
            #---------------------------------------------------------------
        
################################################################
# pytorch trace model
def jit_trace_model(model, exampletensor, save_path):
     
     traced_script_module = torch.jit.trace(model, exampletensor)
     frozen_model = torch.jit.optimize_for_inference(traced_script_module.eval())
     torch.jit.save(frozen_model,save_path)

################################################################
# crete a clean state -> no checkpoint loading
def create_clean_state(configParams):
    global state
    ################################################################
    ## create clean state
    #---------------------------------------------------------------
    # config, etc
    state['config'] = configParams
    state['epoch'] = 0
    state['best_loss'] = float('inf')
    state['model_zoo'] = sitof.models.model_zoo.ModelZoo(configParams.device)
    state['model_zoo'].add_model('renderer', configParams.network_type, configParams.network_args, 'normal')
    param_size = 0
    buffer_size = 0
    num_params = 0
    for param in state['model_zoo'].models['renderer'].parameters():
        # print(param)
        param_size += param.nelement() * param.element_size()
        num_params += param.nelement()
    for buffer in state['model_zoo'].models['renderer'].buffers():
        buffer_size += buffer.nelement() * buffer.element_size()
    size_all_mb = (param_size + buffer_size) / 1024**2
    # size_all_mb = (param_size ) / 1024**2
    print('model size: {:.3f}MB'.format(size_all_mb))
    print('parameter_amount: {}'.format(num_params))
    #---------------------------------------------------------------
    # dataloader, dataset
    dataloaders, factory = build_dataloaders.build_factories_for_config(configParams)
    state['dataloader_factories'] = factory
    state['dataloaders'] = dataloaders
    #---------------------------------------------------------------
    # model_zoo: model; optimizer; sheduler 

    state['model_zoo'].add_optimizer('renderer_opt', ['renderer'], configParams.learning_rate)
    state['model_zoo'].add_scheduler('scheduler', 'renderer_opt', configParams.scheduler_type, configParams.scheduler_args)
    #---------------------------------------------------------------
    # vgg loss
    state['vgg_loss'] = VGG_loss(configParams.device)
    #---------------------------------------------------------------

################################################################
# load state -> checkpoint loading
def load_state(dir_name, configParams):
    global state
    ################################################################
    ## load state
    #---------------------------------------------------------------
    # sets and initializes state['config'], state['dataloaders'], state['model_zoo'], state['renderer'], state['renderer_optimizer']
    #---------------------------------------------------------------
    # config, etc
    # store new config
    configParams_new = configParams
    #---------------------------------------------------------------
    ## create state for resuming
    meta = torch.load(dir_name+"/meta.pth")
    print(meta)
    #---------------------------------------------------------------
    print("LOAD Config")
    state["epoch"] = meta['epoch']
    state['best_loss'] = meta['best_loss']
    state['config'] = meta['config']
    state['config'].training_name = meta['config'].training_name+'++'+configParams.training_name # We append the original name
    configParams = state['config']
    #---------------------------------------------------------------
    # handle dataloader
    print("LOAD Dataloader: restore Dataloader: ", configParams_new.restore_dataloader)
    #---------------------------------------------------------------
    # restore dataloader: TODO deprecated, because cache is not written
    if configParams_new.restore_dataloader:
        print("LOAD Checkpointed Dataloader")
        state['dataloader_factories'] = sitof.utils.dataloader_factory.DataloaderFactory.load(dir_name+'/datafactory.pth')
        state['dataloaders'] = state['dataloader_factories'].produce('train', 'val')
    #---------------------------------------------------------------
    # create new dataloader
    else:
        #---------------------------------------------------------------
        # dataset should be created from scratch -> set respective dataset specified in the new config
        print("CREATE New Dataloader")
        #---------------------------------------------------------------
        # reset training epoch to 0
        state['epoch'] = 0
        state['best_loss'] = float('inf')
        #---------------------------------------------------------------
        # output into a new folder
        state['config'].base_dir_output = configParams_new.base_dir_output
        state['config'].train_output = configParams_new.train_output
        state['config'].eval_output = configParams_new.eval_output
        state['config'].results_output = configParams_new.results_output
        state['config'].test_output = configParams_new.test_output
        state['config'].checkpoints_output = configParams_new.checkpoints_output
        #---------------------------------------------------------------
        #checkpoint behaviour
        state['config'].save_checkpoint_frequency = configParams_new.save_checkpoint_frequency
        state['config'].save_checkpoint_best_epoch_frequency = configParams_new.save_checkpoint_best_epoch_frequency
        #---------------------------------------------------------------
        #dataset config
        state['config'].set_description = configParams_new.set_description
        state['config'].lod_description = configParams_new.lod_description
        state['config'].cache_output = configParams_new.cache_output
        state['config'].max_images_per_drawelement = configParams_new.max_images_per_drawelement
        state['config'].input_image_width_and_height = configParams_new.input_image_width_and_height
        state['config'].train_input_image_width_and_height = configParams_new.train_input_image_width_and_height
        state['config'].batch_size = configParams_new.batch_size
        state['config'].test_indices = configParams_new.test_indices
        #---------------------------------------------------------------
        # log new config
        print ("new dataset: ", configParams.set_description)
        print(configParams)
        dataloaders, factory = build_dataloaders.build_factories_for_config(configParams)
        state['dataloader_factories'] = factory
        state['dataloaders'] = dataloaders
    #---------------------------------------------------------------
    print("Config after loaded Checkpoint")
    print(state['config'])
    ########################################################
    # model
    print("LOAD Model")
    state['model_zoo'] = sitof.models.model_zoo.ModelZoo.load(dir_name+'/model_zoo.pth')
    ########################################################
    # vgg loss
    state['vgg_loss'] = VGG_loss(configParams.device) #should be fine to create here anew, because the vgg loss is pretrained
    #---------------------------------------------------------------

   
################################################################
# save state to 
def save_state(dir_name):
    global state
    ################################################################
    # saves current state to 
    #---------------------------------------------------------------
    # create checkpoint directory
    print('save_state to: ', dir_name)
    checkpoint_dir = dir_name + "__epoch-"+ str(state["epoch"])
    create_output_dir(checkpoint_dir)
    #---------------------------------------------------------------
    # save metadata
    meta_dict = {
        'epoch' : state["epoch"],
        'best_loss' : state['best_loss'],
        'config' : state['config']
    }
    logging.info(str(meta_dict))
    torch.save(meta_dict, checkpoint_dir+"/meta.pth")
    #---------------------------------------------------------------
    # save datafactory
    state['dataloader_factories'].save(checkpoint_dir+'/datafactory.pth')
    #---------------------------------------------------------------
    # save model
    state['model_zoo'].save(checkpoint_dir+'/model_zoo.pth')
    #---------------------------------------------------------------
    # trace model
    trace_model(checkpoint_dir)
    logging.info('Saved state to '+checkpoint_dir)
    #---------------------------------------------------------------

################################################################
# init training
def init(configParams):
    global state
    state = {}
    ################################################################
    # general init 
    torch.set_num_threads(4)
    # logging
    logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')
    logging.info(f'Using device {configParams.device}')
    #---------------------------------------------------------------
    # create cache output
    try:
        os.mkdir(configParams.cache_output)
    except:
        ()
    #---------------------------------------------------------------
    # init state (state['config'], state['network'], state['network_optimizer'])
    if configParams.checkpoint_load_dir: 
        load_state(configParams.checkpoint_load_dir, configParams)
    else:
        create_clean_state(configParams)
    #---------------------------------------------------------------
    # create out dirs AFTER loading the config!
    create_output_dir("./output_warped/")
    create_output_dir(configParams.base_dir_output)
    create_output_dir(configParams.train_output)
    create_output_dir(configParams.eval_output)
    if(configParams.output_results):
        create_output_dir(configParams.results_output)
    try:
        os.mkdir(configParams.checkpoints_output)
    except:
        ()
    #---------------------------------------------------------------
    # write network file
    with open(configParams.checkpoints_output + "network.txt", 'w') as f:
        f.write("name: " + configParams.training_name + "\n")
        f.write("feature_extraction_depth: " + str(configParams.network_args['feature_extraction_depth']) + "\n")
        f.write("movec_channels: 3\n")
        f.write("groundtruth_amount: " + str(configParams.network_args['in_gt_frame_amount']) + "\n")
        
        is_preinit = False
        for dset in configParams.set_description:
            if "preInitMV" in dset:
                is_preinit = True
                break
        if is_preinit:
            f.write("prevInitMV: 1\n")
        else:
            f.write("prevInitMV: 0\n")
    #---------------------------------------------------------------

################################################################
# start training
def start_training():
    #---------------------------------------------------------------
    # more logging setup
    global tensorboard_writer
    global state
    tensorboard_writer = SummaryWriter('out_runs/' + state['config'].training_name)
    #---------------------------------------------------------------
    # train
    logging.info(f'''Starting training:
            Epochs:          {state['config'].epochs}
            Training size:   {len(state['dataloaders']['train'])}
            Validation size: {len(state['dataloaders']['val'])}
            Checkpoints:     {state['config'].save_checkpoint}
            Device:          {state['config'].device.type}
        ''')
    train()
    #---------------------------------------------------------------
    # clean up state 
    state = {}
    #---------------------------------------------------------------

################################################################
# main function
def main(configParams):
    init(configParams)
    start_training()
    #---------------------------------------------------------------

################################################################
# helper function to parse a boolean from string -- argparse does not handle booleans well -.-
def getBoolFromStr(val):
    return True if val[0] == "T" else False
    #---------------------------------------------------------------

################################################################
# cmd argument parser 
def parseArgs():
    ################################################################
    # init parser
    params = config.myParams()
    print("Custom Overwrite for network params:")
    import argparse
    parser = argparse.ArgumentParser()
    ################################################################
    # setup parser
    #---------------------------------------------------------------
    # training parameters
    parser.add_argument('name', type=str, nargs='?', help="Name of the training.")
    # settings for checkpoint loading
    parser.add_argument('-checkpoint_load_dir', type=str, help="Specify a folder of a checkpoint to be loaded.")
    parser.add_argument('-restore_dataloader',  type=str, choices=['True', 'False'], help="Specify if the dataloader should be restore to continue training or if a new dataloader should be created from scratch.")
    # settings for checkpoint saving
    parser.add_argument('-save_checkpoint_frequency',            type=int, help="Specify how often checkpoints should be saved.")
    parser.add_argument('-save_checkpoint_best_epoch_frequency', type=int, help="Specify a cooldwon of saving a new checkpoint if the loss improved compared to the last checkpoint.")
    # settings for dataset
    parser.add_argument('-set_description', type=str, nargs="*", help="Names of the dataset folders. Each name corresponds to a folder in the training/data folder. All names should share a target resolution and must share the same ground truth resolution")
    parser.add_argument('-dataset_size',    type=int, help="Specify how many views should be loaded from the datasets. 0 equals to all views.")
    parser.add_argument('-batch_size',      type=int, help="Batch size for the dataloader. Choose as large as possible w.r.t. the available VRAM and image sizes.")
    parser.add_argument('-test_indices',    type=int, nargs="*", help="Test/Holdout indices while training. These Views are not used for training.")
    parser.add_argument('-input_width',     type=int, help="Width of the target images. Should fit the width of the groundtruth/target images and must be divisible by 32.")
    parser.add_argument('-input_height',    type=int, help="Height of the target images. Should fit the height of the groundtruth/target images and must be divisible by 32.")
    parser.add_argument('-use_fixed_train_size', action='store_true', help="Specify if train_size should be dependent on input_width and input_height or fixed.")  
    # settings for output
    parser.add_argument('-output_results',         type=str, choices=['True', 'False'], help="Specify if rendered output images are saved.")
    parser.add_argument('-write_output_nth_epoch', type=int, help="Specify how often rendered output images are saved.")
    # settings for optimizer
    parser.add_argument('-epochs',          type=int, help="Specify the numer of epochs to be trained.")
    parser.add_argument('-learning_rate',   type=float, help="Start Learning rate of the optimizer.")
    parser.add_argument('-loss_function',   type=str, choices=['INOVIS','MSE', 'L1', 'VGG', 'SSIM', 'XIAO', 'Spectral'], help="Type of loss function.")
    parser.add_argument('-lod_description', type=str, help="DEPRECATED. 'lod0' is used to reference the folders. this matches the C++ application.")
    #---------------------------------------------------------------
    # network parameters 
    parser.add_argument('--in_channels_current',       type=int, nargs=4, help="Number of channels the current frame input contains.")
    parser.add_argument('--in_gt_frame_amount',        type=int, help="Number of ground truth frames feeded to the network.")
    parser.add_argument('--in_channels_gt',            type=int, help="Number of channels the previous frames input contains.")
    parser.add_argument('--feature_scale',             type=int, help="Scale factor to modify the filter sizes: 4 corresponds to 16,32,64,128,256.")
    parser.add_argument('--warp_rgbd',                 type=str, choices=['True', 'False'], help="Specify if rgbd of gt should be warped and appended to the reweighting network.")
    parser.add_argument('--output_warped',             type=str, choices=['True', 'False'], help="Specify if warped images should be written to disk.")
    parser.add_argument('--mask_warped',               type=str, choices=['True', 'False'], help="Specify if warped image should be masked by multiplying with 3rd channel of the movec.")
    parser.add_argument('--extract_lowres_features',   type=str, choices=['True', 'False'], help="Specify if features should be extracted from lower res point renderings.")
    parser.add_argument('--conv_block_extraction_pr',     type=str, choices=['basic', 'gated'], help="Specify the convolution blocks for feature extraction: point rendering.")
    parser.add_argument('--conv_block_extraction_gt',     type=str, choices=['basic', 'gated'], help="Specify the convolution blocks for feature extraction: auxiliary images.")
    parser.add_argument('--conv_block_reweighting',    type=str, choices=['basic', 'gated'], help="Specify the convolution blocks for feature reweighting.")
    parser.add_argument('--conv_block_reconstruction', type=str, choices=['basic', 'gated'], help="Specify the convolution blocks for feature reconstruction.")
    parser.add_argument('--upsample_mode',             type=str, choices=['deconv', 'bilinear'], help="Specify which upsampling type is used.")
    parser.add_argument('--pooling_mode',              type=str, choices=['avg', 'max'], help="Specify which pooling type is used.")
    parser.add_argument('--reweight_with_low_res',     type=str, choices=['True', 'False'], help="Specify if the reweighting network gets the low resolution point rendering on top.")
    parser.add_argument('--filter_sizes',              type=int, nargs=5, help="Specify filter_sizes of the reconstruction network: e.g. [64, 128, 256, 512, 1024].")
    parser.add_argument('--fe_current_filter_base',    type=int, help="Specify the filter base of the feature extraction network of current frames.")
    parser.add_argument('--fe_gt_filter_base',         type=int, help="Specify the filter base of the feature extraction network of gt images.")
    parser.add_argument('--disable_reweighting',       type=str, choices=['True', 'False'], help="Specify if the reweighting should be disabled.")
    parser.add_argument('--feature_extraction_depth',  type=int, help="DEPRECATED. warping and thus feature extraction on lower resolutions is not supported anymore.")
    args = parser.parse_args()
    ################################################################
    # set config according to arguments
    #---------------------------------------------------------------
    # training arguments
    if args.name:
        params.training_name += "_" + args.name
        params.base_dir_output = params.training_name+'/' 
        params.train_output = params.base_dir_output+'out_train/'
        params.eval_output = params.base_dir_output+'out_eval/'
        params.results_output = params.base_dir_output+'out_results/'
        params.test_output = params.base_dir_output+'out_test/'
        params.checkpoints_output = params.base_dir_output+'out_checkpoints/'
    if args.lod_description:
        params.lod_description = args.lod_description
        print("param: lod_description   val: ", params.lod_description, "   type: ", type(params.lod_description))
    if args.batch_size:
        params.batch_size = args.batch_size
        print("param: batch_size   val: ", params.batch_size, "   type: ", type(params.batch_size))
    if args.dataset_size: 
        params.max_images_per_drawelement = args.dataset_size
        print("param: dataset_size   val: ", params.max_images_per_drawelement, "   type: ", type(params.max_images_per_drawelement))
    if args.loss_function: 
        params.loss_function = args.loss_function
        print("param: loss_function   val: ", params.loss_function, "   type: ", type(params.loss_function))
    if args.set_description: 
        params.set_description = args.set_description
        print("param: set_description   val: ", params.set_description, "   type: ", type(params.set_description))
    if args.use_fixed_train_size: 
        #params.save_checkpoint_frequency = args.save_checkpoint_frequency
        print("param: use_fixed_train_size   val: ", args.use_fixed_train_size, "   type: ", type(args.use_fixed_train_size))
    if args.input_width: 
        params.input_image_width_and_height[0] = args.input_width
        print("\tinput_width: ", params.input_image_width_and_height[0])
        if not args.use_fixed_train_size:
            params.train_input_image_width_and_height[0] = int(args.input_width / 2)
            print("\ttrain_width: ", params.train_input_image_width_and_height[0])
        else:
            print("\tfixed train width is used as specified with -use_fixed_train_size: ", params.train_input_image_width_and_height[0])
        print("param: input_width   val: ", params.input_image_width_and_height[0], "   type: ", type(params.input_image_width_and_height[0]))
    if args.input_height: 
        params.input_image_width_and_height[1] = args.input_height
        print("\tinput_height: ", params.input_image_width_and_height[1])
        if not args.use_fixed_train_size:
            params.train_input_image_width_and_height[1] = int(args.input_height / 2)
            print("\ttrain_height: ", params.train_input_image_width_and_height[1])
        else:
            print("\tfixed train height is used as specified with -use_fixed_train_size: ", params.train_input_image_width_and_height[1])
        print("param: input_height   val: ", params.input_image_width_and_height[1], "   type: ", type(params.input_image_width_and_height[1]))
    if args.save_checkpoint_frequency: 
        params.save_checkpoint_frequency = args.save_checkpoint_frequency
        print("param: save_checkpoint_frequency   val: ", params.save_checkpoint_frequency, "   type: ", type(params.save_checkpoint_frequency))
    if args.save_checkpoint_best_epoch_frequency: 
        params.save_checkpoint_best_epoch_frequency = args.save_checkpoint_best_epoch_frequency
        print("param: save_checkpoint_best_epoch_frequency   val: ", params.save_checkpoint_best_epoch_frequency, "   type: ", type(params.save_checkpoint_best_epoch_frequency))
    if args.checkpoint_load_dir: 
        params.checkpoint_load_dir = args.checkpoint_load_dir
        print("param: checkpoint_load_dir   val: ", params.checkpoint_load_dir, "   type: ", type(params.checkpoint_load_dir))
    if args.restore_dataloader: 
        params.restore_dataloader = getBoolFromStr(args.restore_dataloader)
        print("param: restore_dataloader   val: ", params.restore_dataloader, "   type: ", type(params.restore_dataloader))
    if args.write_output_nth_epoch: 
        params.write_output_nth_epoch = args.write_output_nth_epoch
        print("param: write_output_nth_epoch   val: ", params.write_output_nth_epoch, "   type: ", type(params.write_output_nth_epoch))
    if args.output_results: 
        params.output_results = getBoolFromStr(args.output_results)
        print("param: output_results   val: ", params.output_results, "   type: ", type(params.output_results))
    if args.test_indices: 
        params.test_indices = args.test_indices
        print("param: test_indices   val: ", params.test_indices, "   type: ", type(params.test_indices))
    if args.epochs: 
        params.epochs = args.epochs
        print("param: epochs   val: ", params.epochs, "   type: ", type(params.epochs))
    if args.learning_rate: 
        params.learning_rate = args.learning_rate
        print("param: learning_rate   val: ", params.learning_rate, "   type: ", type(params.learning_rate))
    #---------------------------------------------------------------
    # network parameters
    if args.in_channels_current: 
        params.network_args['in_channels_current'] = args.in_channels_current
        print("param: in_channels_current   val: ", params.network_args['in_channels_current'], "   type: ", type(params.network_args['in_channels_current']))
    if args.in_gt_frame_amount: 
        params.network_args['in_gt_frame_amount'] = args.in_gt_frame_amount
        print("param: in_gt_frame_amount   val: ", params.network_args['in_gt_frame_amount'], "   type: ", type(params.network_args['in_gt_frame_amount']))
    if args.in_channels_gt: 
        params.network_args['in_channels_gt'] = args.in_channels_gt
        print("param: in_channels_gt   val: ", params.network_args['in_channels_gt'], "   type: ", type(params.network_args['in_channels_gt']))
    if args.feature_scale: 
        params.network_args['feature_scale'] = args.feature_scale
        print("param: feature_scale   val: ", params.network_args['feature_scale'], "   type: ", type(params.network_args['feature_scale']))
    if args.feature_extraction_depth: 
        params.network_args['feature_extraction_depth'] = args.feature_extraction_depth
        print("param: feature_extraction_depth   val: ", params.network_args['feature_extraction_depth'], "   type: ", type(params.network_args['feature_extraction_depth']))
    if args.warp_rgbd: 
        params.network_args['warp_rgbd'] = getBoolFromStr(args.warp_rgbd)
        print("param: warp_rgbd   val: ", params.network_args['warp_rgbd'], "   type: ", type(params.network_args['warp_rgbd']))
    if args.output_warped: 
        params.network_args['output_warped'] = getBoolFromStr(args.output_warped)
        print("param: output_warped   val: ", params.network_args['output_warped'], "   type: ", type(params.network_args['output_warped']))
    if args.mask_warped: 
        params.network_args['mask_warped'] = getBoolFromStr(args.mask_warped)
        print("param: mask_warped   val: ", params.network_args['mask_warped'], "   type: ", type(params.network_args['mask_warped']))
    if args.extract_lowres_features: 
        params.network_args['extract_lowres_features'] = getBoolFromStr(args.extract_lowres_features)
        print("param: extract_lowres_features   val: ", params.network_args['extract_lowres_features'], "   type: ", type(params.network_args['extract_lowres_features']))
    if args.conv_block_extraction_pr: 
        params.network_args['conv_block_extraction_pr'] = args.conv_block_extraction_pr
        print("param: conv_block_extraction_pr   val: ", params.network_args['conv_block_extraction_pr'], "   type: ", type(params.network_args['conv_block_extraction_pr']))
    if args.conv_block_extraction_gt: 
        params.network_args['conv_block_extraction_gt'] = args.conv_block_extraction_gt
        print("param: conv_block_extraction_gt   val: ", params.network_args['conv_block_extraction_gt'], "   type: ", type(params.network_args['conv_block_extraction_gt']))
    if args.conv_block_reweighting: 
        params.network_args['conv_block_reweighting'] = args.conv_block_reweighting
        print("param: conv_block_reweighting   val: ", params.network_args['conv_block_reweighting'], "   type: ", type(params.network_args['conv_block_reweighting']))
    if args.conv_block_reconstruction: 
        params.network_args['conv_block_reconstruction'] = args.conv_block_reconstruction
        print("param: conv_block_reconstruction   val: ", params.network_args['conv_block_reconstruction'], "   type: ", type(params.network_args['conv_block_reconstruction']))
    if args.upsample_mode: 
        params.network_args['upsample_mode'] = args.upsample_mode
        print("param: upsample_mode   val: ", params.network_args['upsample_mode'], "   type: ", type(params.network_args['upsample_mode']))
    if args.pooling_mode: 
        params.network_args['pooling_mode'] = args.pooling_mode
        print("param: pooling_mode   val: ", params.network_args['pooling_mode'], "   type: ", type(params.network_args['pooling_mode']))
    if args.reweight_with_low_res: 
        params.network_args['reweight_with_low_res'] = getBoolFromStr(args.reweight_with_low_res)
        print("param: reweight_with_low_res   val: ", params.network_args['reweight_with_low_res'], "   type: ", type(params.network_args['reweight_with_low_res']))
    if args.filter_sizes: 
        params.network_args['filter_sizes'] = args.filter_sizes
        print("param: filter_sizes   val: ", params.network_args['filter_sizes'], "   type: ", type(params.network_args['filter_sizes']))
    if args.fe_current_filter_base: 
        params.network_args['fe_current_filter_base'] = args.fe_current_filter_base
        print("param: fe_current_filter_base   val: ", params.network_args['fe_current_filter_base'], "   type: ", type(params.network_args['fe_current_filter_base']))
    if args.fe_gt_filter_base: 
        params.network_args['fe_gt_filter_base'] = args.fe_gt_filter_base
        print("param: fe_gt_filter_base   val: ", params.network_args['fe_gt_filter_base'], "   type: ", type(params.network_args['fe_gt_filter_base']))
    if args.disable_reweighting: 
        params.network_args['disable_reweighting'] = getBoolFromStr(args.disable_reweighting)
        print("param: disable_reweighting   val: ", params.network_args['disable_reweighting'], "   type: ", type(params.network_args['disable_reweighting']))
    #---------------------------------------------------------------
    # log final args and params
    print(args)
    print(params)
    return params
    #---------------------------------------------------------------

################################################################
if __name__ == '__main__':
    params = parseArgs()
    main(params)
    #---------------------------------------------------------------