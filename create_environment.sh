#!/bin/bash

#git submodule update --init --recursive --jobs 0


echo "################################################################"
echo "### Create Environment: inovis\n"

source $(conda info --base)/etc/profile.d/conda.sh

conda update -y -n base -c defaults conda

conda create -y -n inovis python=3.9.12
conda activate inovis

echo "################################################################"
echo "### install cudnn and cudatoolkit\n"
# cuda
conda install -y cudnn=8.3.2.44 cudatoolkit-dev=11.6 cudatoolkit=11.6.0=hecad31d_11 -c conda-forge -c nvidia
echo "################################################################"
echo "### install pytorch\n"
# pytorch
conda install -y pytorch==1.12.0 torchvision==0.13.0 torchaudio==0.12.0 cudatoolkit=11.6 -c pytorch -c conda-forge
echo "### ################################################################"
echo "install dependencies with conda\n"
# conda stuff
conda install -y pillow=9.0.1 numpy==1.23.5 tqdm=4.64.0 tensorboard=2.0.0 matplotlib=3.4.3 h5py=3.6.0 dill=0.3.4 libtiff=4.2.0
echo "################################################################"
echo "### install remaining dependencies with pip\n"
# pip stuff
pip install torchsummary==1.5.1 libtiff==0.4.2 imgaug==0.4.0

#conda install libtiff
#conda install libtiff=4.2.0
#pip install libtiff

#pip install imgaug
#conda install dill=0.3.4