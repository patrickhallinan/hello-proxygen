# README
  
Tested on ubuntu 24.04 and Rockly Linux 9 (red hat clone)

## get repo
```
git clone --recurse-submodules  git@github.com:patrickhallinan/hello-proxygen
```

## build proxygen

**From project root**

```
cd proxygen/proxygen && ./build.sh
```

## build hello proxygen

**From project root**

```
mkdir build && cd build && cmake ..
```

```
make
```

## test with curl

```
curl http://localhost:8080
```

```
curl -X POST -D "nacho" http://localhost:8080
```
