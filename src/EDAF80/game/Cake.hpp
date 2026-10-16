#pragma once

#include "core/node.hpp"

#include <vector>

namespace edaf80::game
{

class Cake
{
public:
	struct Geometry
	{
		bonobo::mesh_data plate;
		bonobo::mesh_data body;
		bonobo::mesh_data cherry;
	};

	Cake();

	~Cake() = default;
	Cake(const Cake&) = delete;
	Cake& operator=(const Cake&) = delete;
	Cake(Cake&&) = delete;
	Cake& operator=(Cake&&) = delete;

	void setGeometry(const Geometry& geometry);
	void setProgram(const GLuint* program, const std::function<void(GLuint)>& setUniforms);
	void reset(const glm::vec3& position, float phase);
	void update(float elapsedTime);
	bool tryCollect(const glm::vec2& characterPosition, float radius);
	void render(const glm::mat4& viewProjection) const;
	bool isCollected() const {return mCollected;};
	glm::vec3 getPosition() const {return mPosition;};

	// create the geometry and generate the positions only once
	static Geometry createGeometry();
	static std::vector<glm::vec2> generatePositions(unsigned int count, float width,
	                                               float depth, float minDistance,
	                                               const glm::vec2& spawn,
	                                               unsigned int seed);

private:
	Node mRoot;
	Node mPlate;
	Node mBody;
	Node mCherry;
	glm::vec3 mPosition{0.0f};
	float mPhase = 0.0f;
	bool mCollected = false;
};

}
