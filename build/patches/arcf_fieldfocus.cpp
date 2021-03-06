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

#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/hw/basicshader.h>
#include <mitsuba/core/warp.h>

// TODO: [SC] this overrides an existing definition. Are we sure that is what we want?
#define DeltaEpsilon (1e-6)

using namespace mitsuba;

namespace arcf
{

// NOTE: [EDD] Currently the energy conservation law is not satisfied! Check how this affects us
class FieldFocus : public BSDF
{
public:

    FieldFocus(const Properties& props) :
        BSDF(props)
    {
        m_cosFieldAngle = std::cos(degToRad(props.getFloat("angle") / 2.0));
    }

    FieldFocus(Stream*          stream,
               InstanceManager* manager) :
        BSDF(stream, manager)
    {
        m_cosFieldAngle = stream->readFloat();

        configure();
    }

    void serialize(Stream*          stream,
                   InstanceManager* manager) const
    {
        BSDF::serialize(stream, manager);

        stream->writeFloat(m_cosFieldAngle);
    }

    void configure()
    {
        // TODO: [EDD] Check if this is just used as an optimization, and if so, do we use the proper flags
        // TODO: [EDD] If we implement the aperture as a material property, then we need to add ESpatiallyVarying
        m_components.clear();
        m_components.push_back(EDeltaTransmission | EFrontSide);// | ENonSymmetric);
        m_components.push_back(EDiffuseTransmission | EBackSide);

        m_usesRayDifferentials = false;

        // TODO: [SC] it seems more natural if the BSDF is configured first (analogous to constructor ordering).
        //            Are we sure this is ok?

        BSDF::configure();
    }

    Spectrum getDiffuseReflectance(const Intersection& its) const
    {
        return Spectrum(0.0f);
    }

    Spectrum eval(const BSDFSamplingRecord& bRec,
                  EMeasure                  measure) const
    {
        if ((bRec.typeMask & EDeltaTransmission)       &&
            measure == EDiscrete                       &&
            Frame::cosTheta(bRec.wo) > m_cosFieldAngle &&
            std::abs(Frame::cosTheta(bRec.wi) + 1) <= DeltaEpsilon)
        {
            // TODO: [EDD] Should radiance be scaled to account for the solid angle compression?
            return Spectrum(1.0f);
        }

        if ((bRec.typeMask & EDiffuseTransmission)     &&
            measure == ESolidAngle                     &&
            Frame::cosTheta(bRec.wi) > m_cosFieldAngle &&
            std::abs(Frame::cosTheta(bRec.wo) + 1) <= DeltaEpsilon)
        {
            return Spectrum(1.0f);
        }

        return Spectrum(0.0f);
    }

    Float pdf(const BSDFSamplingRecord& bRec,
              EMeasure                  measure) const
    {
        if ((bRec.typeMask & EDeltaTransmission)       &&
            measure == EDiscrete                       &&
            Frame::cosTheta(bRec.wo) > m_cosFieldAngle &&
            std::abs(Frame::cosTheta(bRec.wi) + 1) <= DeltaEpsilon)
        {
            return 1.0f;
        }

        if ((bRec.typeMask & EDiffuseTransmission)     &&
            measure == ESolidAngle                     &&
            Frame::cosTheta(bRec.wi) > m_cosFieldAngle &&
            std::abs(Frame::cosTheta(bRec.wo) + 1) <= DeltaEpsilon)
        {
            return warp::squareToUniformConePdf(m_cosFieldAngle) * Frame::cosTheta(bRec.wo);
        }

        return 0.0f;
    }

    Spectrum sample(BSDFSamplingRecord& bRec,
                    const Point2&       sample) const
    {
        if (!(bRec.typeMask & m_combinedType))
        {
            return Spectrum(0.0f);
        }

        // When the BSDF acts as field collimator
        if (Frame::cosTheta(bRec.wi) > m_cosFieldAngle)
        {
            bRec.wo = -bRec.its.shFrame.n;
            bRec.sampledComponent = 0;
            bRec.sampledType = EDeltaTransmission;

            return Spectrum(1.0f);
        }

        // When the BSDF acts as field decollimator
        if (std::abs(Frame::cosTheta(bRec.wi) + 1) <= DeltaEpsilon)
        {
            bRec.wo  = warp::squareToUniformCone(m_cosFieldAngle, sample);
            bRec.sampledComponent = 1;
            bRec.sampledType = EDiffuseTransmission;

            return Spectrum(1.0f);
        }

        return Spectrum(0.0f);
    }

    Spectrum sample(BSDFSamplingRecord& bRec,
                    Float&              pdf,
                    const Point2&       sample) const
    {
        if (!(bRec.typeMask & m_combinedType))
        {
            return Spectrum(0.0f);
        }

        // When the BSDF acts as field collimator
        if (Frame::cosTheta(bRec.wi) > m_cosFieldAngle)
        {
            bRec.wo  = -bRec.its.shFrame.n;
            bRec.eta = 1.0f;
            bRec.sampledComponent = 0;
            bRec.sampledType = EDeltaTransmission;
            pdf = 1.0f;

            return Spectrum(1.0f);
        }

        // When the BSDF acts as field decollimator
        // TODO: [EDD] In theory, -1 <= cosTheta <= 1, so taking abs() is not required; Check if numerical precision allows this
        if (std::abs(Frame::cosTheta(bRec.wi) + 1) <= DeltaEpsilon)
        {
            bRec.wo  = warp::squareToUniformCone(m_cosFieldAngle, sample);
            bRec.eta = 1.0f;
            bRec.sampledComponent = 1;
            bRec.sampledType = EDiffuseTransmission;
            pdf = warp::squareToUniformConePdf(m_cosFieldAngle) * Frame::cosTheta(bRec.wo);

            return Spectrum(1.0f);
        }

        return Spectrum(0.0f);
    }

    Float getRoughness(const Intersection& its,
                       int                 component) const
    {
        return 0.0f;
    }

    std::string toString() const
    {
        std::ostringstream oss;
        // TODO: [SC]: note that we show cos(angle/2) here rather than the specified 'angle'. This may be confusing for a user.
        oss << "ARCF Field Focus BSDF"           << endl
            << "["                               << endl
            << "  id = \""  << getID() << "\","  << endl
            << "  angle = " << m_cosFieldAngle << endl
            << "]";
        return oss.str();
    }

    Shader* createShader(Renderer* renderer) const;

    MTS_DECLARE_CLASS()

private:

    Float m_cosFieldAngle;
};

// ================ Hardware shader implementation ================

class FieldFocusShader : public Shader
{
public:

    FieldFocusShader(Renderer* renderer) :
        Shader(renderer, EBSDFShader)
    {
    }

    void generateCode(std::ostringstream&             oss,
                      const std::string&              evalName,
                      const std::vector<std::string>& depNames) const
    {
        oss << "vec3 " << evalName << "(vec2 uv, vec3 wi, vec3 wo)"         << endl
            << "{"                                                          << endl
            << "    if (cosTheta(wi) < 0.0 || cosTheta(wo) < 0.0)"          << endl
            << "        return vec3(0.0);"                                  << endl
            << "    return vec3(inv_pi * cosTheta(wo));"                    << endl
            << "}"                                                          << endl
            << ""                                                           << endl
            << "vec3 " << evalName << "_diffuse(vec2 uv, vec3 wi, vec3 wo)" << endl
            << "{"                                                          << endl
            << "    return " << evalName << "(uv, wi, wo);"                 << endl
            << "}"                                                          << endl;
    }

    MTS_DECLARE_CLASS()
};

Shader* FieldFocus::createShader(Renderer* renderer) const
{
    return new FieldFocusShader(renderer);
}

MTS_IMPLEMENT_CLASS(FieldFocusShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(FieldFocus, false, BSDF)
MTS_EXPORT_PLUGIN(FieldFocus, "ARCF FieldFocus BSDF")

} // namespace arcf
