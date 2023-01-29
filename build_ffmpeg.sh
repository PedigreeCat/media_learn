# !/bin/bash

set -x
echo $1

if [[ '$1' == '' ]]
then
    echo -e "\e[31m"
    echo -e "usage \n"
    echo -e "    $0 <ffmpeg_dir_path>"
    echo -e "\e[0m"
    exit -1
fi

ffmpeg_dir=$1
install_dir=$(pwd)/dependence

ARGS=(
	--prefix=$install_dir
	--arch=x86_64
	--target-os=mingw64
	--disable-debug
	--enable-shared
	--disable-static
)

cd $ffmpeg_dir
./configure ${ARGS[@]}
make -j 4
make install