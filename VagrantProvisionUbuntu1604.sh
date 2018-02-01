#!/usr/bin/env bash

IN_DIR="/opt"
HOME="/home/vagrant"

echo "Adding ubuntugis and ubuntu-toolchain-r-test repositories..."
add-apt-repository -y ppa:ubuntu-toolchain-r/test
add-apt-repository -y ppa:ubuntugis/ubuntugis-unstable
apt-get update >/dev/null 2>&1

echo "Installing C++ packages..."
apt-get install -y \
gcc-6 \
g++-6 \
cmake \
libgdal-dev \
libboost-dev \
libboost-filesystem-dev \
libboost-locale-dev \
libboost-thread-dev \
libboost-iostreams-dev \
libboost-program-options-dev \
libyaml-cpp-dev \
libproj-dev \
libgeotiff-dev \
libgmp-dev \
libmpfr-dev \
unzip

apt-get update >/dev/null 2>&1
apt-get upgrade -y >/dev/null 2>&1

# -------
echo "Installing LASzip..."
# note that LASzip need to be compiled before libLAS

cd $IN_DIR
wget https://github.com/LASzip/LASzip/releases/download/v2.2.0/laszip-src-2.2.0.tar.gz
wget https://github.com/LASzip/LASzip/releases/download/v2.2.0/laszip-src-2.2.0.tar.gz.md5
md5sum -c laszip-src-2.2.0.tar.gz.md5
rm laszip-src-2.2.0.tar.gz.md5
tar -xf laszip-src-2.2.0.tar.gz
rm laszip-src-2.2.0.tar.gz
cd laszip-src-2.2.0

# The Makefile need to be modified in order to be compliant to what libLAS
# is looking for. https://github.com/libLAS/libLAS/issues/9
sed -i 's/laszipdir = $(includedir)\//laszipdir = $(includedir)\/laszip/' ./include/laszip/Makefile.am
sed -i 's/laszipdir = $(includedir)\//laszipdir = $(includedir)\/laszip/' ./include/laszip/Makefile.in

# Store the compiled sofware within its directory instead of distributing
# the files in the filesystem. It is easier for libLAS to locate and link
# the executables this way.
mkdir build
./configure --prefix=$IN_DIR/laszip-src-2.2.0/build
make
make install
make clean

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$IN_DIR/laszip-src-2.2.0/build/lib

#--------
echo "Installing libLAS ..."

cd $IN_DIR
wget http://download.osgeo.org/liblas/libLAS-1.8.1.tar.bz2
# for some reason the md5 file was not accessible on osgeo when I checked it
# wget http://download2.osgeo.org/liblas/libLAS-1.8.1.tar.bz2.md5
# md5sum -c libLAS-1.8.1.tar.bz2.md5
# rm libLAS-1.8.1.tar.bz2.md5
tar -xf libLAS-1.8.1.tar.bz2
rm libLAS-1.8.1.tar.bz2
cd libLAS-1.8.1
mkdir build
mkdir cmake_build
cd cmake_build
# Compile with GDAL and LASzip. GDAL should be found automatically, thus no
# need to provide the link here.
cmake .. \
-DCMAKE_INSTALL_PREFIX=$IN_DIR/libLAS-1.8.1/build \
-DWITH_GDAL=ON \
-DWITH_LASZIP=ON \
-DLASZIP_INCLUDE_DIR=$IN_DIR/laszip-src-2.2.0/build/include \
-DLASZIP_LIBRARY=$IN_DIR/laszip-src-2.2.0/build/lib/liblaszip.so
make
make install
make clean

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$IN_DIR/libLAS-1.8.1/build/lib

# test installation, should GDAL, LASzip should be listed
$IN_DIR/libLAS-1.8.1/build/bin/lasinfo -h | grep -iq "lasinfo (libLAS 1.8.1 with GeoTIFF 1.4.0 GDAL 2.1.0 LASzip 2.2.0)"


export CC=gcc-6
export CXX=g++-6


echo "Building CGAL ..."
cd $IN_DIR
wget https://github.com/CGAL/cgal/archive/releases/CGAL-4.10.tar.gz
wget https://github.com/CGAL/cgal/releases/download/releases%2FCGAL-4.10/md5sum.txt
md5sum -c md5sum.txt
rm md5sum.txt
tar -xf CGAL-4.10.tar.gz
rm CGAL-4.10.tar.gz
cd cgal-releases-CGAL-4.10

mkdir build
cd build
cmake ..
make

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$IN_DIR/cgal-releases-CGAL-4.10/build/lib

echo "Building 3dfier ..."
cd $HOME
# need to build outside of the shared folder for better performance
mkdir $HOME/3dfier_build && cd $HOME/3dfier_build

# note that cmake might need to be run twice
cmake ../3dfier \
-DLIBLASC_LIBRARY=$IN_DIR/libLAS-1.8.1/build/lib/liblas_c.so \
-DLIBLAS_INCLUDE_DIR=$IN_DIR/libLAS-1.8.1/build/include \
-DLIBLAS_LIBRARY=$IN_DIR/libLAS-1.8.1/build/lib/liblas.so \
-DLASZIP_INCLUDE_DIR=$IN_DIR/laszip-src-2.2.0/build/include \
-DLASZIP_LIBRARY=$IN_DIR/laszip-src-2.2.0/build/lib/liblaszip.so \
-DCGAL_DIR=$IN_DIR/cgal-releases-CGAL-4.10/build


cmake ../3dfier \
-DLIBLASC_LIBRARY=$IN_DIR/libLAS-1.8.1/build/lib/liblas_c.so \
-DLIBLAS_INCLUDE_DIR=$IN_DIR/libLAS-1.8.1/build/include \
-DLIBLAS_LIBRARY=$IN_DIR/libLAS-1.8.1/build/lib/liblas.so \
-DLASZIP_INCLUDE_DIR=$IN_DIR/laszip-src-2.2.0/build/include \
-DLASZIP_LIBRARY=$IN_DIR/laszip-src-2.2.0/build/lib/liblaszip.so \
-DCGAL_DIR=$IN_DIR/cgal-releases-CGAL-4.10/build
make

cd $HOME/3dfier/example_data
$HOME/3dfier_build/3dfier testarea_config.yml -o output/testarea.obj | grep -iq "Successfully terminated"

# Set ownership
chown vagrant:vagrant $HOME/3dfier_build

echo "See VAGRANT.md for additional configuration instructions and then run 'vagrant ssh' to log into the val3dity virtual machine."

