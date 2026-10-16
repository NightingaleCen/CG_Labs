#pragma once

#include "core/node.hpp"

#include <array>
#include <random>
#include <vector>

namespace edaf80::game
{

enum class MonsterState
{
	Wandering,
	Chasing
};

class Monster
{
public:
	struct Settings
	{
		float patrolSpeed = 1.4f;
		float chaseSpeed = 3.2f;
		float detectionRadius = 14.0f;
		float chaseRadius = 16.0f;
		float patrolTurnSpeed = glm::radians(360.0f);
		float chaseTurnSpeed = glm::radians(900.0f);
		float captureRadius = 1.0f;
		float radius = 0.45f;
	};

	explicit Monster(const Settings& settings);

	~Monster() = default;
	Monster(const Monster&) = delete;
	Monster& operator=(const Monster&) = delete;
	Monster(Monster&&) = delete;
	Monster& operator=(Monster&&) = delete;

	void setGeometry(const bonobo::mesh_data& geometry);
	void setProgram(const GLuint* program, const std::function<void(GLuint)>& setUniforms);
	void reset(const glm::vec2& position, float groundSize, unsigned int seed);
	void setPatrolTargets(const std::vector<glm::vec2>& positions);
	bool update(float deltaTimeSec, const glm::vec2& characterPosition, float characterRadius);
	void updatePose(float groundHeight, float elapsedTime);
	void beginCapture(const glm::vec3& eyePosition);
	void updateCapture(float elapsedTime);
	void render(const glm::mat4& viewProjection) const;

	glm::vec2 getPosition() const {return mPosition;};
	glm::vec3 getFacePosition() const;
	MonsterState getState() const {return mState;};
	static bonobo::mesh_data createGeometry();

private:
	static constexpr unsigned int samples_per_curve = 32u;

	void choosePatrolPath();
	void moveAlongPath(float deltaTimeSec);
	void turnTowards(const glm::vec2& direction, float turnSpeed, float deltaTimeSec);

	Settings mSettings;
	Node mRoot;
	Node mBody;
	std::array<Node, 2> mEyes;

	glm::vec2 mPosition{0.0f};
	glm::vec3 mCapturePosition{0.0f};
	glm::vec3 mCaptureDirection{0.0f};
	glm::vec2 mPreviousCake{-1.0f}; // previous patrol target
	float mHeading = 0.0f;			// the angle the monster is facing. 0 is along the negative z-axis
	float mGroundSize = 0.0f;		// needed to ensure the monster doesn't go outside the ground
	float mWaitTime = 0.0f;			// the time to wait before moving again

	MonsterState mState = MonsterState::Wandering;
	std::mt19937 mRandom;
	std::vector<glm::vec2> mPatrolTargets;
	std::array<glm::vec2, 2u * samples_per_curve + 1u> mPatrolPath; // the path the monster is currently following
	size_t mPathIndex = 1u; // the index of the next point in the path to move towards
};

}
