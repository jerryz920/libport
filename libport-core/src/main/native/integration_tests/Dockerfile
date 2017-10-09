
from ubuntu:trusty

#COPY test_integration /opt/
COPY install_lib.sh /opt/

RUN sudo apt-get update -y
RUN cd /opt/ && bash install_lib.sh
RUN apt-get install -y gdb strace vim
RUN apt-get install -y rsyslog
RUN cd /opt/net/casablanca/ && git pull origin dev && cd Release/build.release && make -j 5 install
COPY setup_java.sh /opt/
RUN cd /opt/ && bash setup_java.sh
COPY 50-default.conf /etc/rsyslog.d/
RUN update-rc.d rsyslog defaults
COPY test_core /opt/
COPY test_integration /opt/
COPY *.jar /opt/
COPY libport.so /usr/lib/
COPY latte_exec /usr/bin/
RUN mkdir /data
# only for debugging purpose
RUN mkdir -p /openstack/git/libport/libport-core/src/main/native/
RUN mkdir -p /openstack/git/libport/libport-core/src/main/native/include/
COPY *.cc /openstack/git/libport/libport-core/src/main/native/
COPY *.h /openstack/git/libport/libport-core/src/main/native/include/
COPY config-root.json /data/
WORKDIR /opt/
CMD /opt/test_integration

