script_dir=$(dirname $(readlink -f $0))
repo_dir=$(dirname $(dirname $script_dir))

mkdir -p ${repo_dir}/build.master
cd ${repo_dir}/build.master

# Config + build
cmake -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX:PATH=/usr/local \
      -DCMAKE_BUILD_TYPE:STRING=Release \
      ..
