#! /bin/sh

set -e

PATCHDIR=$(dirname $0)
MITSUBADIR=$PWD

if [ ! -f ${MITSUBADIR}/SConstruct ] ; then
    echo "error: please execute this script at the top level of a Mitsuba checkout."
    exit 1
fi

### Patch the "mfilm" implementation for Mathematica output.
#patch -p1 < ${PATCHDIR}/mfilm_mathematica_fix.patch
# --> Included in Mitsuba as per 22-07-2014.

### Add arcf_source emitter.

# Copy the source file

cp ${PATCHDIR}/arcf_source.cpp ${MITSUBADIR}/src/emitters

# In the CMake and SCons build files, copy the line for the "area" emitter and add a line that replaces "area" with "arcf_source".

sed -i 's/\(\(.*\)area\(.\+\)area\(\.cpp.*\)\)/\1\n\2arcf_source\3arcf_source\4/' ${MITSUBADIR}/src/emitters/CMakeLists.txt
sed -i 's/\(\(.*\)area\(.\+\)area\(\.cpp.*\)\)/\1\n\2arcf_source\3arcf_source\4/' ${MITSUBADIR}/src/emitters/SConscript

### Add arcf_collimated_disk emitter.

# Copy the source file

cp ${PATCHDIR}/arcf_collimated_disk.cpp ${MITSUBADIR}/src/emitters

# In the CMake and SCons build files, copy the line for the "area" emitter and add a line that replaces "area" with "arcf_collimated_disk".

sed -i 's/\(\(.*\)area\(.\+\)area\(\.cpp.*\)\)/\1\n\2arcf_collimated_disk\3arcf_collimated_disk\4/' ${MITSUBADIR}/src/emitters/CMakeLists.txt
sed -i 's/\(\(.*\)area\(.\+\)area\(\.cpp.*\)\)/\1\n\2arcf_collimated_disk\3arcf_collimated_disk\4/' ${MITSUBADIR}/src/emitters/SConscript

### Add arcf_aperture texture.

# Copy the source file

cp ${PATCHDIR}/arcf_aperture.cpp ${MITSUBADIR}/src/textures

# In the CMake and SCons build files, copy the line for the "checkerboard" texture and add a line that replaces "checkerboard" with "arcf_aperture".

sed -i 's/\(\(.*\)checkerboard\(.\+\)checkerboard\(\.cpp.*\)\)/\1\n\2arcf_aperture\3arcf_aperture\4/' ${MITSUBADIR}/src/textures/CMakeLists.txt
sed -i 's/\(\(.*\)checkerboard\(.\+\)checkerboard\(\.cpp.*\)\)/\1\n\2arcf_aperture\3arcf_aperture\4/' ${MITSUBADIR}/src/textures/SConscript

### Add arcf_fieldfocus bsdf.

cp ${PATCHDIR}/arcf_fieldfocus.cpp ${MITSUBADIR}/src/bsdfs

# In the CMake and SCons build files, copy the line for the "null" bsdf and add a line that replaces "null" with "arcf_fieldfocus".

sed -i 's/\(\(.*\)null\(.\+\)null\(\.cpp.*\)\)/\1\n\2arcf_fieldfocus\3arcf_fieldfocus\4/' ${MITSUBADIR}/src/bsdfs/CMakeLists.txt
sed -i 's/\(\(.*\)null\(.\+\)null\(\.cpp.*\)\)/\1\n\2arcf_fieldfocus\3arcf_fieldfocus\4/' ${MITSUBADIR}/src/bsdfs/SConscript
