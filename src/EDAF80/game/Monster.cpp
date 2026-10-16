#include "Monster.hpp"
#include "Helpers.hpp"
#include "../interpolation.hpp"
#include "../parametric_shapes.hpp"

#include <algorithm>
#include <cmath>

edaf80::game::Monster::Monster(const Settings& settings) : mSettings{settings}
{
	mRoot.add_child(&mBody);
	for (auto& eye : mEyes)
		mRoot.add_child(&eye);
}

bonobo::mesh_data edaf80::game::Monster::createGeometry()
{
	// use the same sphere for the body and eyes
	return parametric_shapes::createSphere(1.0f, 24u, 16u);
}

void edaf80::game::Monster::setGeometry(const bonobo::mesh_data& geometry)
{
	// body
	mBody.set_geometry(geometry);
	mBody.get_transform().SetScale(mSettings.radius);
	bonobo::material_data material;
	material.diffuse = glm::vec3(0.85f, 0.88f, 0.82f);
	mBody.set_material_constants(material);

	// eyes
	material.diffuse = glm::vec3(0.06f, 0.03f, 0.03f);
	for (size_t i = 0u; i < mEyes.size(); ++i)
	{
		mEyes[i].set_geometry(geometry);
		mEyes[i].set_material_constants(material);
		mEyes[i].get_transform().SetScale(0.09f);
		mEyes[i].get_transform().SetTranslate(glm::vec3(
			i == 0u ? -0.16f : 0.16f, 0.12f, -0.9f * mSettings.radius));
	}
}

void edaf80::game::Monster::setProgram(const GLuint* program,
                                      const std::function<void(GLuint)>& setUniforms)
{
	const auto set_solid_uniforms = [setUniforms](GLuint shader) {
		setUniforms(shader);
		glUniform1i(glGetUniformLocation(shader, "use_layers"), 0);
	};
	mBody.set_program(program, set_solid_uniforms);
	for (auto& eye : mEyes)
		eye.set_program(program, set_solid_uniforms);
}

void edaf80::game::Monster::reset(const glm::vec2& position, float groundSize,
                                 unsigned int seed)
{
	mPosition = position;
	mGroundSize = groundSize;
	mHeading = 0.0f;
	mWaitTime = 0.0f;
	mPreviousCake = glm::vec2(-1.0f);
	mState = MonsterState::Wandering;
	mRandom.seed(seed);
}

void edaf80::game::Monster::setPatrolTargets(const std::vector<glm::vec2>& positions)
{
	mPatrolTargets = positions;
	if (mState == MonsterState::Wandering)
		choosePatrolPath();
}

void edaf80::game::Monster::choosePatrolPath()
{
	// choose a different cake when there is more than one left
	std::uniform_int_distribution<size_t> random_cake(0u, mPatrolTargets.size() - 1u);
	size_t cake_index;
	do {
		cake_index = random_cake(mRandom);
	} while (mPatrolTargets.size() > 1u && mPatrolTargets[cake_index] == mPreviousCake);
	mPreviousCake = mPatrolTargets[cake_index];

	// choose a random target near the cake
	std::uniform_real_distribution<float> random_angle(0.0f, glm::two_pi<float>());
	std::uniform_real_distribution<float> random_distance(2.0f, 5.0f);
	const float angle = random_angle(mRandom);
	const auto target = glm::clamp(mPreviousCake + random_distance(mRandom) *
		glm::vec2(std::cos(angle), std::sin(angle)), glm::vec2(mSettings.radius),
		glm::vec2(mGroundSize - mSettings.radius)); // don't go outside the ground!
	
	// we want a curve path so add an additional middle point
	const auto difference = target - mPosition;
	const glm::vec2 sideways(-difference.y, difference.x);
	std::uniform_real_distribution<float> random_bend(-0.2f, 0.2f);
	const auto middle = glm::mix(mPosition, target, 0.5f) + sideways * random_bend(mRandom);

	// sample two Catmull-Rom curves into a short path
	const auto to_world = [](const glm::vec2& position) {
		return glm::vec3(position.x, 0.0f, position.y);
	};
	const std::array<glm::vec3, 5> control_points{
		to_world(2.0f * mPosition - middle), to_world(mPosition), to_world(middle),
		to_world(target), to_world(2.0f * target - middle)
	};
	mPatrolPath[0] = mPosition;
	for (unsigned int curve = 0u; curve < 2u; ++curve) // two CR curves
	{
		for (unsigned int i = 1u; i <= samples_per_curve; ++i) // 32 samples per curve
		{
			const float progress = static_cast<float>(i) / static_cast<float>(samples_per_curve);
			const auto sample = interpolation::evalCatmullRom(control_points[curve],
				control_points[curve + 1u], control_points[curve + 2u],
				control_points[curve + 3u], 0.5f, progress); // sample on the CR curve
			mPatrolPath[curve * samples_per_curve + i] = glm::clamp(
				glm::vec2(sample.x, sample.z), glm::vec2(mSettings.radius),
				glm::vec2(mGroundSize - mSettings.radius));
		}
	}
	mPathIndex = 1u;
	mWaitTime = 0.0f;
}

void edaf80::game::Monster::moveAlongPath(float deltaTimeSec)
{
	// stay in place for a while
	if (mWaitTime > 0.0f) {
		mWaitTime -= deltaTimeSec;
		if (mWaitTime <= 0.0f)
			choosePatrolPath();
		return;
	}

	// move for deltaTimeSec along the path
	float remaining_distance = mSettings.patrolSpeed * deltaTimeSec; // fixed velocity
	glm::vec2 direction(0.0f);
	while (remaining_distance > 0.0f && mPathIndex < mPatrolPath.size())
	{
		const auto difference = mPatrolPath[mPathIndex] - mPosition;
		const float distance = glm::length(difference);
		if (distance > 0.0f)
			direction = difference / distance;
		if (distance <= remaining_distance) {
			mPosition = mPatrolPath[mPathIndex++];
			remaining_distance -= distance;
		} else {
			mPosition += direction * remaining_distance;
			remaining_distance = 0.0f;
		}
	}
	turnTowards(direction, mSettings.patrolTurnSpeed, deltaTimeSec);
	if (mPathIndex == mPatrolPath.size()) {
		std::uniform_real_distribution<float> random_wait(0.5f, 1.5f);
		mWaitTime = random_wait(mRandom);
	}
}

void edaf80::game::Monster::turnTowards(const glm::vec2& direction, float turnSpeed,
                                       float deltaTimeSec)
{
	if (glm::dot(direction, direction) == 0.0f)
		return;
	const float target_heading = std::atan2(-direction.x, -direction.y);
	const float difference = std::atan2(std::sin(target_heading - mHeading),
	                                   std::cos(target_heading - mHeading));
	const float turn = turnSpeed * deltaTimeSec;
	mHeading += glm::clamp(difference, -turn, turn);
}

bool edaf80::game::Monster::update(float deltaTimeSec, const glm::vec2& characterPosition,
                                  float characterRadius)
{
	const auto difference = characterPosition - mPosition;
	const float distance_squared = glm::dot(difference, difference);
	if (mState == MonsterState::Wandering &&
	    distance_squared <= mSettings.detectionRadius * mSettings.detectionRadius) {
		mState = MonsterState::Chasing;
	} else if (mState == MonsterState::Chasing &&
	           distance_squared > mSettings.chaseRadius * mSettings.chaseRadius) {
		mState = MonsterState::Wandering;
		choosePatrolPath();
	}

	if (mState == MonsterState::Wandering) {
		moveAlongPath(deltaTimeSec);
		const auto from_character = characterPosition - mPosition;
		const float new_distance_squared = glm::dot(from_character, from_character);
		if (new_distance_squared <= mSettings.detectionRadius * mSettings.detectionRadius)
			mState = MonsterState::Chasing;
		return new_distance_squared <= mSettings.captureRadius * mSettings.captureRadius; // return true if Chasing
	}

	// chase directly and stop at contact with the character
	const float distance = std::sqrt(distance_squared);
	const float movement = mSettings.chaseSpeed * deltaTimeSec;
	if (distance > mSettings.captureRadius)
		mPosition += difference / distance * std::min(movement, distance - mSettings.captureRadius);
	turnTowards(difference, mSettings.chaseTurnSpeed, deltaTimeSec);
	return distance <= mSettings.captureRadius + movement;
}

void edaf80::game::Monster::updatePose(float groundHeight, float elapsedTime)
{
	const float height = 0.9f + 0.08f * std::sin(2.0f * elapsedTime);
	mRoot.get_transform().SetTranslate(glm::vec3(mPosition.x, groundHeight + height, mPosition.y));
	mRoot.get_transform().SetRotateY(mHeading);
}

void edaf80::game::Monster::beginCapture(const glm::vec3& eyePosition)
{
	// save the position and direction for the capture animation
	mCapturePosition = mRoot.get_transform().GetTranslation();
	mCaptureDirection = glm::normalize(eyePosition - mCapturePosition);

	
	mRoot.get_transform().LookTowards(mCaptureDirection);
}

// the capture animation
void edaf80::game::Monster::updateCapture(float elapsedTime)
{
	const float shake_amplitude = 0.015f;
	const float approach_speed = 0.02f;
	std::uniform_real_distribution<float> random_offset(-shake_amplitude, shake_amplitude);
	const glm::vec3 shake(random_offset(mRandom), random_offset(mRandom), random_offset(mRandom));
	const auto approach = mCaptureDirection * approach_speed * elapsedTime;
	mRoot.get_transform().SetTranslate(mCapturePosition + shake + approach);
}

glm::vec3 edaf80::game::Monster::getFacePosition() const
{
	return glm::vec3(mRoot.get_transform().GetMatrix() *
	                 glm::vec4(0.0f, 0.12f, -mSettings.radius, 1.0f));
}

void edaf80::game::Monster::render(const glm::mat4& viewProjection) const
{
	renderSubtree(mRoot, viewProjection);
}
