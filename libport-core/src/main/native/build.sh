

for n in Debug Release; do
  if test -d build.$n; then
    cd build.$n
    cmake .. -DCMAKE_BUILD_TYPE=$n
  else
    mkdir -p build.$n
    cd build.$n
    cmake .. -DCMAKE_BUILD_TYPE=$n
  fi
  make -j
  cd ..
done

