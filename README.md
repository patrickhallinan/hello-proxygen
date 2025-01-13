# README
  
Tested on ubuntu 24.04 and Rockly Linux 9 (red hat clone)


## get repo
```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```


## install dependencies

### Rocky Linux 9 (red hat clone)

The proxygen build script `build.sh` installs dependencies for Linux with `apt` so it doesn't work for Rocky Linux.

```bash
script/rocky-linux-9-deps.sh
```

### Ubuntu 24.04

Linking fails on Rocky Linux unless `bz2` is added to the `target_link_libraries` in `cmake`.  When this is added we have to install `libbz2-dev` on `ubuntu` or it will fail to link.

```bash
sudo apt install clang libbz2-dev
```


## build proxygen

**From project root**

```bash
cd proxygen/proxygen && ./build.sh
```

With 16GB of RAM got this error and had to run `./build.sh` again.

```bash
c++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
```

With 20GB proxygen fully built first time.


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
