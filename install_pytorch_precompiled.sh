#!/bin/bash

CONDA_PATH=~/anaconda3/

if test -f "$CONDA_PATH/etc/profile.d/conda.sh"; then
    echo "Found Conda at $CONDA_PATH"
    source "$CONDA_PATH/etc/profile.d/conda.sh"
    conda --version
else
    echo "Could not find conda!"
fi


conda activate inovis


cd neural-point-rendering-cpp/external/thirdparty


#wget https://download.pytorch.org/libtorch/cu117/libtorch-cxx11-abi-shared-with-deps-2.0.0%2Bcu117.zip -O libtorch.zip
wget https://download.pytorch.org/libtorch/cu116/libtorch-cxx11-abi-shared-with-deps-1.12.0%2Bcu116.zip -O libtorch.zip
unzip libtorch.zip -d .


cp -rv libtorch/ $CONDA_PATH/envs/inovis/lib/python3.9/site-packages/torch/
