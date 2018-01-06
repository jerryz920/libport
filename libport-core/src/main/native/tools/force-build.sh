

for n in Debug Release; do
mkdir -p build.$n
cd build.$n
cmake .. -DCMAKE_BUILD_TYPE=$n
make -j
cd ..
done

