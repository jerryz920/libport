
from ubuntu:trusty

#COPY test_integration /opt/
COPY install_lib.sh /opt/

RUN sudo apt-get update -y
RUN cd /opt/ && bash install_lib.sh
RUN apt-get install -y gdb strace vim
RUN apt-get install -y rsyslog
COPY setup_java.sh /opt/
RUN cd /opt/ && bash setup_java.sh
COPY 50-default.conf /etc/rsyslog.d/
RUN update-rc.d rsyslog defaults
COPY latte_exec /usr/bin/
WORKDIR /opt/

# install libport
COPY install_proto.sh /opt/
RUN cd /opt/ && bash install_proto.sh

RUN cd /opt && git clone https://github.com/jerryz920/utils utils && \
	cd utils/library && bash install_lib.sh
RUN cd /opt && git clone https://github.com/jerryz920/libport libport && \
	cd libport/libport-core/src/main/native/ && LD_LIBRARY_PATH=/usr/local/lib/ bash build.sh && \
	cd build.Release && make install && ln -s /usr/lib/liblatte.so /usr/lib/libport.so
RUN cd /opt/libport && mvn install && \
	mkdir -p /opt/java/ && cp dist/libport-java-0.1.0-with-dependencies.jar /opt/java/libport.jar
COPY rerun.sh /opt

# continue setup Spark from here



CMD /bin/bash

