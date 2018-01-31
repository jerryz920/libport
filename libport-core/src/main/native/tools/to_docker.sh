#!/bin/bash

# assume we have docker-machine
mname=${1:-v1}

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
ssh -tt -i /local/data-population/key1 docker@$mname 'sudo mv *.so /opt/lib/;'
ssh -tt -i /local/data-population/key1 docker@$mname 'sudo mv attguard /usr/local/bin/;'
