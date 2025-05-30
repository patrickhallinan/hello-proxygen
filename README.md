# README

This is a starter (fixer-upper) project for anyone interested in [proxygen](https://github.com/facebook/proxygen), Facebook's event driven HTTP framework.

Tested on macOS Sonoma (14.7.1), ubuntu 24.04 (on Windows 11 in a VM and in WSL), Debian 12 Bookworm on a chromebook, and Rocky Linux 9 (red hat clone)

## Overview

`hello-proxygen` is a minimal HTTP server built with proxygen, Facebook's event-driven HTTP framework. It supports GET and POST requests, includes feature and performance tests, and serves as a starting point for exploring Proxygen's capabilities. By default, the server responds to requests at `http://localhost:8080` and includes a test server for automated testing.

## Documentation

- [proxygen's curl sample](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/proxygen-curl-sample.adoc)
- [http client](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/http-client.adoc)
- [feature test](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/feature-test.adoc)
- [performance test](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/performance-test.adoc)
- [docker](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/docker.adoc)
- [kubernetes (microk8s)](https://github.com/patrickhallinan/hello-proxygen/blob/master/doc/k8s.adoc)


## get repo

**NOTE:** Do not forget `--recurse-submodules`

```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```


## install dependencies

### mac

install `brew` then

```bash
brew install cmake fast_float pkg-config nlohmann-json
```

### rocky linux 9 (red hat clone)

The proxygen build script `build.sh` installs dependencies for Linux with `apt` which doesn't work for Rocky Linux.

```bash
script/rocky-linux-9-deps.sh
```

### ubuntu 24.04 and debian 12

```bash
sudo apt install clang cmake libfast-float-dev nlohmann-json3-dev
```


## build proxygen

To build with `clang` set `clang` as your compiler by exporting environment variables before running the build script:

**From project root**

```bash
cd proxygen/proxygen && ./build.sh
```

Running `build.sh` uses a lot of memory.  On ubuntu 24.04 the build failed the first time with 16 GB of memory and the build script had to be run multiple times.  It succeeded building the first time on ubuntu 24.04 with 20 GB.

If memory is low and cannot be increased the number of jobs can be reduced like this:

```bash
./build.sh -j <number-of-jobs>
```

It built fine on a chromebook having 8 GB of memory with the number of jobs set to 1.

### for rtags support on ubuntu

In order to use `rtags` on `proxygen` source (.cpp files) you must build with `clang` and generate `compile_commands.json`.

#### install `bear`

```
sudo apt-get install bear
```

#### build `proxygen` using `clang` and `bear`

```
export CC=clang CXX=clang++
```

```
bear -- ./build.sh
```


## build hello-proxygen

**From project root**

```bash
mkdir build && cd build && cmake ..
```

```bash
make
```


## run and test

With the exception of `curl` everything is run from the `build` directory.

### run server

```bash
hello/hello
```


### run feature test

```bash
test/ft
```


### run performance test

```bash
test/perf
```


### test with curl

```bash
curl http://localhost:8080
```

```bash
curl -X POST -d "echo" http://localhost:8080
```


### test server

Created to simplify automated testing.

**TODO**: Implement automated testing

#### run the server

```bash
test/test
```

options:

```
-test_server_host (IP address) type: string default: "0.0.0.0"
-test_server_port (HTTP port) type: int32 default: 8000
```

#### api help

```
curl  http://localhost:8000
```

#### feature test

```
curl -X POST http://localhost:8000/feature-test
```

OR

```
curl -d '{"host":"127.0.0.1", "port":8080}' http://localhost:8000/feature-test
```


#### performance test

```
curl -X POST http://localhost:8000/performance-test
```
