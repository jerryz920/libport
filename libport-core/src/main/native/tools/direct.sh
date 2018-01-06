bash build.sh
for n in /openstack/git/libport/libport-core/src/main/native/build.Debug/libport.so \
  /openstack/git/libport/libport-core/src/main/native/build.Debug/test_integration \
 /lib/x86_64-linux-gnu/libpthread.so.0 \
 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 \
 /lib/x86_64-linux-gnu/libgcc_s.so.1 \
 /usr/lib/x86_64-linux-gnu/libboost_system.so.1.54.0 \
 /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 \
 /lib/x86_64-linux-gnu/libssl.so.1.0.0 \
 /lib/x86_64-linux-gnu/libm.so.6 \
 /usr/local/lib/libcpprest.so.2.9 \
 /lib/x86_64-linux-gnu/libc.so.6 \
 /lib/x86_64-linux-gnu/libdl.so.2  ; do
 docker-machine scp $n ${1:-v1}:
 done

ip=`docker-machine ip ${1:-v1}`

go build metadata_stub.go

if test -f gostub.pid; then
  kill -KILL $(cat gostub.pid)
fi
./metadata_stub >stub.out 2>stub.err &
echo $! > gostub.pid

ssh-keygen -f "/root/.ssh/known_hosts" -R $ip
 ssh -i /local/data-population/key1 docker@$ip

kill -KILL $(cat gostub.pid)
