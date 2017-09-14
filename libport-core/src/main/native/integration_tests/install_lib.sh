
apt-get install -y libboost-all-dev libssl-dev cmake3 git g++
mkdir -p net
cd net
git clone -b dev https://github.com/jerryz920/cpprestsdk.git casablanca
cd casablanca/Release
mkdir -p build.release && cd build.release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j 4 # be gentle...
make install

