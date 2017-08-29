
apt-get install -y libboost-all-dev libssl-dev cmake3
mkdir -p net
cd net
git clone https://github.com/Microsoft/cpprestsdk.git casablanca
cd casablanca/Release
mkdir -p build.release && cd build.release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
make install



