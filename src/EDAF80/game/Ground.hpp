#pragma once

#include "core/helpers.hpp"
#include "external/PerlinNoise.hpp"


#include <vector>

namespace edaf80::game
{

class Ground
{
public:
	Ground(float width = 40.0f, float depth = 40.0f,
	       unsigned int resolution = 80u,
	       float amplitude = 0.9f, unsigned int seed = 42u);

	float heightAt(float x, float z, float frequency = 0.5) const;
	glm::vec3 normalAt(float x, float z, float epsilon = 1e-3f) const;

	bonobo::mesh_data getMesh() const {return mesh;};

private:
	float mWidth;
	float mDepth;
	float mAmplitude;
	unsigned int mResolution;

	const siv::PerlinNoise mNoise;
	
	bonobo::mesh_data mesh;
};

}
