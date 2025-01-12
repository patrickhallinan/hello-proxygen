# README
  
Tested on ubuntu 24.04 and Rockly Linux 9 (red hat clone)


## get repo
```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```


## Rocky Linux 9 (red hat clone)

```bash
script/rocky-linux-9-deps.sh
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
