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

#include <mitsuba/render/texture.h>
#include <mitsuba/render/shape.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/hw/gpuprogram.h>

MTS_NAMESPACE_BEGIN

/*!\plugin{arcfshape}{ARCFShape}
 * \order{2}
 * \parameters{
 *     \parameter{color0, color1}{\Spectrum}{
 *       Color values for the two differently-colored patches
 *       \default{0.4 and 0.2}
 *     }
 *     \parameter{uoffset, voffset}{\Float}{
 *       Numerical offset that should be applied to UV values before a lookup
 *     }
 *     \parameter{uscale, vscale}{\Float}{
 *       Multiplicative factors that should be applied to UV values before a lookup
 *     }
 *     \parameter{ubar, vbar}{\Float}{
 *       Fraction of UV bars. Make these 0 for a circular texture shape; 1 for a square texture shape.
 *     }
 * }
 * This plugin implements a simple procedural ARCFShape texture with
 * customizable colors.
 */
class ARCFShape : public Texture2D {
public:
	ARCFShape(const Properties &props) : Texture2D(props) {
		m_color0 = props.getSpectrum("color0", Spectrum(.4f));
		m_color1 = props.getSpectrum("color1", Spectrum(.2f));
		m_uvBar = Point2(
			props.getFloat("ubar", 0.0f),
			props.getFloat("vbar", 0.0f)
		);
	}

	ARCFShape(Stream *stream, InstanceManager *manager)
	 : Texture2D(stream, manager) {
		m_color0 = Spectrum(stream);
		m_color1 = Spectrum(stream);
		m_uvBar = Point2(stream);
	}

	void serialize(Stream *stream, InstanceManager *manager) const {
		Texture2D::serialize(stream, manager);
		m_color0.serialize(stream);
		m_color1.serialize(stream);
		m_uvBar.serialize(stream);
	}

	inline Spectrum eval(const Point2 &uv) const {

		const Float x = (m_uvBar.x == 1.0f) ? (std::abs(uv.x) < 1.0f ? 0.0f : uv.x) : (uv.x + 0.5f * (std::abs(m_uvBar.x - uv.x) - std::abs(m_uvBar.x + uv.x))) / (1.0f - m_uvBar.x);
		const Float y = (m_uvBar.y == 1.0f) ? (std::abs(uv.y) < 1.0f ? 0.0f : uv.y) : (uv.y + 0.5f * (std::abs(m_uvBar.y - uv.y) - std::abs(m_uvBar.y + uv.y))) / (1.0f - m_uvBar.y);

		if (x * x + y * y < 1.0f)
		{
			return m_color0;
		}
		else
		{
			return m_color1;
		}
	}

	Spectrum eval(const Point2 &uv,
			const Vector2 &d0, const Vector2 &d1) const {
		/* Filtering is currently not supported */
		return ARCFShape::eval(uv);
	}

	bool usesRayDifferentials() const {
		return false;
	}

	Spectrum getMaximum() const {
		Spectrum max;
		for (int i=0; i<SPECTRUM_SAMPLES; ++i)
			max[i] = std::max(m_color0[i], m_color1[i]);
		return max;
	}

	Spectrum getMinimum() const {
		Spectrum min;
		for (int i=0; i<SPECTRUM_SAMPLES; ++i)
			min[i] = std::min(m_color0[i], m_color1[i]);
		return min;
	}

	Spectrum getAverage() const {
		return (m_color0 + m_color1) * 0.5f;
	}

	bool isConstant() const {
		return false;
	}

	bool isMonochromatic() const {
		return Spectrum(m_color0[0]) == m_color0
			&& Spectrum(m_color1[0]) == m_color1;
	}

	std::string toString() const {
		std::ostringstream oss;
		oss << "ARCFShape[" << endl
			<< "    color1 = " << m_color1.toString() << "," << endl
			<< "    color0 = " << m_color0.toString() << "," << endl
			<< "    uvBar = " << m_uvBar.toString() << endl
			<< "]";
		return oss.str();
	}

	Shader *createShader(Renderer *renderer) const;

	MTS_DECLARE_CLASS()
protected:
	Spectrum m_color1;
	Spectrum m_color0;
	Point2 m_uvBar;
};

// ================ Hardware shader implementation ================

class ARCFShapeShader : public Shader {
public:
	ARCFShapeShader(Renderer *renderer,
		const Spectrum &color0, const Spectrum &color1, const Point2 &uvBar,
		const Point2 &uvOffset, const Vector2 &uvScale) : Shader(renderer, ETextureShader),
		m_color0(color0), m_color1(color1), m_uvBar(uvBar),
		m_uvOffset(uvOffset), m_uvScale(uvScale) {
	}

	void generateCode(std::ostringstream &oss,
			const std::string &evalName,
			const std::vector<std::string> &depNames) const {
		oss << "uniform vec3 " << evalName << "_color0;" << endl
			<< "uniform vec3 " << evalName << "_color1;" << endl
			<< "uniform vec2 " << evalName << "_uvBar;" << endl
			<< "uniform vec2 " << evalName << "_uvOffset;" << endl
			<< "uniform vec2 " << evalName << "_uvScale;" << endl
			<< endl
			<< "vec3 " << evalName << "(vec2 uv) {" << endl
			<< "    uv = vec2(" << endl
			<< "        uv.x * " << evalName << "_uvScale.x + " << evalName << "_uvOffset.x," << endl
			<< "        uv.y * " << evalName << "_uvScale.y + " << evalName << "_uvOffset.y);" << endl
			<< "    float x = (" << evalName << "_uvBar.x == 1.0f) ? (abs(uv.x) < 1.0f ? 0.0f : uv.x) : (uv.x + 0.5f * (abs(" << evalName << "_uvBar.x - uv.x) - abs(" << evalName << "_uvBar.x + uv.x))) / (1.0f - " << evalName << "_uvBar.x);" << endl
			<< "    float y = (" << evalName << "_uvBar.y == 1.0f) ? (abs(uv.y) < 1.0f ? 0.0f : uv.y) : (uv.y + 0.5f * (abs(" << evalName << "_uvBar.y - uv.y) - abs(" << evalName << "_uvBar.y + uv.y))) / (1.0f - " << evalName << "_uvBar.y);" << endl
			<< "    if (x*x + y*y < 1)" << endl
			<< "        return " << evalName << "_color0;" << endl
			<< "    else" << endl
			<< "        return " << evalName << "_color1;" << endl
			<< "}" << endl;
	}

	void resolve(const GPUProgram *program, const std::string &evalName, std::vector<int> &parameterIDs) const {
		parameterIDs.push_back(program->getParameterID(evalName + "_color0", false));
		parameterIDs.push_back(program->getParameterID(evalName + "_color1", false));
		parameterIDs.push_back(program->getParameterID(evalName + "_uvBar", false));
		parameterIDs.push_back(program->getParameterID(evalName + "_uvOffset", false));
		parameterIDs.push_back(program->getParameterID(evalName + "_uvScale", false));
	}

	void bind(GPUProgram *program, const std::vector<int> &parameterIDs,
		int &textureUnitOffset) const {
		program->setParameter(parameterIDs[0], m_color0);
		program->setParameter(parameterIDs[1], m_color1);
		program->setParameter(parameterIDs[2], m_uvBar);
		program->setParameter(parameterIDs[3], m_uvOffset);
		program->setParameter(parameterIDs[4], m_uvScale);
	}

	MTS_DECLARE_CLASS()
private:
	Spectrum m_color0;
	Spectrum m_color1;
	Point2   m_uvBar;
	Point2   m_uvOffset;
	Vector2  m_uvScale;
};

Shader *ARCFShape::createShader(Renderer *renderer) const {
	return new ARCFShapeShader(renderer, m_color0, m_color1, m_uvBar,
		m_uvOffset, m_uvScale);
}

MTS_IMPLEMENT_CLASS(ARCFShapeShader, false, Shader)
MTS_IMPLEMENT_CLASS_S(ARCFShape, false, Texture2D)
MTS_EXPORT_PLUGIN(ARCFShape, "ARCFShape texture");
MTS_NAMESPACE_END
