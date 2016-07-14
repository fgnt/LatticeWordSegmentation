#!/bin/sh

case "$1" in
  --help|-h)
    echo "Script to install the latticewordsegmentation tool";
    echo "option --no-boost-install: Do not install boost library"
    exit 0;;
  --no-boost-install);;
  "");;
  *)echo "Unknown parameter!"; exit 1;;
esac

cd tools
./install.sh "$1"
cd ..

mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../src
make
cd ..

cd test/WSJCAM0_WSJ0+1_Cross_Lattice/openfst/
tar -xjf WSJCAM0_WSJ0+1_Cross_Phoneme_OpenFST_Lattice_16.tar.bz2
cd ../htk
tar -xjf WSJCAM0_Phoneme_htk_Lattice.tar.bz2
cd ../../../

cd test/WSJCAM0_Lattice/openfst/
tar -xjf WSJCAM0_Phoneme_OpenFST_Lattice_10.tar.bz2
cd ../../../
