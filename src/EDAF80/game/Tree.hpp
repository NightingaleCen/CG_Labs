#pragma once

#include "core/node.hpp"

#include <array>
#include <vector>

namespace edaf80::game
{

class Tree
{
public:
	struct Geometry
	{
		bonobo::mesh_data trunk;
		bonobo::mesh_data crown;
	};

	Tree();

	~Tree() = default;
	Tree(const Tree&) = delete;
	Tree& operator=(const Tree&) = delete;
	Tree(Tree&&) = delete;
	Tree& operator=(Tree&&) = delete;

	void setGeometry(const Geometry& geometry);
	void setProgram(const GLuint* program, const std::function<void(GLuint)>& setUniforms);
	void reset(const glm::vec3& position, const glm::vec3& scale, float rotation);
	bool overlaps(const glm::vec2& position, float radius) const;
	void render(const glm::mat4& viewProjection) const;

	static Geometry createGeometry();
	static std::vector<glm::vec2> generatePositions(float width, float depth,
	                                               float minDistance, unsigned int seed);

private:
	static constexpr float trunk_radius = 0.25f;

	Node mRoot;
	Node mTrunk;
	std::array<Node, 3> mCrowns;
	glm::vec2 mPosition{0.0f};
	float mRadius = trunk_radius;
};

}
