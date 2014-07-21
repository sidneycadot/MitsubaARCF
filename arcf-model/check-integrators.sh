#! /bin/sh

rm -rf integrators
mkdir integrators

ALL12="direct path volpath_simple volpath bdpt photonmapper ppm sppm pssmlt mlt erpt ptracer"

for integrator in $ALL12
do
    if [ $integrator = erpt ] ; then
        samplecount=4
    else
        samplecount=16
    fi
    mitsuba -Dintegrator-type=$integrator -Ddetector.sample_count=$samplecount arcf.xml
    mv arcf.exr integrators/$integrator.exr
done

montage -label %t `echo $ALL12 | tr ' ' '\n' | sed 's/\(.*\)/integrators\/\1.exr/'` montage.png
