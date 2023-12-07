# Networks
***Place network traces here to load them into the `C++` application.***

Each network consists of two files. During training, these files are created and placed in the runs checkpoint folder (`neural-point-rendering-training -> <run_folder> -> out_checkpoints`):
* `.pt`-file: the trace that is loaded with libtorch.
* `.txt`-file: meta information on the trained network.

Make sure that both files are named the same e.g. `network_trace.pt` and `network_trace.txt` and place the networks name in the `.txt`-files `name` field.

The `preInitMV` field in the `.txt`-file sets whether warping vectors are pre initialized with a fallback environment handling when the network is loaded (Set this flag acordingly. This can be adjusted during runtime as well.).

The networks placed in this folder will automatically be loaded on startup.

## Examples

* `FinetunedOnKitti_scene1-3_PretrainedOnOffice_preInitMV_epoch101`: trained on the Office scene mentioned in the [paper](https://reality.tf.fau.de/publications/2023/harrerfranke2023inovis/harrerfranke2023inovis.pdf) and finetuned on parts of the first 3 subscenes of the Kitti. This is the network used in the results section and the [video](https://reality.tf.fau.de/publications/2023/harrerfranke2023inovis/harrerfranke2023inovis.mp4) for inference on our custom datasets without scene specific training or finetuning. 
* `Office_epoch419`: trained on the Office scene mentioned in the [paper](https://reality.tf.fau.de/publications/2023/harrerfranke2023inovis/harrerfranke2023inovis.pdf). Since this is an indoor dataset, the fallback option during warping is deactivated by default.