
(cd /opt/libport/; git pull origin master)
(cd /opt/libport/libport-core/src/main/native/; bash build.sh; cd build.Release; make install)
(cd /opt/libport/; mvn install)
(cd /opt/libport/; cp dist/libport*-dep*.jar ../java/libport.jar)

