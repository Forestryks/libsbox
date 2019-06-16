# libsbox
[![Build Status](https://travis-ci.org/Forestryks/libsbox.svg?branch=master)](https://travis-ci.org/Forestryks/libsbox)

libsbox is open source C++ library introduced to be used in programming contests environments. It is simple and provide full functionality needed to evaluate participants' solutions.

## Getting started

These instructions will guide you through the installation process of libsbox.

### Prerequisites

##### To build libsbox you need:
 - GCC or Clang compiler that can compile C++14 source
 - cmake version 3.10 or higher
 
##### To use libsbox you need:
 - linux kernel supporting following features (kernel version 5 or higher is recommended):
    - cgroup filesystem, namely cpuacct and memory controllers
    - oom_kill counter in memory cgroup
    - namespaces, namely mount, ipc, net and pid namespaces
 - cgroup hierarchy mounted in /sys/fs/cgroup

### Installing
Follow the steps bellow to install libsbox on your system

1. Create build directory and change to it
 ```bash
 mkdir build
 cd build
 ```

2. Run Cmake to generate build files
 ```bash
 cmake ..
 ```

3. Compile sources using make
 ```bash
 make
 ```

4. Install library and associated headers
 ```bash
 make install
 ```

## Running the tests

To run tests type
```bash
sudo ctest --output-on-failure
```
or
```bash
sudo CTEST_OUTPUT_ON_FAILURE=1 make test
```

If memory tests fail disable swap
```bash
sudo swapoff -a
```

If tests don't pass check whether your system satisfies prerequisites. If you can't solve the problem yourself, open an issue in [project github](https://github.com/Forestryks/libsbox)

## Documentation

Not ready yet

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/Forestryks/libsbox/tags).

## Authors

 - **Andrei Odintsov** ([@Forestryks](https://github.com/Forestryks))

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

## Acknowledgments
libsbox inspired by [isolate](https://github.com/ioi/isolate/) and created to simplify writing invokers (evaluators) for various task types.
