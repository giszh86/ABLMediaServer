#!/bin/bash

export PREFIX="/home/admin-nx/ABLMediaServer/3rd/faac-1.29.9.2"
sudo ./configure -prefix=${PREFIX} --enable-static=yes --enable-shared=no --with-pic=yes 
mkdir -p $PREFIX/bin/lib
cp ./libfaac/.libs/*.a $PREFIX/bin/lib
cp ./libfaac/.libs/*.so $PREFIX/bin/lib
mkdir -p $PREFIX/bin/include
cp ./include/*.h $PREFIX/bin/include
