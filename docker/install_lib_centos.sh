#yum -y install libboost-all-dev libssl-dev cmake3 git g++

yum -y install wget
wget http://repo.enetres.net/enetres.repo -O /etc/yum.repos.d/enetres.repo
wget ftp://fr2.rpmfind.net/linux/Mandriva/official/2010.0/x86_64/media/main/release/lib64icu42-4.2.1-1mdv2010.0.x86_64.rpm
rpm -ivh lib64icu42-4.2.1-1mdv2010.0.x86_64.rpm
yum -y install boost-devel openssl-devel gcc-c++ git make

wget https://cmake.org/files/v3.9/cmake-3.9.4.tar.gz
tar xf cmake*.tar.gz
cd cmake-3.9.4
./bootstrap
make -j 8
make install

mkdir -p net
cd net
git clone -b dev https://github.com/jerryz920/cpprestsdk.git casablanca
cd casablanca/Release
mkdir -p build.release && cd build.release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 6 # be gentle...
make install

