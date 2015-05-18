#! /bin/sh

set -e

if [ ! -f tip.zip ] ; then
    wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
fi

# Unpack "mitsuba" directory.

rm -rf mitsuba

rm -rf tempdir
mkdir tempdir
cd tempdir
unzip ../tip.zip > /dev/null
mv mitsuba-* ../mitsuba
cd ..
rmdir tempdir

# Patch and build

cd mitsuba

../patches/patch-mitsuba.sh

cp build/config-linux-gcc.py config.py

# Do double-precision build instead of default single-precision build

sed -i 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

scons -j 8

cd ..
