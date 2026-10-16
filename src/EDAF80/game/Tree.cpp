#include "Tree.hpp"
#include "Helpers.hpp"

#include <algorithm>
#include <cmath>
#include "external/PoissonGenerator.h"

edaf80::game::Tree::Tree()
{
	mRoot.add_child(&mTrunk);
	for (auto& crown : mCrowns)
		mRoot.add_child(&crown);
}

edaf80::game::Tree::Geometry edaf80::game::Tree::createGeometry()
{
	// the trunk
	const std::vector<glm::vec2> trunk_profile{
		{0.0f, 0.0f}, {trunk_radius, 0.0f}, {0.10f, 4.2f}, {0.0f, 4.2f}
	};

	// the crown
	const std::vector<glm::vec2> crown_profile{
		{0.0f, 0.0f}, {1.5f, 0.0f}, {0.0f, 2.1f}
	};

	Geometry geometry;
	geometry.trunk = createRevolvedShape(trunk_profile, 8u);
	geometry.crown = createRevolvedShape(crown_profile, 6u);
	return geometry;
}

void edaf80::game::Tree::setGeometry(const Geometry& geometry)
{
	mTrunk.set_geometry(geometry.trunk);

	bonobo::material_data material;
	material.diffuse = glm::vec3(0.30f, 0.17f, 0.08f);
	mTrunk.set_material_constants(material);

	// stack three crowns on top of each other
	for (size_t i = 0u; i < mCrowns.size(); ++i)
	{
		const float level = static_cast<float>(i);
		mCrowns[i].set_geometry(geometry.crown);
		material.diffuse = glm::vec3(0.08f, 0.25f + 0.025f * level, 0.12f);
		mCrowns[i].set_material_constants(material);
		mCrowns[i].get_transform().SetTranslate(glm::vec3(0.0f, 1.5f + 1.2f * level, 0.0f));
		mCrowns[i].get_transform().SetScale(1.0f - 0.2f * level);
	}
}

void edaf80::game::Tree::setProgram(const GLuint* program,
                                   const std::function<void(GLuint)>& setUniforms)
{
	const auto set_solid_uniforms = [setUniforms](GLuint shader) {
		setUniforms(shader);
		glUniform1i(glGetUniformLocation(shader, "use_layers"), 0);
	};
	mTrunk.set_program(program, set_solid_uniforms);
	for (auto& crown : mCrowns)
		crown.set_program(program, set_solid_uniforms);
}

std::vector<glm::vec2> edaf80::game::Tree::generatePositions(float width, float depth,
                                                          float minDistance,
                                                          unsigned int seed)
{
	// generate points in a square and convert them to ground coordinates
	const float map_size = std::max(width, depth);
	const auto estimated_count = static_cast<unsigned int>(std::ceil(
		2.0f * map_size * map_size / (minDistance * minDistance)));
	PoissonGenerator::DefaultPRNG random(seed);
	const auto samples = PoissonGenerator::generatePoissonPoints(
		estimated_count, random, false, 30u, minDistance / map_size);

	std::vector<glm::vec2> positions;
	positions.reserve(samples.size());
	for (const auto& sample : samples)
	{
		const glm::vec2 position(sample.x * map_size, sample.y * map_size);
		if (position.x >= 1.0f && position.x < width - 1.0f &&
		    position.y >= 1.0f && position.y < depth - 1.0f)
			positions.push_back(position);
	}
	return positions;
}

void edaf80::game::Tree::reset(const glm::vec3& position, const glm::vec3& scale, float rotation)
{
	mPosition = glm::vec2(position.x, position.z);
	mRadius = trunk_radius * scale.x;
	mRoot.get_transform().SetTranslate(position - glm::vec3(0.0f, 0.10f, 0.0f));
	mRoot.get_transform().SetScale(scale);
	mRoot.get_transform().SetRotateY(rotation);
}

bool edaf80::game::Tree::overlaps(const glm::vec2& position, float radius) const
{
	const auto difference = position - mPosition;
	const float combined_radius = mRadius + radius;
	return glm::dot(difference, difference) < combined_radius * combined_radius;
}

void edaf80::game::Tree::render(const glm::mat4& viewProjection) const
{
	renderSubtree(mRoot, viewProjection);
}
