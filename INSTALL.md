Build and install instructions
==============================

To install Otter Browser from source you need to have the [Qt SDK](https://www.qt.io/download-open-source/) (5.4.0 or newer) installed. You can use system-wide libraries when available, but you will need to install header files for modules specified in *CMakeLists.txt* (listed in the line starting with `qt5_use_modules`). You might also need to install additional header files for GStreamer and libxml2.

Sometimes you may need to manually add QtConcurrent to the list of required modules in *CMakeLists.txt*, by adding it to the line starting with `qt5_use_modules(otter-browser`, resulting in `qt5_use_modules(otter-browser Concurrent`.

CMake (3.1.0 or newer) is used for the build system. You need to perform these steps to get clean build (last step is optional):

    mkdir build
    cd build
    cmake ../
    make
    make install

Alternatively you can use either Qt Creator to compile sources or export native project files using CMake generators. You can also use CPack to create packages.

To make a portable version of Otter, create a file named *arguments.txt* with this line:
`--portable`. Place this file in the directory containing the main Otter executable (the file with name starting with `otter-browser`).
