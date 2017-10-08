#!/bin/bash


go build metadata_stub.go

if test -f gostub.pid; then
  kill -KILL $(cat gostub.pid)
fi
./metadata_stub >stub.out 2>stub.err &
echo $! > gostub.pid

mkdir -p integration_tests
cd build.Debug
cmake ..
make -j
cp test_core test_integration libport.so libport.a ../integration_tests
cp ../*.cc ../integration_tests
cp ../include/*.h ../integration_tests
cd ..
current=`pwd`
cd ../../../../
mvn package
cp libport-dist/*.jar $current

cd $current/integration_tests
eval $(docker-machine env ${1:-v1})
docker build -t int_test .
docker run -it -e LD_LIBRARY_PATH=/usr/local/lib/ --rm --security-opt seccomp=unconfined int_test
cd ..

kill -KILL $(cat gostub.pid)



