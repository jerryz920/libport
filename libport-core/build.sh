

build_script_path=`readlink -f $0`
build_script_dir=`dirname $build_script_path`

cd $build_script_dir/src/main/native/
bash build.sh
cp build.Debug/client/liblatte.so $build_script_dir/../dist/libport-debug.so
cp build.Release/client/liblatte.so $build_script_dir/../dist/libport-release.so
cd $build_script_dir/../dist
rm libport.so liblatte.so
ln -s libport-release.so libport.so
ln -s libport-release.so liblatte.so
