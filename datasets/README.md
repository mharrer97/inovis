# Datasets
***Place dataset config files here to load them into the `C++` application.***

Each dataset is described with one file containing meta information about the set:
* `name`: used in the application for identification
* `folder`: path to the datasets root directory (relative from the `neural-point-rendering-cpp/src` directory)
* `type`: type of dataset e.g. `Generic`
* `size`: amount of images
* `width`: target width of the dataset
* `height`: target height of the dataset

The datasets placed in this folder will automatically be choosable on startup. 

You can download the media files for the example dataset [here](https://zenodo.org/records/10286628).
Place the `bagger2`` folder in this folder.
