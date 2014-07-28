
Running an irradiance simulation
================================

The "bdpt" integrator works well for this case.

rm -f arcf.m arcf-irradiance.m && time mitsuba -Dsample-model=none -Ddetector.motor_angle=180 arcf.xml && mv arcf.m arcf-irradiance.m && ./extract_power.py arcf-irradiance.m

Running a radiance simulation
=============================

The "bdpt" integrator works well for this case.

rm -f arcf.m arcf-radiance.m && time mitsuba -Dsample-model=none -Ddetector.motor_angle=180 arcf.xml && mv arcf.m arcf-radiance.m && ./extract_power.py arcf-irradiance.m

