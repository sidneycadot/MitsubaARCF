#! /bin/sh

FILENAME=`date +mitsuba-documentation-%Y%m%d.pdf`

exec curl https://www.mitsuba-renderer.org/releases/current/documentation.pdf -o $FILENAME
