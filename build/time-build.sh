#! /bin/sh

rm TIMINGS.txt

for PARJOBS in 1 2 3 4 5 6 7 8 9 10 11 12 ; do
    ./BuildMitsuba-Linux.sh $PARJOBS 2>> TIMINGS.txt
done
