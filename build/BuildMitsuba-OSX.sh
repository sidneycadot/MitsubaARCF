#! /bin/sh

set -e

if [ -z "$1" ] ; then
    PARJOBS=4
else
    PARJOBS=$1
fi

# Fetch tip of the Mitsuba repository

if [ ! -f tip.zip ] ; then
    echo "Fetching Mitsuba repository ..."
    curl https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip -o tip.zip
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

echo "Applying ARCF patches ..."

#../patches/patch-mitsuba.sh

# Make sure we have a proper 'config.py'

echo "Making config.py ..."

#cp build/config-macos10.9-gcc-x86_64.py config.py
cp ../osx-config.py config.py

# Do double-precision build instead of default single-precision build

#sed --in-place=.orig 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

# Execute the build

echo "Executing SCONS build ..."

../my-time ../`hostname`-report.txt scons -j $PARJOBS

cd ..
