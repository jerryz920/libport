
if [ $# -ne 1 ]; then
  echo "Must provide the stub server location"
  echo $@
  exit 1
fi

go build $1/metadata_stub.go
sleep 0.5
if test -f $1/gostub.pid; then
  kill -KILL $(cat $1/gostub.pid)
  echo "kill state $?"
fi
./metadata_stub >$1/stub.out 2>$1/stub.err &
echo $! > $1/gostub.pid
sleep 1.0
./test_metadata
if [ $? ]; then
  echo passed
else
  exit 2
fi
kill -KILL $(cat $1/gostub.pid)
echo "kill state $?"
