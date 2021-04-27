# Library installation #

This file describes detailed steps for installing libraries needed for compiling QSP simulators.

*	Operating System: Ubuntu 18.04 LTS
*	Library:
	*	CVODE (SUNDIALS), version: 4.0.1
		*	Solving ODE modules 
	*	boost, version: 1.70.0
		*	Serialization, testing, command line options, RNG, etc.

# SUNDIALS #

## Version 4.0.1 ##

### Get source code ###

**1\. Download is available at**:

https://computing.llnl.gov/projects/sundials/sundials-software

The following files are downloaded (to `~/Downloads/`):

`sundials-4.0.1.tar.gz`

**2\. Uncompress the archive**:
```
$ tar xzf sundials-4.0.1.tar.gz
```
`INSTALL_GUIDE.pdf` contains official installation instructions, and is available in `~/Downloads/sundials-4.0.1/`. The steps in this document follow the instruction in the official guide.

### Prepare for installation ###

**3\. Install cmake if not already availabe**:
```
$ sudo apt install cmake-curses-gui
```

**4\. Create install and build directories**.
```
$ mkdir -p ~/lib/sundials-4.0.1
$ mkdir -p ~/Downloads/sundials-build
$ cd ~/Downloads/sundials-build
```

### Build with CMake ###

**5\. Configuration**
```
$ ccmake ~/Downloads/sundials-4.0.1
```

Press `c` key to enter configuration interface

Set install directory: `CMAKE_INSTALL_PREFIX` set to `~/lib/sundials-4.0.1`

Set example install directory: `EXAMPLE_INSTALL_PATH` set to `~/lib/sundials-4.0.1/examples`

Press `c` repeatedly to process configuration; press `g` to generate Makefile and exit configuration.

**6\. Build**

From `~/Downloads/sundials-build/`:
```
$ make
$ make install
```

# boost #

## Version 1.70.0 ##

### Get source code ###

**1\. Source code available at:**

https://www.boost.org/users/history/version_1_70_0.html

The following files are downloaded (to `~/Downloads/`):

`boost_1_70_0.tar.gz`

**2\. Uncompress the archive**:
```
$ tar xzf boost_1_70_0.tar.gz
```
Official instructions is available at:

https://www.boost.org/doc/libs/1_70_0/more/getting_started/unix-variants.html

### Building and installation ###

**3\. Building separately-compiled boost libraries**

Some of the boost libraries we use (e.g. Serialization, ProgramOptions, etc.) must be built separately.
```
$ cd ~/Downloads/boost_1_70_0
$ ./bootstrap.sh --prefix=$HOME/lib/boost_1_70_0
$ ./b2 install
```

