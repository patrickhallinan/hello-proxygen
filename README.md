# README

This is a starter project for anyone interested in using [proxygen](https://github.com/facebook/proxygen), Facebook's event driven HTTP framework.

Tested on macOS Sonoma (14.7.1), ubuntu 24.04 (on Windows 11 in a VM and in WSL), Debian 12 Bookworm on a chromebook, and Rockly Linux 9 (red hat clone)


## Documentation

- [proxygen's curl sample](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/proxygen-curl-sample.adoc)
- [http client](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/http-client.adoc)
- [test](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/test.adoc)


## get repo
```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```


## install dependencies

### mac

```bash
brew install pkg-config
```

### rocky linux 9 (red hat clone)

The proxygen build script `build.sh` installs dependencies for Linux with `apt` which doesn't work for Rocky Linux.

```bash
script/rocky-linux-9-deps.sh
```

### ubuntu 24.04 and debian 12

```bash
sudo apt install clang
```


## build proxygen

**From project root**

```bash
cd proxygen/proxygen && ./build.sh
```

### ubuntu

With 16GB of RAM got this error and had to run `./build.sh` again.

```bash
c++: fatal error: Killed signal terminated program cc1plus
compilation terminated.
```

With 20GB proxygen fully built first time.

### debian on chromebook

Set number of jobs option to 1

```bash
./build.sh -j <number-of-jobs>
```

## build hello-proxygen

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
