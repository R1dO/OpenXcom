script_dir=$(dirname $(readlink -f $0))
repo_dir=$(dirname $(dirname $script_dir))

mkdir -p ${repo_dir}/build
cd ${repo_dir}/build

# Config + build
cmake -G "Unix Makefiles" \
      -DCMAKE_INSTALL_PREFIX:PATH=/data/r1d0/opt/openXcom \
      -DCMAKE_BUILD_TYPE:STRING=Release \
      -DCMAKE_INSTALL_RPATH:STRING=/usr/lib64:/data/r1d0/usr/local/lib:/data/r1d0/opt/anaconda3/lib: \
      -DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=TRUE \
      -DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=TRUE \
      ..
