#!/bin/bash

# assume we have docker-machine
mname=${1:-v1}
mip=`docker-machine ip $mname`


mkdir -p deplibs
copydep `which attguard` deplibs
if test -f /usr/lib/liblatte.so ; then
  cp /usr/lib/liblatte.so deplibs
else
  cp /usr/local/lib/liblatte.so deplibs
fi
# for compatibility
cp deplibs/liblatte.so deplibs/libport.so
if ! [ $? ] ; then
  echo "fail to copy files, check if liblatte exist"
  exit 2
fi

for lib in `ls deplibs/`; do
  docker-machine scp "deplibs/$lib" $mname:
done
docker-machine scp `which attguard` $mname:
ssh -tt -i /local/data-population/key1 docker@$mip 'sudo mv *.so *.so.* /opt/lib/;'
ssh -tt -i /local/data-population/key1 docker@$mip 'sudo mv attguard /usr/local/bin/;'
ssh -tt -i /local/data-population/key1 docker@$mip '
rm /opt/lib/libdl.so.2
rm /opt/lib/libm.so.6
rm /opt/lib/libc.so.6
rm /opt/lib/libresolv.so.2
rm /opt/lib/libpthread.so.0
'

