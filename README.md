# virt86-demos

A set of demo applications using [virt86](https://github.com/StrikerX3/virt86).
Each application has its own README.md file describing its purpose.

## Building

virt86-demos can be built with [CMake](https://cmake.org/) 3.8.0 or later.

You will need to install virt86 before compiling these demos. Follow the instructions on that project's [README.md](https://github.com/StrikerX3/virt86/blob/master/README.md).

### Building on Windows with Visual Studio 2017

To make a Visual Studio 2017 project you'll need to specify the `"Visual Studio 15 2017"` CMake generator (`-G` command line parameter) and a target architecture (`-A`). Use either `Win32` for a 32-bit build or `x64` for a 64-bit build.

The following commands create and open a Visual Studio 2017 64-bit project:

```cmd
git clone https://github.com/StrikerX3/virt86-demos.git
cd virt86-demos
md build && cd build
cmake -G "Visual Studio 15 2017" -A x64 ..
start virt86-demos.sln
```
The project can be built directly from the command line with CMake, without opening Visual Studio:

```cmd
cd virt86-demos/build
cmake --build . --target ALL_BUILD --config Release -- /nologo /verbosity:minimal /maxcpucount
```

### Building on Linux with GCC 7+

Make sure you have at least GCC 7, `make` and CMake 3.8.0 installed on your system.

```bash
git clone https://github.com/StrikerX3/virt86-demos.git
cd virt86-demos
mkdir build; cd build
cmake ..
make
```

## Support

You can support [the author](https://github.com/StrikerX3) on [Patreon](https://www.patreon.com/StrikerX3).
