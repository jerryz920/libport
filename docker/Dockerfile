
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
RUN cd /opt && git clone https://github.com/jerryz920/libport libport && \
	cd libport/libport-core/src/main/native/ && bash build.sh && \
	cd build.Release && make install
RUN cd /opt/libport && mvn install && \
	mkdir -p /opt/java/ && cp dist/libport-java-0.1.0-with-dependencies.jar /opt/java

# continue setup Spark from here



CMD /bin/bash

