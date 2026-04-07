This is the code for Spring 2026 ECE 445/555 project T24. It implements a simple hardware accelerated raytracer for spheres using Nvidia's CUDA and Optix libraries. The raytracer has support for light sources, reflection, refraction, transparency, and Lambertian diffuse reflection. The scene can be modified in real time via a GUI. Requires an RTX enabled Nvidia GPU.

 Ingo Wald's SIGGRAPH optix course used as a starting point https://github.com/ingowald/optix7course

## Conrols
- W - Move forward
- S - Move backward
- A - Move left
- D - Move right
- Space - Move up
- Shift - Move down
- F - Change scene

## Compilation
I have the following at the end of my path:
```
export PATH=/usr/local/cuda/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
export PATH=$PATH:/usr/local/cuda/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
export OptiX_ROOT_DIR_91=/home/mae/sdk/NVIDIA-OptiX-SDK-9.1.0-linux64-x86_64
export OptiX_ROOT_DIR_77=/home/mae/sdk/NVIDIA-OptiX-SDK-7.7.0-linux64-x86_64
```

This project requires version 7.7 of the Optix SDK. Newer versions may work, but they have not been tested. It can be installed from:
```
https://developer.nvidia.com/designworks/optix/downloads/legacy
```

INSTALL GLFW (window API)
```
sudo apt install libglfw3-dev cmake-curses-gui
```

BUILD
```
git pull
git submodule update --init --recursive
mkdir build
cd build
cmake .. -DOptiX_ROOT_DIR=$OptiX_ROOT_DIR_77
make
spherePathTracer_SBTData
```
