


wget --no-check-certificate -c --header "Cookie: oraclelicense=accept-securebackup-cookie" http://download.oracle.com/otn-pub/java/jdk/8u152-b16/aa0333dd3019491ca4f6ddbe78cdb6d0/jdk-8u152-linux-x64.rpm
rpm -ivh jdk-8u152-linux-x64.rpm


export JAVA_HOME=/usr/java/latest/
echo "export JAVA_HOME=/usr/java/latest" >> /etc/profile
yum -y install maven

alternatives --install /usr/bin/java java /usr/java/latest/jre/bin/java 200000
## javaws ##
alternatives --install /usr/bin/javaws javaws /usr/java/latest/jre/bin/javaws 200000

## Install javac only if you installed JDK (Java Development Kit) package ##
alternatives --install /usr/bin/javac javac /usr/java/latest/bin/javac 200000
alternatives --install /usr/bin/jar jar /usr/java/latest/bin/jar 200000

echo 1 | alternatives --config java

