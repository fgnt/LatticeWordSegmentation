#!/bin/sh

cd tools
./install.sh
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
