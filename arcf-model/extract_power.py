#! /usr/bin/env python3

import sys, math

areaOfDetector = 0.020 ** 2
areaOfAperture = (16 * (4 + math.pi)) / 1e6

detectorFieldOfViewDiameter = 2.0 # degrees

detectorFieldOfViewSolidAngle = 2 * math.pi * (1 - math.cos(math.radians(detectorFieldOfViewDiameter) / 2))

argv = sys.argv[1:]
len_argv = len(argv)

if True:
    # check for "-i" option
    argv = [arg for arg in argv if not (arg == "-i")]
    has_option_irradiance = len(argv) < len_argv
    len_argv = len(argv)

if True:
    # check for "-r" option
    argv = [arg for arg in argv if not (arg == "-r")]
    has_option_radiance = len(argv) < len_argv
    len_argv = len(argv)

if len_argv != 1:
    print("Expected a filename.")
    sys.exit(1)

filename = argv[0]

with open(filename) as f:
    data = f.read()

data = data.replace("\n", "")


(dummy, data) = data.split("=", 1)

data = data.replace("*^", "e")

for c in "{} \t;":
    data = data.replace(c, "")


data = data.split(",")
data = list(map(float, data))

meanPowerDetectorArea = sum(data)/len(data)

print("meanPowerDetectorArea", meanPowerDetectorArea)

totalPower = meanPowerDetectorArea * areaOfDetector

powerPerApertureArea = totalPower / areaOfAperture

print("powerPerApertureArea", powerPerApertureArea)

if has_option_radiance:
    brdf = (powerPerApertureArea / detectorFieldOfViewSolidAngle) / (1.0 * math.cos(math.radians(45)))
    brdfPerfect = 1.0 / math.pi

    errPercents = (brdf / brdfPerfect - 1) * 100.0

    print("brdf calculated: {} ({:.2f} %)".format(brdf, errPercents))
