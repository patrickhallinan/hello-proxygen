# dependencies from repos
sudo dnf install -y epel-release
sudo dnf update -y
sudo dnf config-manager --set-enabled crb
sudo dnf install -y cmake gcc gcc-c++ boost boost-devel double-conversion-devel
sudo dnf install -y fast_float-devel libevent-devel openssl-devel glog-devel zlib-devel libsodium-devel gperf
sudo dnf install -y libunwind-devel binutils-devel xz-devel lz4-devel snappy-devel libzstd-devel bzip2-devel
sudo dnf install -y clang json-devel
