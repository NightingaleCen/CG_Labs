#include "Helpers.hpp"

#include <cmath>
#include <vector>

bonobo::mesh_data edaf80::game::createRevolvedShape(const std::vector<glm::vec2>& profile,
                                                  unsigned int resolution)
{
	const auto ring_vertices = resolution + 1u;
	const auto band_count = profile.size() - 1u;
	const auto vertices_nb = band_count * 2u * ring_vertices;

	auto vertices = std::vector<glm::vec3>(vertices_nb);
	auto normals = std::vector<glm::vec3>(vertices_nb);
	auto index_sets = std::vector<glm::uvec3>();
	index_sets.reserve(band_count * resolution * 2u);

	// generate vertices
	for (size_t j = 0u; j < band_count; ++j)
	{
		// Takes an edge and rotate it along the y axis to form the shape
		const auto tangent = profile[j + 1u] - profile[j];
		for (unsigned int i = 0u; i <= resolution; ++i)
		{
			const float theta = glm::two_pi<float>() * static_cast<float>(i) /
			                    static_cast<float>(resolution);
			const float cos_theta = std::cos(theta);
			const float sin_theta = std::sin(theta);
			const auto normal = glm::normalize(glm::vec3(tangent.y * cos_theta,
			                                            -tangent.x, tangent.y * sin_theta));

			for (unsigned int side = 0u; side < 2u; ++side)
			{
				const auto index = (2u * j + side) * ring_vertices + i;
				const auto point = profile[j + side];
				vertices[index] = glm::vec3(point.x * cos_theta, point.y, point.x * sin_theta);
				normals[index] = normal;
			}
		}
	}

	// generate indices
	for (size_t j = 0u; j < band_count; ++j)
	{
		for (unsigned int i = 0u; i < resolution; ++i)
		{
			const auto a = static_cast<unsigned int>(2u * j * ring_vertices + i);
			const auto b = a + ring_vertices;
			if (profile[j].x > 0.0f)
				index_sets.emplace_back(a, b, a + 1u);
			if (profile[j + 1u].x > 0.0f)
				index_sets.emplace_back(a + 1u, b, b + 1u);
		}
	}

	bonobo::mesh_data mesh;
	glGenVertexArrays(1, &mesh.vao);
	glBindVertexArray(mesh.vao);

	const auto vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	const auto normals_offset = vertices_size;
	const auto normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));

	glGenBuffers(1, &mesh.bo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.bo);
	glBufferData(GL_ARRAY_BUFFER, vertices_size + normals_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_size, vertices.data());
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices),
	                      3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, normals.data());
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals),
	                      3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<const GLvoid*>(normals_offset));

	mesh.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &mesh.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)),
	             index_sets.data(), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	return mesh;
}

void edaf80::game::renderSubtree(const Node& root, const glm::mat4& viewProjection,
                                const glm::mat4& parentTransform)
{
	struct NodeRef
	{
		const Node* node;
		glm::mat4 parent_transform;
	};

	std::vector<NodeRef> node_stack{
		{
			&root,
			parentTransform
		}
	};

	NodeRef current_node;
	glm::mat4 transform_for_child;
	while (!node_stack.empty()) {
		current_node = node_stack.back();
		node_stack.pop_back();

		current_node.node->render(viewProjection, current_node.parent_transform);
		transform_for_child = current_node.parent_transform *
		                      current_node.node->get_transform().GetMatrix();

		for (size_t i = 0u; i < current_node.node->get_children_nb(); ++i)
		{
			node_stack.push_back({
				current_node.node->get_child(i),
				transform_for_child
			});
		}
	}
}
