#! /bin/sh

rm -rf integrators
mkdir integrators
touch integrators/begin

# direct path volpath_simple volpath bdpt photonmapper ppm sppm pssmlt mlt erpt ptracer

for integrator in direct path volpath_simple volpath bdpt photonmapper ppm sppm pssmlt mlt erpt ptracer
do
    if [ $integrator = erpt ] ; then
        samplecount=4
    else
        samplecount=256
    fi

    mitsuba -Dintegrator-type=$integrator -Ddetector.sample_count=$samplecount arcf.xml
    mv arcf.exr integrators/$integrator.exr
done

montage -label %t integrators/*.exr montage.png
