# Inovis Training

The training is implemented in ```Python``` with ```PyTorch```. 

## Datasets
The `data`-folder contains datasets for training. See [here](./data/) for a description of  the structure of training datasets.

Training datasets can be created in the real-time `C++` application and are stored directly to the `data` folder. See [here](../neural-point-rendering-cpp/) for a description of how to create a training dataset. 

## Execution

For executuion, activate the conda environment and execute the ```train.py``` script:
```
conda activate inovis
python train.py run_name
```
```run_name``` is an identifier that is used to set the name of the results folder, which are created as a folder starting with the date, followed by the ```run_name```.\
Parameters can be overwritten as follows:
```
python train.py run_name -param=value -param=value --network_param=value --network_param=value
```

## Config

The file ```config.py``` contains the parameters for training. The following are some of the most relevant:

* ```save_checkpoint_frequency```: frequency a checkpoint is written, regardless of loss comparison etc.
* ```save_checkpoint_best_epoch_frequency```: cooldown for saving a checkpoint due to a new best epoch is found. 
* ```write_output_nth_epoch```: frequency how often ```out_results``` etc. are rendered and stored.
* ```test_indices```: list of indices to exclude from training to use them e.g. for metrics.
* ```epochs```: number of epochs. The best results are achieved in ~ 750 - 950 epochs.
* ```batch_size```: how many samples are drawn per batch. Can be used for memory management.
* ```input_image_width_and_height```: image dimensions of the highest input resolution training images. Can be used for memory management.
* ```train_input_image_width_and_height```: size of the crops for data augmentation.
* ```set_description```: name of the dataset. This must match the folder name in the ```data``` folder. To train with multiple datasets, provide datasets as a list in config.py (`["dataset1", "dataset2"]`) or as command line argument (`-set_description dataset1 dataset2`). **NOTE** that for training with multiple datasets, the target resolution of both datasets must match.
  
The parameters of the network can be adjusted here as well.

Parameters can either set the configuration in the python file or overwrite some of them with command line arguments.

## Results
Training creates folders (folder names beginning with a date) which contain results and checkpoints of trainings. ```out_results``` are renderings of the validation set next to the corresponding ground truth image. Do not use these for metrics, because they are used in training to choose the best checkpoints. Instead load trained networks in the real-time application, enable the ```Skip Nearest Groundtruth``` option and press ```"0"``` to capture a dataset, which also writes out all rendered images in the ```C++``` ```src``` [folder](../neural-point-rendering-cpp/src/).

Error Metrics are stored in `out_runs` and can be viewed with tensorboard. 

## Checkpoints
Checkpoints are stored to the respective runs folder in `out_checkpoints` and contains checkpoints (subfolders) to resume training and traces (´.pt´ files and a ´network.txt´, which contains meta information on how the network is parsed) of the network that can be loaded from the [real-time application](../neural-point-rendering-cpp/).