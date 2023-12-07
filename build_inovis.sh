#!/bin/bash

#git submodule update --init --recursive --jobs 0
source $(conda info --base)/etc/profile.d/conda.sh
conda activate inovis

#install gl stuff
#sudo apt-get install make make g++ libx11-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxrandr-dev libxext-dev libxcursor-dev libxinerama-dev libxi-dev

gcc_v=9
export CC=gcc-$gcc_v
export CXX=g++-$gcc_v
export CUDAHOSTCXX=g++-$gcc_v
echo "Using g++-$gcc_v"


echo "############################################################"
echo "Compiler Versions"
echo "------------------------------------------------------------"

echo $CC
gcc-$gcc_v --version
echo $CXX
g++-$gcc_v --version

nvcc --version
echo "############################################################"

# make sure to have the other dependencies installed
# sudo apt-get install cmake make g++ libx11-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxrandr-dev libxext-dev libxcursor-dev libxinerama-dev libxi-dev libglew-dev libglfw3-dev

unset CUDA_HOME

cd neural-point-rendering-cpp
#rm -rf build
mkdir build
cd build
export CONDA=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}

export LIBTORCH=$CONDA_PATH/envs/inovis/lib/python3.9/site-packages/torch/
echo $CONDA

cmake -DCMAKE_PREFIX_PATH="${CONDA}/lib/python3.9/site-packages/torch/;${CONDA}" -DCONDA_PATH=$CONDA .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

make -j10
