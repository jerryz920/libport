
for n in /openstack/git/libport/libport-core/src/main/native/build.Debug/server/liblatte.so \
  /usr/local/lib/libprotobuf.so.15 \
 /lib/x86_64-linux-gnu/libpthread.so.0 \
 /usr/lib/x86_64-linux-gnu/libstdc++.so.6 \
 /lib/x86_64-linux-gnu/libgcc_s.so.1 \
 /usr/lib/x86_64-linux-gnu/libboost_system.so.1.54.0 \
 /lib/x86_64-linux-gnu/libcrypto.so.1.0.0 \
 /lib/x86_64-linux-gnu/libssl.so.1.0.0 \
 /lib/x86_64-linux-gnu/libm.so.6 \
 /usr/local/lib/libcpprest.so.2.9 \
 /lib/x86_64-linux-gnu/libdl.so.2  ; do
 scp -i /local/data-population/key1 $n docker@${1:-v1}:
 done



