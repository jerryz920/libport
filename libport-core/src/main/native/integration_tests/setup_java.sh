sudo apt-get install -y software-properties-common
sudo apt-add-repository -y ppa:webupd8team/java
sudo apt-get update
echo "oracle-java8-installer shared/accepted-oracle-license-v1-1 select true" | sudo debconf-set-selections
sudo apt-get install -y oracle-java8-installer
export JAVA_HOME=/usr/lib/jvm/java-8-oracle
sudo echo "export JAVA_HOME=/usr/lib/jvm/java-8-oracle" >> /etc/profile
sudo apt-get install -y maven
sudo mkdir -p /opt/app/
cd /opt/app/
sudo wget http://supergsego.com/apache/maven/maven-3/3.5.0/binaries/apache-maven-3.5.0-bin.tar.gz
tar xvf apache-maven-3.5.0-bin.tar.gz
echo 'export PATH=/openstack/app/apache-maven-3.5.0/bin:$PATH' >> /etc/profile


