#! /usr/bin/env python

import mitsuba, multiprocessing

from mitsuba.core import *
from mitsuba.render import Scene, SceneHandler, RenderQueue, RenderJob

def loadScene():
    # Get a reference to the thread's file resolver
    fileResolver = Thread.getThread().getFileResolver()
    fileResolver.appendPath('cbox')

    paramMap = StringMap()
    paramMap['myParameter'] = 'value'

    scene = SceneHandler.loadScene(fileResolver.resolve("cbox.xml"), paramMap)

    return scene

def makeScene():

    pmgr = PluginManager.getInstance()

    scene = Scene()

    scene.addChild(pmgr.create({
        'type' : 'perspective',
        'toWorld' : Transform.lookAt(
            Point(0, 0, -10),
            Point(0, 0, 0),
            Vector(0, 1, 0)
        ),
        'film' : {
            'type'   : 'ldrfilm',
            'width'  : 1920,
            'height' : 1080
        },
        'sampler' : {
            'type'        : 'ldsampler',
            'sampleCount' : 2
        }
    }))

    scene.addChild(pmgr.create({
        'type' : 'point',
        'position' : Point(5, 0, -10),
        'intensity' : Spectrum(100)
    }))

    scene.addChild(pmgr.create({
            'type' : 'sphere',
            'center' : Point(0, 0, 0),
            'radius' : 1.0,
            'bsdf' : {
                    'type' : 'diffuse',
                    'reflectance' : Spectrum(0.4)
            }
    }))

    scene.configure()

    return scene

def main():

    scene = makeScene()

    #print(scene)

    scheduler = Scheduler.getInstance()

    for i in range(0, multiprocessing.cpu_count()):
        scheduler.registerWorker(LocalWorker(i, "wrk{}".format(i)))

    scheduler.start()

    queue = RenderQueue()

    scene.setDestinationFile("renderedResult")

    job = RenderJob("MyRenderJob", scene, queue)
    job.start()

    queue.waitLeft(0)
    queue.join()

    print(Statistics.getInstance().getStats())

if __name__ == "__main__":
    main()
