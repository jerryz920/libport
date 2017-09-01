
if [ $# -ne 1 ]; then
  echo "Must provide the stub server location"
  echo $@
  exit 1
fi

go build $1/metadata_stub.go
./metadata_stub &
p=$!
sleep 1
./test_metadata
if [ $? ]; then
  echo passed
else
  exit 2
fi
kill $p
