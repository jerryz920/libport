sudo apt-get install -y software-properties-common
sudo apt-add-repository -y ppa:webupd8team/java
sudo apt-get update
echo "oracle-java8-installer shared/accepted-oracle-license-v1-1 select true" | sudo debconf-set-selections

cd /var/lib/dpkg/info/
sudo apt-get install -y oracle-java8-installer
sudo sed -i 's|JAVA_VERSION=8u144|JAVA_VERSION=8u152|' oracle-java8-installer.*
sudo sed -i 's|PARTNER_URL=http://download.oracle.com/otn-pub/java/jdk/8u144-b01/090f390dda5b47b9b721c7dfaa008135/|PARTNER_URL=http://download.oracle.com/otn-pub/java/jdk/8u152-b16/aa0333dd3019491ca4f6ddbe78cdb6d0/|' oracle-java8-installer.*
sudo sed -i 's|SHA256SUM_TGZ="e8a341ce566f32c3d06f6d0f0eeea9a0f434f538d22af949ae58bc86f2eeaae4"|SHA256SUM_TGZ="218b3b340c3f6d05d940b817d0270dfe0cfd657a636bad074dcabe0c111961bf"|' oracle-java8-installer.*
sudo sed -i 's|J_DIR=jdk1.8.0_144|J_DIR=jdk1.8.0_152|' oracle-java8-installer.*
sudo apt-get install -y oracle-java8-installer
export JAVA_HOME=/usr/lib/jvm/java-8-oracle
sudo echo "export JAVA_HOME=/usr/lib/jvm/java-8-oracle" >> /etc/profile
sudo apt-get install -y maven
sudo mkdir -p /opt/app/
cd /opt/app/
sudo wget http://supergsego.com/apache/maven/maven-3/3.5.0/binaries/apache-maven-3.5.0-bin.tar.gz
tar xvf apache-maven-3.5.0-bin.tar.gz
echo 'export PATH=/openstack/app/apache-maven-3.5.0/bin:$PATH' >> /etc/profile


