# README

This is a starter project for anyone interested in using [proxygen](https://github.com/facebook/proxygen), Facebook's event driven HTTP framework.

Tested on macOS Sonoma (14.7.1), ubuntu 24.04 (on Windows 11 in a VM and in WSL), Debian 12 Bookworm on a chromebook, and Rockly Linux 9 (red hat clone)


## get repo
```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```


## install dependencies

### mac

```bash
brew install pkg-config
```

### Rocky Linux 9 (red hat clone)

The proxygen build script `build.sh` installs dependencies for Linux with `apt` which doesn't work for Rocky Linux.

```bash
script/rocky-linux-9-deps.sh
```

### Ubuntu 24.04 and Debian 12

```bash
sudo apt install clang
```


## build proxygen

**From project root**

```bash
cd proxygen/proxygen && ./build.sh
```

### Ubuntu

With 16GB of RAM got this error and had to run `./build.sh` again.

```bash
c++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
```

With 20GB proxygen fully built first time.

### Debian on chromebook

Set number of jobs option to 1

```bash
./build.sh -j <number-of-jobs>
```

## build hello proxygen

**From project root**

```bash
mkdir build && cd build && cmake ..
```

```bash
make
```


## run server

```bash
hello/hello
```


## run test

```bash
test/test
```


## test with curl

```bash
curl http://localhost:8080
```

```bash
curl -X POST -d "nacho" http://localhost:8080
```
