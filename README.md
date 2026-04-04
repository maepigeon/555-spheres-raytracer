 Ingo Wald's SIGGRAPH optix course used as a starting point https://github.com/ingowald/optix7course

I have the following at the end of my path:
    export PATH=/usr/local/cuda/bin:$PATH
    export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
    export PATH=$PATH:/usr/local/cuda/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64
    export OptiX_ROOT_DIR_91=/home/mae/sdk/NVIDIA-OptiX-SDK-9.1.0-linux64-x86_64
    export OptiX_ROOT_DIR_77=/home/mae/sdk/NVIDIA-OptiX-SDK-7.7.0-linux64-x86_64

Optix SDK 7.X can be installed from: 
    https://developer.nvidia.com/designworks/optix/downloads/legacy (I used 7.7)

INSTALL GLFW (window API)
     sudo apt install libglfw3-dev cmake-curses-gui

BUILD
    mkdir build
    cd build
    cmake .. -DOptiX_ROOT_DIR=$OptiX_ROOT_DIR_77
    make
    spherePathTracer_SBTData
