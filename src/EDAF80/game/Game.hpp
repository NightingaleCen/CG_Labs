#pragma once

#include "Cake.hpp"
#include "Character.hpp"
#include "Ground.hpp"
#include "Monster.hpp"
#include "Tree.hpp"

#include <array>

namespace edaf80::game
{

enum class GameState
{
	Playing,
	Caught,
	Lost,
	Won
};

class Game
{
public:
	static constexpr unsigned int cake_count = 7u;
	static constexpr unsigned int tree_capacity = 512u;
	static constexpr float capture_turn_duration = 0.5f;
	static constexpr float capture_animation_duration = 1.75f;

	Game(float groundSize, unsigned int seed, float eyeHeight, float treeMinDistance = 4.0f,
	     const Monster::Settings& monsterSettings = {});
	~Game();

	void setPrograms(const GLuint* groundProgram, const GLuint* cakeProgram,
	                 const std::function<void(GLuint)>& setUniforms);
	void update(float deltaTimeSec, const glm::vec3& forwardDirection, InputHandler& ih);
	void render(const glm::mat4& viewProjection) const;
	void reset(unsigned int seed);

	Character& getCharacter() {return mCharacter;};
	const Monster& getMonster() const {return mMonster;};
	glm::vec3 getEyePosition() const; 

	GameState getState() const {return mState;};
	unsigned int getCollectedCount() const;
	float getCaptureTime() const {return mCaptureTime;};

private:
	bool isBlockedByTree(const glm::vec2& position) const;
	void updatePatrolTargets();

	float mGroundSize;
	unsigned int mSeed;
	float mTreeMinDistance;

	Ground mGround;
	Node mGroundNode;

	Character mCharacter;

	Cake::Geometry mCakeGeometry;
	std::array<Cake, cake_count> mCakes;

	Tree::Geometry mTreeGeometry;
	std::array<Tree, tree_capacity> mTrees;
	size_t mTreeCount = 0u;

	bonobo::mesh_data mMonsterGeometry;
	Monster mMonster;
	float mCaptureTime = 0.0f; // for capture animation

	float mElapsedTime = 0.0f;
	GameState mState = GameState::Playing;
};

}
