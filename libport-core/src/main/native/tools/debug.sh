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
cp dist/libport-java-0.1.0-with-dependencies.jar $current/integration_tests/libport.jar

cd $current/integration_tests
eval $(docker-machine env ${1:-v1})
docker build -t int_test .
docker run -it --name=int_test -e LD_LIBRARY_PATH=/usr/local/lib/ --rm --cap-add SYS_PTRACE --security-opt seccomp=unconfined int_test bash
cd ..

kill -KILL $(cat gostub.pid)


