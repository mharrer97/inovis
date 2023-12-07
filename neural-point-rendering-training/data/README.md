# `data`-Directory for Training Datasets

***Place any datasets for training in this directory.***
 
Datasets created in the real-time application are stored to this folder. 
Each folder depicts one datasets and contains subfolders with image data.
Each dataset contains the following subfolders:
* ```input_0_lod0```: highest resolution rgb point rendering
* ```input_1_lod0```: 1/2 resolution rgb point rendering
* ```input_2_lod0```: 1/4 resolution rgb point rendering
* ```input_3_lod0```: 1/8 resolution rgb point rendering
* ```depth_0_lod0```: highest resolution depth point rendering. . If no depth images were provided in the original dataset, depth images to supplement original datasets can be created in this folder (see [datset section](../../neural-point-rendering-cpp/)).
* ```depth_1_lod0```: 1/2 resolution depth point rendering
* ```depth_2_lod0```: 1/4 resolution depth point rendering
* ```depth_3_lod0```: 1/8 resolution depth point rendering
* ```nearest1_groundtruth_0```: nearest auxiliary image. 
**NOTE**: that while capturing a dataset, ```Skip Nearest Groundtruth``` should be enabled because otherwise this folder contains the auxiliary images identical to the ```groundtruth``` folder.
* ```nearest2_groundtruth_0```: second nearest auxiliary image
* ```nearest3_groundtruth_0```: third nearest auxiliary image
* ```nearest1_depth_0_lod0```: depth of nearest auxiliary image
* ```nearest2_depth_0_lod0```: depth of second nearest auxiliary image.
* ```nearest3_depth_0_lod0```: depth of third nearest auxiliary image.
* ```nearest1_motion_0_lod0```: motion vector to warp to the nearest auxiliary view.
* ```nearest2_motion_0_lod0```: motion vector to warp to the second nearest auxiliary view.
* ```nearest3_motion_0_lod0```: motion vector to warp to the third nearest auxiliary view.
* ```groundtruth```: the groundtruth image to be reconstructed. Training loss is computed against these images. 
* ```mask```: to mask out invalid regions. Masked regions do not impact any gradients during training.




