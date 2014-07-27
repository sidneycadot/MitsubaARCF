#! /usr/bin/env python

import multiprocessing

from mitsuba.core import PluginManager, Properties, Transform, Point, Vector, Scheduler, LocalWorker, Statistics
from mitsuba.render import Scene, RenderQueue, RenderJob

# We have:
# - a geometrical configuration of the sample and the detector:
# - an environment (optional)
# - a monochromator source
# - a monochromator source model
# - a beam inhomogeneity mask
# - a sample (eg. spectralon diffuser, qvd diffuser)
# - a detector (optional field limiter; optional texture; optional color; size).
# - a camera that we want to render to,

def render(scene, filename):
    scheduler = Scheduler.getInstance()
    for i in range(multiprocessing.cpu_count()):
        scheduler.registerWorker(LocalWorker(i, "wrk{}".format(i)))
    scheduler.start()

    queue = RenderQueue()
    scene.setDestinationFile(filename)
    job = RenderJob("myRenderJob", scene, queue)
    job.start()
    queue.waitLeft(0)
    queue.join()
    print(Statistics.getInstance().getStats())

def makeScene():

    scene = Scene()

    pmgr = PluginManager.getInstance()

    # make shapes
    for i in range(100):
        shapeProps = Properties("sphere")
        shapeProps["center"] = Point(i, i, i)
        shapeProps["radius"] = 0.1
        shape = pmgr.createObject(shapeProps)
        shape.configure()

        scene.addChild(shape)

    # make perspective sensor
    sensorProps = Properties("perspective")
    sensorProps["toWorld"] = Transform.lookAt(Point(0, 0, 10), Point(0, 0, 0), Vector(0, 1, 0))
    sensorProps["fov"] = 45.0

    sensor = pmgr.createObject(sensorProps)

    # make film
    filmProps = Properties("ldrfilm")
    filmProps["width"]  = 640
    filmProps["height"] = 480

    film = pmgr.createObject(filmProps)
    film.configure()

    sensor.addChild("film", film)
    sensor.configure()

    scene.addChild(sensor)
    scene.configure()

    return scene

def main():

    scene = makeScene()

    render(scene, "sidney.png")

if __name__ == "__main__":
    main()
