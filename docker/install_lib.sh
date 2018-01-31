
apt-get install -y cmake
apt-get install -y cmake3
apt-get install -y build-essential make clang cscope autoconf
apt-get install -y python-dev libpython-dev
apt-get install -y vim git curl wget
apt-get install -y python-dev libpython-dev build-essential make
apt-get install -y python-pip python-jedi python-virtualenv
apt-get install -y bmon strace gdb valgrind faketime 
apt-get install -y libcrypto++-dev maven
apt-get install -y libtool
apt-get install -y libboost-all-dev libssl-dev cmake3 git g++
mkdir -p net
cd net
git clone -b dev https://github.com/jerryz920/cpprestsdk.git casablanca
cd casablanca/Release
mkdir -p build.release && cd build.release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 4 # be gentle...
make install

