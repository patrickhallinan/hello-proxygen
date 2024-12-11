# dependencies from repos
sudo dnf install -y epel-release
sudo dnf update -y
sudo dnf config-manager --set-enabled crb
sudo dnf install -y cmake gcc gcc-c++ boost boost-devel double-conversion-devel
sudo dnf install -y fast_float-devel libevent-devel openssl-devel glog-devel zlib-devel libsodium-devel gperf
sudo dnf install libunwind-devel binutils-devel xz-devel lz4-devel snappy-devel libzstd-devel bzip2-devel


# fmt
git clone https://github.com/fmtlib/fmt.git

cd fmt
git checkout 11.0.2
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .

