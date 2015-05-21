#! /bin/sh

rm -f my-time-report.txt

for PARJOBS in 1 2 3 4 5 6 7 8 9 10 11 12 ; do
    ./BuildMitsuba-Linux.sh $PARJOBS
done
