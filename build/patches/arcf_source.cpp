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

using namespace mitsuba;


namespace arcf
{

/*!\plugin{area}{Area light}
 * \icon{emitter_area}
 * \order{2}
 * \parameters{
 *     \parameter{radiance}{\Spectrum}{
 *         Specifies the emitted radiance in units of
 *         power per unit area per unit steradian.
 *     }
 *     \parameter{samplingWeight}{\Float}{
 *         Specifies the relative amount of samples
 *         allocated to this emitter. \default{1}
 *     }
 * }
 *
 * This plugin implements an area light, i.e. a light source that emits
 * diffuse illumination from the exterior of an arbitrary shape.
 * Since the emission profile of an area light is completely diffuse, it
 * has the same apparent brightness regardless of the observer's viewing
 * direction. Furthermore, since it occupies a nonzero amount of space, an
 * area light generally causes scene objects to cast soft shadows.
 *
 * When modeling scenes involving area lights, it is preferable
 * to use spheres as the emitter shapes, since they provide a
 * particularly good direct illumination sampling strategy (see
 * the \pluginref{sphere} plugin for an example).
 *
 * To create an area light source, simply instantiate the desired
 * emitter shape and specify an \code{area} instance as its child:
 *
 * \vspace{4mm}
 * \begin{xml}
 * <!-- Create a spherical light source at the origin -->
 * <shape type="sphere">
 *     <emitter type="area">
 *         <spectrum name="radiance" value="1"/>
 *     </emitter>
 * </shape>
 * \end{xml}
 */

class Source : public Emitter
{
public:

    Source(const Properties& props) :
        Emitter(props)
    {
        m_type |= EDeltaDirection
               |  EOnSurface;

        if (props.hasProperty("toWorld"))
        {
            Log(EError, "Found a 'toWorld' transformation -- this is not allowed -- the area light inherits this transformation from its parent shape");
        }

        m_normalIrradiance = props.getSpectrum("irradiance", Spectrum::getD65());
        m_power = Spectrum(0.0f);  // Don't know the power yet (Depends on attached shape)
    }

    Source(Stream*          stream,
           InstanceManager* manager) :
        Emitter(stream, manager)
    {
        m_normalIrradiance = Spectrum(stream);
        m_power            = Spectrum(stream);
        configure();
    }

    void serialize(Stream*          stream,
                   InstanceManager* manager) const
    {
        Emitter::serialize(stream, manager);
        m_normalIrradiance.serialize(stream);
        m_power           .serialize(stream);
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
        // TODO: [EDD] Check if we should use Epsilon (Depends on precision) instead of DeltaEpsilon
        if (std::abs(dot(its.shFrame.n, d) - 1) <= DeltaEpsilon)
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
        m_shape->sampleDirect(dRec, sample);

        /* Check that the emitter and reference position are oriented correctly
           with respect to each other. Note that the < 0 check
           for 'refN' is intentional -- those sampling requests that specify
           a reference point within a medium or on a transmissive surface
           will set dRec.refN = 0, hence they should always be accepted. */
        if (dot(dRec.d, dRec.refN) <  0                       ||
            std::abs(dot(dRec.d, dRec.n) + 1) >= DeltaEpsilon ||
            dRec.pdf == 0)
        {
            dRec.pdf = 0.0f;
            return Spectrum(0.0f);
        }

/*
        dRec.p       = ;
        dRec.n       = ;
        dRec.d       = ;
        dRec.dist    = ;
        dRec.uv      = ;
        dRec.measure = EDiscrete;
*/
        return m_normalIrradiance / dRec.pdf;
    }

    Float pdfDirect(const DirectSamplingRecord& dRec) const
    {
        // Check that the emitter and receiver are oriented correctly with respect to each other.
        if (dot(dRec.d, dRec.refN) <  0                       ||
            std::abs(dot(dRec.d, dRec.n) + 1) >= DeltaEpsilon ||
            dRec.pdf == 0)
        {
            return 0.0f;
        }

        return m_shape->pdfDirect(dRec);
    }

    void setParent(ConfigurableObject* parent)
    {
        Emitter::setParent(parent);

        if (parent->getClass()->derivesFrom(MTS_CLASS(Shape)))
        {
            Shape* shape = static_cast<Shape*>(parent);
            if (m_shape == shape || shape->isCompound())
                return;

            if (m_shape != NULL)
                Log(EError, "An area light cannot be parent of multiple shapes");

            m_shape = shape;
            m_shape->configure();
            m_power = m_normalIrradiance * m_shape->getSurfaceArea();
        }
        else
        {
            Log(EError, "An area light must be child of a shape instance");
        }
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
            << "  surfaceArea = ";

        if (m_shape)
            oss << m_shape->getSurfaceArea()                          << "," << endl;
        else
            oss << "<no shape attached!>"                             << "," << endl;

        oss << "  medium = "         << indent(m_medium.toString())          << endl
            << "]";

        return oss.str();
    }

    Shader* createShader(Renderer* renderer) const;

    MTS_DECLARE_CLASS()

protected:

    Spectrum m_normalIrradiance;
    Spectrum m_power;
};


// ================ Hardware shader implementation ================

class SourceShader : public Shader
{
public:
    SourceShader(Renderer*       renderer,
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

Shader* Source::createShader(Renderer* renderer) const
{
    return new SourceShader(renderer, m_normalIrradiance);
}

MTS_IMPLEMENT_CLASS(SourceShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(Source, false, Emitter)
MTS_EXPORT_PLUGIN(Source, "ARCF source");

} // namespace arcf
