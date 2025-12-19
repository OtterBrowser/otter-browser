Build and install instructions
==============================

To install Otter Browser from source you need to have the [Qt SDK](https://www.qt.io/development/download-open-source) (5.15.0 or newer) installed. You can use system-wide libraries when available, but you will need to install header files for modules specified in *CMakeLists.txt* (listed in the line starting with `target_link_libraries`). You might also need to install additional header files for GStreamer and libxml2.

In case of errors mentioning *QtConcurrent*, you may need to ad it to the list of required modules in *CMakeLists.txt*, by adding it to the line starting with `target_link_libraries(otter-browser Qt5::Core`, resulting in `target_link_libraries(otter-browser Qt5::Core Qt5::Concurrent`.

CMake (3.7.0 or newer) is used for the build system. You need to perform these steps to get clean build (last step is optional):

    mkdir build
    cd build
    cmake ../
    make
    make install

Alternatively you can use either Qt Creator to compile sources or export native project files using CMake generators. You can also use CPack to create packages.

To make a portable version of Otter Browser, create a file named *arguments.txt* with this line:

`--portable`

Place this file in the directory containing the main Otter Browser executable (the file with name starting with `otter-browser`).
