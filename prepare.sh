
apt-get install -y libboost-all-dev libssl-dev cmake3
mkdir -p net
cd net
git clone -b dev https://github.com/jerryz920/cpprestsdk.git casablanca
cd casablanca/Release
mkdir -p build.release && cd build.release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
make install

go get github.com/sirupsen/logrus
go get github.com/gorilla/mux


