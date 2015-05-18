#! /bin/sh

set -e

# Fetch tip of the Mitsuba repository

if [ ! -f tip.zip ] ; then
    echo "Fetching Mitsuba repository ..."
    wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
fi

# Fetch OSX dependencies using Mercurial

if [ ! -d dependencies_macos ] ; then
    echo "Fetching OSX dependencies ..."
    hg clone https://www.mitsuba-renderer.org/hg/dependencies_macos
fi

# Unpack "mitsuba" directory.

echo "Unpacking Mitsuba directory ..."

rm -rf mitsuba

rm -rf tempdir
mkdir tempdir
cd tempdir
unzip ../tip.zip > /dev/null
mv mitsuba-* ../mitsuba
cd ..
rmdir tempdir

# Change working directory to the fresh 'mitsuba' directory.

cd mitsuba

# Add OSX dependencies

echo "Adding OSX dependencies ..."

cp -r ../dependencies_macos dependencies

# Apply ARCF patches

../patches/patch-mitsuba.sh

# Make sure we have a proper 'config.py'

#cp build/config-macos10.9-gcc-x86_64.py config.py
cp ../osx-config.py config.py

# Do double-precision build instead of default single-precision build

#sed -i 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

# Execute the build

echo "Executing SCONS build ..."

# -- 1  13m29
# -- 2  07m21
# -- 3  07m17
# -- 4  07m14
# -- 5  07m31
# -- 6  07m52
# -- 7  07m59
# -- 8  08m12

time scons -j 4

cd ..
