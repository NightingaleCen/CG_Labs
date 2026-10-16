#include "Ground.hpp"
#include "external/PerlinNoise.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>
#include <stdexcept>

edaf80::game::Ground::Ground(float width, float depth,
                            unsigned int resolution,
                            float amplitude, unsigned int seed) :
	mWidth{width}, mDepth{depth}, mResolution{resolution}, mAmplitude{amplitude}, mNoise{seed}
{
	if (width <= 0.0f || depth <= 0.0f || resolution == 0u)
		throw std::invalid_argument("Invalid ground parameters!");

	auto const vertices = std::array<glm::vec3, 4>{
		glm::vec3(0.0f,  0.0f,   0.0f),
		glm::vec3(mWidth, 0.0f,   0.0f),
		glm::vec3(mWidth, 0.0f, mDepth),
		glm::vec3(0.0f,  0.0f, mDepth)
	};

	auto const index_sets = std::array<glm::uvec3, 2>{
		glm::uvec3(0u, 2u, 1u),
		glm::uvec3(0u, 3u, 2u)
	};

	auto tessellated_vertices = std::vector<glm::vec3>(static_cast<size_t>((mResolution + 1u) * (mResolution + 1u)));
	auto tessellated_normals = std::vector<glm::vec3>(static_cast<size_t>((mResolution + 1u) * (mResolution + 1u)));
	auto tessellated_texcoords = std::vector<glm::vec3>(static_cast<size_t>((mResolution + 1u) * (mResolution + 1u)));
	
	// generate vertices
	size_t index = 0u;
	for (unsigned int i = 0u; i <= mResolution; ++i)
	{
		for (unsigned int j = 0u; j <= mResolution; ++j)
		{
			auto const x = static_cast<float>(i) / static_cast<float>(mResolution);
			auto const z = static_cast<float>(j) / static_cast<float>(mResolution);

			tessellated_vertices[index] = glm::vec3(x * mWidth, heightAt(x * mWidth, z * mDepth), z * mDepth);
			tessellated_normals[index] = normalAt(x * mWidth, z * mDepth);
			tessellated_texcoords[index] = glm::vec3(x, z, 0.0f);

			++index;
		}
	}

	// generate indices
	auto tessellated_index_sets = std::vector<glm::uvec3>(static_cast<size_t>(mResolution * mResolution * 2u));
	const auto vertical_vertex_count = mResolution + 1;
	index = 0u;
	for (unsigned int i = 0u; i < mResolution; ++i)
	{
		for (unsigned int j = 0u; j < mResolution; ++j)
		{
			tessellated_index_sets[index] = glm::uvec3(vertical_vertex_count * (i + 0u) + (j + 0u),
											vertical_vertex_count * (i + 0u) + (j + 1u),
											vertical_vertex_count * (i + 1u) + (j + 0u));
			++index;

			tessellated_index_sets[index] = glm::uvec3(vertical_vertex_count * (i + 1u) + (j + 0u),
											vertical_vertex_count * (i + 0u) + (j + 1u),
											vertical_vertex_count * (i + 1u) + (j + 1u));
			++index;
		}
	}

	glGenVertexArrays(1, &mesh.vao);
	assert(mesh.vao != 0u);
	glBindVertexArray(mesh.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(tessellated_vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_offset + vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(tessellated_normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(tessellated_texcoords.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
												+normals_size
												+texcoords_size
												);

	glGenBuffers(1, &mesh.bo);
	assert(mesh.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(tessellated_vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(tessellated_normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(tessellated_texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	mesh.indices_nb = static_cast<GLsizei>(tessellated_index_sets.size() * 3u);
	glGenBuffers(1, &mesh.ibo);
	assert(mesh.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(tessellated_index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(tessellated_index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
}

float edaf80::game::Ground::heightAt(float x, float z, float frequency) const
{
	if (x < 0.0f || x > mWidth || z < 0.0f || z > mDepth)
	{
		std::ostringstream message;
		message << "Coordinate (x=" << x << ", z=" << z
				<< ") out of bounds when computing height; expected x in [0, "
				<< mWidth << "] and z in [0, " << mDepth << "].";
		throw std::invalid_argument(message.str());
	}

	return mAmplitude * mNoise.octave2D_01(x * frequency, z * frequency, 2);
}

glm::vec3 edaf80::game::Ground::normalAt(float x, float z, float epsilon) const
{
	if (x < 0.0f || x > mWidth || z < 0.0f || z > mDepth)
	{
		std::ostringstream message;
		message << "Coordinate (x=" << x << ", z=" << z
				<< ") out of bounds when computing normal; expected x in [0, "
				<< mWidth << "] and z in [0, " << mDepth << "].";
		throw std::invalid_argument(message.str());
	}

	if (epsilon <= 0.0f)
		throw std::invalid_argument("Epsilon must be positive when computing normal.");

	auto const xL = std::max(0.0f, x - epsilon);
	auto const xR = std::min(mWidth, x + epsilon);
	auto const zD = std::max(0.0f, z - epsilon);
	auto const zU = std::min(mDepth, z + epsilon);

	auto const hL = heightAt(xL, z);
	auto const hR = heightAt(xR, z);
	auto const hD = heightAt(x, zD);
	auto const hU = heightAt(x, zU);

	auto const dx = xR - xL;
	auto const dz = zU - zD;
	return glm::normalize(glm::vec3((hL - hR) / dx, 2.0f, (hD - hU) / dz));
}