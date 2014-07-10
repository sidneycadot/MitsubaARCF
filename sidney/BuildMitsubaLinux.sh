#! /bin/sh

TIMESTAMP=`date +%Y-%m-%d_%H:%M:%S`

rm tip.zip
wget https://www.mitsuba-renderer.org/repos/mitsuba/archive/tip.zip
cp tip.zip tip-$TIMESTAMP.zip

rm -rf mitsuba-*

unzip tip-$TIMESTAMP.zip

exit

mv mitsuba-* mitsuba-$TIMESTAMP

ln -sf mitsuba-$TIMESTAMP mitsuba-current

cd mitsuba-current

patch -p1 < ../arcfshape.patch

cp build/config-linux-gcc.py config.py

# Do double precision build by default
sed -i 's/-DSINGLE_PRECISION/-DDOUBLE_PRECISION/;s/-DMTS_SSE//;s/-DMTS_HAS_COHERENT_RT//' config.py

scons -j 8

cd ..
