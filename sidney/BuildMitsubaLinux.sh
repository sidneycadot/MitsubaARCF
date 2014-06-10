#! /bin/sh

TIMESTAMP=`date +%Y-%m-%d_%H:%M:%S`
#TIMESTAMP="timestamp"

rm tip.zip
wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
mv tip.zip tip-$TIMESTAMP.zip

rm -rf mitsuba-*

unzip tip-$TIMESTAMP.zip

mv mitsuba-* mitsuba-$TIMESTAMP

cd mitsuba-$TIMESTAMP

cp build/config-linux-gcc.py config.py

scons -j 8

cd ..

ln -sf mitsuba-$TIMESTAMP mitsuba-current
