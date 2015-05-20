/*
    This file is part of Mitsuba, a physically based rendering system.

    Copyright (c) 2007-2012 by Wenzel Jakob and others.

    Mitsuba is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Mitsuba is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <mitsuba/render/emitter.h>
#include <mitsuba/render/shape.h>
#include <mitsuba/render/medium.h>
#include <mitsuba/hw/gpuprogram.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/core/plugin.h>

using namespace mitsuba;


namespace arcf
{

class CollimatedDisk : public Emitter
{
public:

    CollimatedDisk(const Properties& props) :
        Emitter(props)
    {
        // TODO: [EDD] Check if EOnSurface still applies
        m_type |= EDeltaDirection
               |  EOnSurface;

        m_objectToWorld = new AnimatedTransform(props.getAnimatedTransform("toWorld", Transform()));
        m_normalIrradiance = props.getSpectrum("irradiance", Spectrum::getD65());
    }

    CollimatedDisk(Stream*          stream,
                   InstanceManager* manager) :
        Emitter(stream, manager)
    {
        m_objectToWorld = new AnimatedTransform(stream);
        m_normalIrradiance = Spectrum(stream);
        configure();
    }

    void serialize(Stream*          stream,
                   InstanceManager* manager) const
    {
        Emitter::serialize(stream, manager);
        m_objectToWorld->serialize(stream);
        m_normalIrradiance.serialize(stream);
    }

    virtual void configure()
    {
        Emitter::configure();

        // Create new toWorld transform for the disk
        Properties props("disk");
        props.setAnimatedTransform("toWorld", m_objectToWorld.get());

        // Create emitter shape
        m_shape = static_cast<Shape*>(PluginManager::getInstance()->createObject(props));
        m_shape->configure();

        // Determine emitter power
        m_power = m_normalIrradiance * m_shape->getSurfaceArea();
    }

    Spectrum samplePosition(PositionSamplingRecord& pRec,
                            const Point2&           sample,
                            const Point2*           extra) const
    {
        m_shape->samplePosition(pRec, sample);

        return m_power;
    }

    Spectrum evalPosition(const PositionSamplingRecord& pRec) const
    {
        return (pRec.measure == EArea) ? m_normalIrradiance : Spectrum(0.0f);
    }

    Float pdfPosition(const PositionSamplingRecord& pRec) const
    {
        return (pRec.measure == EArea) ? m_shape->pdfPosition(pRec) : 0.0f;
    }

    Spectrum sampleDirection(DirectionSamplingRecord& dRec,
                             PositionSamplingRecord&  pRec,
                             const Point2&            sample,
                             const Point2*            extra) const
    {
        dRec.d       = pRec.n;
        dRec.pdf     = 1.0f;
        dRec.measure = EDiscrete;

        return Spectrum(1.0f);
    }

    Spectrum evalDirection(const DirectionSamplingRecord& dRec,
                           const PositionSamplingRecord&  pRec) const
    {
        return Spectrum((dRec.measure == EDiscrete) ? 1.0f : 0.0f);
    }

    Float pdfDirection(const DirectionSamplingRecord& dRec,
                       const PositionSamplingRecord&  pRec) const
    {
        return (dRec.measure == EDiscrete) ? 1.0f : 0.0f;
    }

    Spectrum eval(const Intersection& its,
                  const Vector&       d) const
    {
        if (std::abs(dot(its.shFrame.n, d) - 1) <= Epsilon)
        {
            return Spectrum(0.0f);
        }

        return m_normalIrradiance;
    }

    Spectrum sampleRay(Ray&          ray,
                       const Point2& spatialSample,
                       const Point2& directionalSample,
                       Float         time) const
    {
        PositionSamplingRecord pRec(time);
        m_shape->samplePosition(pRec, spatialSample);

        ray.setTime(time);
        ray.setOrigin(pRec.p);
        ray.setDirection(pRec.n);

        return m_power;
    }

    Spectrum sampleDirect(DirectSamplingRecord& dRec,
                          const Point2&         sample) const
    {
        const Transform& trafo = m_worldTransform->eval(dRec.time);

        dRec.n = trafo(Vector(0, 0, 1));
        Float scale = dRec.n.length();

        Point localP = trafo.inverse().transformAffine(dRec.ref);

        if (Vector2(localP.x, localP.y).length() > 1.0)
        {
            dRec.pdf = 0.0f;
            return Spectrum(0.0f);
        }

        dRec.p = trafo.transformAffine(Point(localP.x,
                                             localP.y,
                                             0.0f));
        dRec.n /= scale;
        dRec.d = -dRec.n;
        dRec.dist = localP.z;
//      dRec.uv = Point2(sample.x * m_resolution.x,
//                       sample.y * m_resolution.y);
        dRec.pdf = 1.0f;
        dRec.measure = EDiscrete;

        return m_normalIrradiance;
    }

    Float pdfDirect(const DirectSamplingRecord& dRec) const
    {
        return (dRec.measure == EDiscrete) ? 1.0f : 0.0f;
    }

    AABB getAABB() const
    {
        return m_shape->getAABB();
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "ARCF source"                                                 << endl
            << "["                                                           << endl
            << "  irradiance = "     << m_normalIrradiance.toString() << "," << endl
            << "  samplingWeight = " << m_samplingWeight              << "," << endl
            << "  surfaceArea = "    << m_shape->getSurfaceArea()     << "," << endl
            << "  medium = "         << indent(m_medium.toString())          << endl
            << "]";

        return oss.str();
    }

    Shader* createShader(Renderer* renderer) const;

    MTS_DECLARE_CLASS()

protected:

    ref<AnimatedTransform> m_objectToWorld;
    ref<Shape>             m_shape;
    Spectrum               m_normalIrradiance;
    Spectrum               m_power;
};


// ================ Hardware shader implementation ================

class CollimatedDiskShader : public Shader
{
public:
    CollimatedDiskShader(Renderer*       renderer,
                         const Spectrum& radiance) :
        Shader(renderer, EEmitterShader),
        m_radiance(radiance)
    {
    }

    void resolve(const GPUProgram*  program,
                 const std::string& evalName,
                 std::vector<int>&  parameterIDs) const
    {
        parameterIDs.push_back(program->getParameterID(evalName + "_radiance", false));
    }

    void generateCode(std::ostringstream&             oss,
                      const std::string&              evalName,
                      const std::vector<std::string>& depNames) const
    {
        oss << "uniform vec3 " << evalName << "_radiance;"    << endl
                                                              << endl
            << "vec3 " << evalName << "_area(vec2 uv)"        << endl
            << "{"                                            << endl
            << "    return " << evalName << "_radiance * pi;" << endl
            << "}"                                            << endl
                                                              << endl
            << "vec3 " << evalName << "_dir(vec3 wo)"         << endl
            << "{"                                            << endl
            << "    if (cosTheta(wo) < 0.0)"                  << endl
            << "        return vec3(0.0);"                    << endl
            << "    return vec3(inv_pi);"                     << endl
            << "}"                                            << endl;
    }

    void bind(GPUProgram*             program,
              const std::vector<int>& parameterIDs,
              int&                    textureUnitOffset) const
    {
        program->setParameter(parameterIDs[0], m_radiance);
    }

    MTS_DECLARE_CLASS()

private:

    Spectrum m_radiance;
};

Shader* CollimatedDisk::createShader(Renderer* renderer) const
{
    return new CollimatedDiskShader(renderer, m_normalIrradiance);
}

MTS_IMPLEMENT_CLASS(CollimatedDiskShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(CollimatedDisk, false, Emitter)
MTS_EXPORT_PLUGIN(CollimatedDisk, "ARCF collimated disk");

} // namespace arcf
