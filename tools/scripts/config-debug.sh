script_dir=$(dirname $(readlink -f $0))
repo_dir=$(dirname $(dirname $script_dir))

mkdir -p ${repo_dir}/build.debug
cd ${repo_dir}/build.debug

# Config + build
cmake -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX:PATH=/usr/local \
      -DCMAKE_BUILD_TYPE:STRING=Debug \
      ..
