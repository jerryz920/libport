
export GOPATH=$HOME/dev
export GOROOT=$HOME/goroot
export PATH=$PATH:$HOME/bin:$GOPATH/bin:$GOROOT/bin/
mkdir $HOME/dev -p
mkdir $HOME/goroot -p

GO_VERSION=1.9.2
PROTOBUF_VERSION="v3.5.1"
SCALA_VERSION=2.12.4

install_protobuf() {
  go get -u github.com/google/protobuf
  cd $GOPATH/src/github.com/google/protobuf/
  # may want some better way?
  if test -f PROTOBUF_INSTALLED ; then 
    echo protobuf already installed
    return 
  fi
  git checkout -b dev $1
  ./autogen.sh
  ./configure
  make -j 4
  sudo make install
  touch PROTOBUF_INSTALLED
  cd $WORKDIR
}

configure_go()
{
  mkdir -p $HOME/goroot
  wget https://redirector.gvt1.com/edgedl/go/go$GO_VERSION.linux-amd64.tar.gz
  tar xf go$GO_VERSION.linux-amd64.tar.gz -C $HOME/goroot
  mv $HOME/goroot/go/* $HOME/goroot/
  rm -f go$GO_VERSION.linux-amd64.tar.gz
  echo "export GOROOT=$GOROOT" >> ~/.bashrc
  echo "export GOPATH=$GOPATH" >> ~/.bashrc
  echo "export PATH=$PATH" >> ~/.bashrc
  # install go tools
  go get -u github.com/nsf/gocode
  go get -u google.golang.org/grpc
  go get -u github.com/golang/protobuf/protoc-gen-go
  install_protobuf $PROTOBUF_VERSION
  gocode set propose-builtins true
  gocode close # just in case
}

configure_go
