#!/usr/bin/env bash
args=("$@")

# parse command line arguments
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help)
      echo "Script to install the LatticeWordSegmentation tool."
      echo "option --help: Print this help."
      echo "option --no-boost-install: Do not install boost library."
      echo "         Environment variable BOOST_ROOT needs to be set,"
      echo "         if boost libraries are installed in custom directory."
      echo "option --no-openfst-install: Do not install openfst library."
      echo "         Environment variable OPENFST_DIR needs to be set,"
      echo "         if openfst libraries are installed in custom directory."
      exit 0;;
    -b|--no-boost-install) no_boost_install=true; shift 1;;
    -o|--no-openfst-install) no_openfst_install=true; shift 1;;
    *) echo "unknown parameter $1!" >&2; exit 1;;
  esac
done

# install tools
cd tools
./install.sh "${args[@]}"
cd ..

# install LatticeWordSegmentation
if [ "${no_boost_install}" != "true" ]
then
  LOCAL_BOOST_FLAG="-DBOOST_ROOT=$(pwd)/tools/boost_1_60_0"
fi

if [ "${no_openfst_install}" != "true" ]
then
  LOCAL_OPENFST_FLAG="-DOPENFST_DIR=$(pwd)/tools/openfst-1.5.3"
fi

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src ${LOCAL_BOOST_FLAG} ${LOCAL_OPENFST_FLAG}
make
cd ..

# unpack example files
cd test/WSJCAM0_WSJ0+1_Cross_Lattice/openfst/
tar -xjf WSJCAM0_WSJ0+1_Cross_Phoneme_OpenFST_Lattice_16.tar.bz2

cd ../htk
tar -xjf WSJCAM0_Phoneme_htk_Lattice.tar.bz2

cd ../../WSJCAM0_Lattice/openfst/
tar -xjf WSJCAM0_Phoneme_OpenFST_Lattice_10.tar.bz2
cd ../../../
