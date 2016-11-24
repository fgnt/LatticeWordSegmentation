#!/usr/bin/env bash
args=("$@")

# parse command line arguments
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help)
      echo "Script to install requeired libraries for LatticeWordSegmentation tool."
      echo "option --help: Print this help."
      echo "option --no-boost-install: Do not install boost library."
      echo "option --no-openfst-install: Do not install openfst library."
      exit 0;;
    -b|--no-boost-install) no_boost_install=true; shift 1;;
    -o|--no-openfst-install) no_openfst_install=true; shift 1;;

    *) echo "unknown parameter $1!" >&2; exit 1;;
  esac
done

# compile openfst
if [ "${no_openfst_install}" = "true" ]
then
  echo "No installation of openfst. Assume use of system-wide install or set environment variable OPENFST_DIR."
else
  wget http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.5.3.tar.gz
  tar -xzf openfst-1.5.3.tar.gz
  cd openfst-1.5.3
  CXXFLAGS="${CXXFLAGS} -O3" ./configure  --prefix=$(pwd)
  make -j4 install
  cd ..
fi

# install boost
if [ "${no_boost_install}" = "true" ]
then
  echo "No installation of boost. Assume use of system-wide install or set environment variable BOOST_ROOT."
else
  wget http://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.bz2/download -O boost_1_60_0.tar.bz2
  tar -xjf boost_1_60_0.tar.bz2
  cd boost_1_60_0
  ./bootstrap.sh --with-libraries=system,filesystem
  ./b2
  cd ..
fi
