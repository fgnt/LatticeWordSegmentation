#!/bin/sh

PREFIX=$(pwd)

wget http://www.openfst.org/twiki/pub/FST/FstDownload/openfst-1.5.0.tar.gz
tar -xzf openfst-1.5.0.tar.gz
cd openfst-1.5.0
./configure  --prefix=${PREFIX}
make -j4 install
cd ..

# wget http://sourceforge.net/projects/boost/files/boost/1.60.0/boost_1_60_0.tar.bz2/download -O boost_1_60_0.tar.bz2
# tar -xjf boost_1_60_0.tar.bz2
# cd boost_1_60_0
# ./bootstrap.sh --prefix=${PREFIX} --with-libraries=system,filesystem
# ./b2 install
# cd ..
