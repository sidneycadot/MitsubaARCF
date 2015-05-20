#! /bin/sh

set -e

if [ -z "$1" ] ; then
    PARJOBS=8
else
    PARJOBS=$1
fi

# Fetch tip of the Mitsuba repository

if [ ! -f tip.zip ] ; then
    echo "Fetching Mitsuba repository ..."
    wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
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

# Apply ARCF patches

echo "Applying ARCF patches ..."

../patches/patch-mitsuba.sh

# Make sure we have a proper 'config.py'

echo "Making config.py ..."

cp build/config-linux-gcc.py config.py

# Do double-precision build instead of default single-precision build

sed --in-place=.orig 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

# Execute the build

echo "Executing SCONS build ..."

time -f "PARJOBS = $PARJOBS --> build-time = %e seconds." scons -j $PARJOBS

cd ..
