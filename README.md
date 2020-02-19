# libsbox
[![Build Status](https://travis-ci.org/Forestryks/libsbox.svg?branch=master)](https://travis-ci.org/Forestryks/libsbox)


Libsbox is sandbox desined for evaluating submissions in programming contests.

## Why libsbox?

 - Performance. Libsbox need only around 2ms to start sandbox, which is close to simple exec()
 - Ease of use. You can include `libsbox.h` to easily run anything in sandbox.

## Getting started

These instructions will guide you through the installation process of libsbox.

### Prerequisites
 - C++17 compiler, especially `std::filesystem` support
 - CMake version 3.10 or higher
 - linux kernel version 5 or higher (may also work with late 4.* versions)
 - cgroup v1 heirarchy mounted in /sys/fs/cgroup

### Installing

1. Build sources
 ```bash
 mkdir build
 cd build
 cmake ..
 make
 ```

2. Install libsbox
 ```bash
 sudo make install
 ```
 Following will be installed:
  - `/usr/include/libsbox.h` - libsbox C++ header
  - `/usr/lib/libsbox.a` - static library
  - `/usr/lib/libsbox.so` - dynamic library
  - `/usr/bin/libsboxd` - libsbox daemon binary
  - `/etc/libsbox/conf.json` - libsbox configuration file
  - invokers group created

[comment]: # (3. To allow users use libsbox add them to `invokers` group TODO
 ```bask
 sudo usermod -aG invokers username
 ```)

3. Run tests
 ```bash
 sudo make bundled_tests
 ```
 If you started libsboxd yourself and want to validate that everything is good run `sudo make tests`

## Documentation

Not ready yet

## Authors

 - **Andrei Odintsov** ([@Forestryks](https://github.com/Forestryks))

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

## Acknowledgments
libsbox inspired by [isolate](https://github.com/ioi/isolate/) and created to simplify writing invokers (evaluators) for various task types.
