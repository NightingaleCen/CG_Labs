#include "Cake.hpp"
#include "Helpers.hpp"
#include "../parametric_shapes.hpp"

#include <cmath>
#include <random>
#include <stdexcept>

edaf80::game::Cake::Cake()
{
	mRoot.add_child(&mPlate);
	mRoot.add_child(&mBody);
	mRoot.add_child(&mCherry);
}

edaf80::game::Cake::Geometry edaf80::game::Cake::createGeometry()
{
	// Create the plate
	const std::vector<glm::vec2> plate_profile{
		{0.0f, 0.0f}, {0.34f, 0.0f}, {0.44f, 0.025f}, {0.50f, 0.06f},
		{0.50f, 0.08f}, {0.44f, 0.045f}, {0.34f, 0.02f}, {0.0f, 0.02f}
	};

	// Create the cake body
	const std::vector<glm::vec2> body_profile{
		{0.0f, 0.0f}, {0.32f, 0.0f}, {0.32f, 0.22f}, {0.0f, 0.22f}
	};

	Geometry geometry;
	geometry.plate = createRevolvedShape(plate_profile, 32u);
	geometry.body = createRevolvedShape(body_profile, 32u);
	geometry.cherry = parametric_shapes::createSphere(0.055f, 12u, 8u);
	return geometry;
}

void edaf80::game::Cake::setGeometry(const Geometry& geometry)
{
	mPlate.set_geometry(geometry.plate);
	mBody.set_geometry(geometry.body);
	mCherry.set_geometry(geometry.cherry);

	bonobo::material_data material;
	material.diffuse = glm::vec3(0.87f, 0.88f, 0.82f);
	mPlate.set_material_constants(material);
	material.diffuse = glm::vec3(0.62f, 0.35f, 0.15f);
	mBody.set_material_constants(material);
	material.diffuse = glm::vec3(0.75f, 0.07f, 0.10f);
	mCherry.set_material_constants(material);

	mBody.get_transform().SetTranslate(glm::vec3(0.0f, 0.02f, 0.0f));
	mCherry.get_transform().SetTranslate(glm::vec3(0.14f, 0.29f, 0.08f));
}

void edaf80::game::Cake::setProgram(const GLuint* program,
                                   const std::function<void(GLuint)>& setUniforms)
{
	const auto set_solid_uniforms = [setUniforms](GLuint shader) {
		setUniforms(shader);
		glUniform1i(glGetUniformLocation(shader, "use_layers"), 0);
	};
	mPlate.set_program(program, set_solid_uniforms);
	mCherry.set_program(program, set_solid_uniforms);
	mBody.set_program(program, [setUniforms](GLuint shader) {
		setUniforms(shader);
		glUniform1i(glGetUniformLocation(shader, "use_layers"), 1); // the cake body has an extra layer
	});
}

std::vector<glm::vec2> edaf80::game::Cake::generatePositions(unsigned int count,
                                                          float width, float depth,
                                                          float minDistance,
                                                          const glm::vec2& spawn,
                                                          unsigned int seed)
{
	std::mt19937 random(seed);
	std::uniform_real_distribution<float> random_x(1.0f, width - 1.0f);
	std::uniform_real_distribution<float> random_z(1.0f, depth - 1.0f);
	std::vector<glm::vec2> positions;

	for (unsigned int attempt = 0u; attempt < 10000u && positions.size() < count; ++attempt)
	{
		const glm::vec2 candidate(random_x(random), random_z(random));
		const auto from_spawn = candidate - spawn;
		bool accepted = glm::dot(from_spawn, from_spawn) >= 9.0f; // at least 9 units away from the spawn position

		for (const auto& position : positions)
		{
			const auto difference = candidate - position;
			if (glm::dot(difference, difference) < minDistance * minDistance) {
				accepted = false;
				break;
			}
		}

		if (accepted)
			positions.push_back(candidate);
	}

	if (positions.size() != count)
		throw std::runtime_error("Failed to place all cakes.");

	return positions;
}

void edaf80::game::Cake::reset(const glm::vec3& position, float phase)
{
	mPosition = position;
	mPhase = phase;
	mCollected = false;
	update(0.0f);
}

void edaf80::game::Cake::update(float elapsedTime)
{
	const float height = 0.40f + 0.08f * std::sin(2.0f * elapsedTime + mPhase);
	mRoot.get_transform().SetTranslate(mPosition + glm::vec3(0.0f, height, 0.0f));
	mRoot.get_transform().SetRotateY(elapsedTime + mPhase);
}

bool edaf80::game::Cake::tryCollect(const glm::vec2& characterPosition, float radius)
{
	const auto difference = characterPosition - glm::vec2(mPosition.x, mPosition.z);
	if (!mCollected && glm::dot(difference, difference) < radius * radius) {
		mCollected = true;
		return true;
	}
	return false;
}

void edaf80::game::Cake::render(const glm::mat4& viewProjection) const
{
	if (!mCollected)
		renderSubtree(mRoot, viewProjection);
}
