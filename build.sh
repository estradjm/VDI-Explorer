#!/bin/bash

echo 'compiling source ...'
make
mkdir build
cp -f vdi build/
rm -f vdi
echo 'successful compilation...'



