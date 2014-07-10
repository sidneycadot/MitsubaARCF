#! /bin/sh

set -e

TIMESTAMP=`date +%Y-%m-%d_%H:%M:%S`

if [ ! -f tip.zip ] ; then
    wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
fi

# Unpack

rm -rf tempdir
mkdir tempdir
cd tempdir
unzip ../tip.zip
mv mitsuba-* ../mitsuba-$TIMESTAMP
cd ..
rmdir tempdir

ln -sf mitsuba-$TIMESTAMP mitsuba-current

# Patch and build

cd mitsuba-current

patch -p1 < ../../patches/arcfshape.patch
patch -p1 < ../../patches/arcf_source.patch

cp build/config-linux-gcc.py config.py

# Do double precision build by default
sed -i 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

scons -j 8

cd ..
