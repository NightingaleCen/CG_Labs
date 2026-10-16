#include "Game.hpp"

#include <algorithm>
#include <random>

edaf80::game::Game::Game(float groundSize, unsigned int seed, float eyeHeight,
                        float treeMinDistance, const Monster::Settings& monsterSettings) :
	mGroundSize{groundSize}, mSeed{seed}, mTreeMinDistance{treeMinDistance},
	mGround{groundSize, groundSize, 160u, 0.6f, seed}, mCharacter{eyeHeight},
	mCakeGeometry{Cake::createGeometry()}, mTreeGeometry{Tree::createGeometry()},
	mMonsterGeometry{Monster::createGeometry()}, mMonster{monsterSettings}
{
	mGroundNode.set_geometry(mGround.getMesh());
	for (auto& cake : mCakes)
		cake.setGeometry(mCakeGeometry);
	for (auto& tree : mTrees)
		tree.setGeometry(mTreeGeometry);
	mMonster.setGeometry(mMonsterGeometry);
	reset(mSeed);
}

edaf80::game::Game::~Game()
{
	const auto ground_mesh = mGround.getMesh();
	const std::array<const bonobo::mesh_data*, 7> meshes{
		&ground_mesh, &mCakeGeometry.plate, &mCakeGeometry.body, &mCakeGeometry.cherry,
		&mTreeGeometry.trunk, &mTreeGeometry.crown, &mMonsterGeometry
	};
	for (const auto* mesh : meshes)
	{
		glDeleteVertexArrays(1, &mesh->vao);
		glDeleteBuffers(1, &mesh->bo);
		glDeleteBuffers(1, &mesh->ibo);
	}
}

void edaf80::game::Game::setPrograms(const GLuint* groundProgram, const GLuint* cakeProgram,
                                   const std::function<void(GLuint)>& setUniforms)
{
	mGroundNode.set_program(groundProgram, setUniforms);
	for (auto& cake : mCakes)
		cake.setProgram(cakeProgram, setUniforms);
	for (auto& tree : mTrees)
		tree.setProgram(cakeProgram, setUniforms); // not really a cake but it works!
	mMonster.setProgram(cakeProgram, setUniforms);
}

void edaf80::game::Game::reset(unsigned int seed)
{
	mSeed = seed;
	mCharacter.setPosition(mGroundSize / 2.0f, mGroundSize / 2.0f); // start in the middle of the ground
	mElapsedTime = 0.0f;
	mCaptureTime = 0.0f;
	mState = GameState::Playing;

	const auto positions = Cake::generatePositions(cake_count, mGroundSize, mGroundSize,
	                                               12.0f, mCharacter.getPosition(), mSeed);
	for (size_t i = 0u; i < mCakes.size(); ++i)
	{
		const auto position = positions[i];
		const float height = mGround.heightAt(position.x, position.y);
		const float phase = glm::two_pi<float>() * static_cast<float>(i) /
		                    static_cast<float>(cake_count);
		mCakes[i].reset(glm::vec3(position.x, height, position.y), phase);
	}

	// place the trees
	auto tree_positions = Tree::generatePositions(mGroundSize, mGroundSize,
	                                              mTreeMinDistance, mSeed + 1u);
	const auto spawn = mCharacter.getPosition();
	// leave space around the character and cakes
	tree_positions.erase(std::remove_if(tree_positions.begin(), tree_positions.end(),
		[&positions, &spawn](const glm::vec2& position) {
			const auto from_spawn = position - spawn;
			if (glm::dot(from_spawn, from_spawn) < 16.0f)
				return true;
			for (const auto& cake_position : positions)
			{
				const auto difference = position - cake_position;
				if (glm::dot(difference, difference) < 6.25f)
					return true;
			}
			return false;
		}), tree_positions.end());

	// have some randomness in tree shape
	std::mt19937 random(mSeed + 2u);
	std::shuffle(tree_positions.begin(), tree_positions.end(), random);
	std::uniform_real_distribution<float> random_width(0.95f, 1.05f);
	std::uniform_real_distribution<float> random_height(0.90f, 1.10f);
	std::uniform_real_distribution<float> random_rotation(0.0f, glm::two_pi<float>());
	mTreeCount = std::min(tree_positions.size(), mTrees.size());
	for (size_t i = 0u; i < mTreeCount; ++i)
	{
		const auto position = tree_positions[i];
		const float height = mGround.heightAt(position.x, position.y);

		const float width_scale = random_width(random);
		const float height_scale = random_height(random);
		const glm::vec3 scale(width_scale, height_scale, width_scale);
		const float rotation = random_rotation(random);
		
		mTrees[i].reset(glm::vec3(position.x, height, position.y), scale, rotation);
	}

	// start the monster near the cake furthest from the character
	const auto monster_spawn = *std::max_element(positions.begin(), positions.end(),
		[&spawn](const glm::vec2& a, const glm::vec2& b) {
			return glm::dot(a - spawn, a - spawn) < glm::dot(b - spawn, b - spawn);
		});
	mMonster.reset(monster_spawn, mGroundSize, mSeed + 3u);
	updatePatrolTargets();
	mMonster.updatePose(mGround.heightAt(monster_spawn.x, monster_spawn.y), mElapsedTime);
}

void edaf80::game::Game::update(float deltaTimeSec, const glm::vec3& forwardDirection,
                               InputHandler& ih)
{
	if (mState == GameState::Caught) {
		mCaptureTime += deltaTimeSec;
		if (mCaptureTime >= capture_turn_duration)
			//play the capture animation
			mMonster.updateCapture(std::min(mCaptureTime - capture_turn_duration,
			                                capture_animation_duration));
		if (mCaptureTime >= capture_turn_duration + capture_animation_duration)
			mState = GameState::Lost;
		return;
	}
	if (mState != GameState::Playing)
		return;

	mElapsedTime += deltaTimeSec;
	const auto previous_position = mCharacter.getPosition();
	mCharacter.move(deltaTimeSec, forwardDirection, ih);

	// keep the character inside the ground
	const float radius = mCharacter.getRadius();
	const auto target_position = glm::clamp(mCharacter.getPosition(), glm::vec2(radius),
	                                        glm::vec2(mGroundSize - radius));
	const auto position = mCharacter.moveAroundTrees(previous_position, target_position,
		[this](const glm::vec2& candidate) {return isBlockedByTree(candidate);});
	mCharacter.setPosition(position.x, position.y);

	// update the cakes and check for pickups
	bool collected_cake = false;
	for (auto& cake : mCakes)
	{
		cake.update(mElapsedTime);
		collected_cake |= cake.tryCollect(position, 1.0f);
	}

	if (getCollectedCount() == cake_count) {
		mState = GameState::Won;
		return;
	}
	if (collected_cake)
		updatePatrolTargets();

	// update the monster and check for capture
	const bool caught = mMonster.update(deltaTimeSec, position, radius);
	const auto monster_position = mMonster.getPosition();
	mMonster.updatePose(mGround.heightAt(monster_position.x, monster_position.y), mElapsedTime);
	if (caught) {
		mState = GameState::Caught;
		mCaptureTime = 0.0f;
		mMonster.beginCapture(getEyePosition());
	}
}

void edaf80::game::Game::render(const glm::mat4& viewProjection) const
{
	mGroundNode.render(viewProjection);
	for (size_t i = 0u; i < mTreeCount; ++i)
		mTrees[i].render(viewProjection);
	for (const auto& cake : mCakes)
		cake.render(viewProjection);
	mMonster.render(viewProjection);
}

void edaf80::game::Game::updatePatrolTargets()
{
	std::vector<glm::vec2> positions;
	for (const auto& cake : mCakes)
	{
		if (!cake.isCollected()) {
			const auto position = cake.getPosition();
			positions.emplace_back(position.x, position.z);
		}
	}
	mMonster.setPatrolTargets(positions);
}

bool edaf80::game::Game::isBlockedByTree(const glm::vec2& position) const
{
	const float radius = mCharacter.getRadius();
	return std::any_of(mTrees.begin(), mTrees.begin() + mTreeCount,
		[&position, radius](const Tree& tree) {return tree.overlaps(position, radius);});
}

glm::vec3 edaf80::game::Game::getEyePosition() const
{
	const auto position = mCharacter.getPosition();
	return glm::vec3(position.x, mGround.heightAt(position.x, position.y) +
	                            mCharacter.getEyeHeight(), position.y);
}

unsigned int edaf80::game::Game::getCollectedCount() const
{
	return static_cast<unsigned int>(std::count_if(mCakes.begin(), mCakes.end(),
		[](const Cake& cake) {return cake.isCollected();}));
}
