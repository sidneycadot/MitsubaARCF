Testcase for "arcf_source".

The arcf_source disk lights the diffuser, rotated at 45 degrees. The detector (with orthographic camera)is at 90 degrees.
It should see a circular disk on the diffuser.

The detector is without field focusser and aperture.

The following integrators fail (i.e. produce a black image)when using the arcf_source:

  - direct, path, volpath_simple, volpath, photonmapper, mlt

The following integrators work:

  - bdpt, ppm, sppm, pssmlt, erpt, ptracer.

If we use a spotlight (positioned to give approximately the same footprint on the diffuser), we see that all integrators
give the expected image.

The "spotlight" can be selected by setting -Dmonochromator-type=spotlight on the command line. See check-integrators.sh
for an example.
