#include "assignment5.hpp"
#include "game/Game.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>
#include <random>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// Global game settings
	const float ground_size = 100.0f;
	std::mt19937 random_generator(std::random_device{}());
	unsigned int random_seed = random_generator();

	// Tree settings
	const float tree_min_distance = 4.0f;

	// Character settings
	const float character_eye_height = 0.9f;
	const float character_walk_speed = 2.0f;
	const float character_run_speed = 4.26767f;

	// Monster settings
	const game::Monster::Settings monster_settings;

	// Lighting settings
	bool night_lighting = true;
	const glm::vec3 flashlight_color(1.0f, 0.94f, 0.80f);
	const float flashlight_range = 30.0f;
	const float flashlight_inner_angle = glm::radians(12.0f);
	const float flashlight_outer_angle = glm::radians(20.0f);
	const float flashlight_brightness = 4.0f;

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(20.0f, 20.0f, 0.0f)); // height will be set later
	mCamera.mWorld.LookTowards(glm::vec3(0.0f, 0.0f, -1.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(6.0f); // speed for fly mode

	// Set up the charecter
	game::Game game(ground_size, random_seed, character_eye_height, tree_min_distance, monster_settings);
	auto& character = game.getCharacter();
	character.setSpeed(character_walk_speed);
	mCamera.mWorld.SetTranslate(game.getEyePosition());

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint ground_shader = 0u;
	program_manager.CreateAndRegisterProgram("Ground",
	                                         { { ShaderType::vertex, "EDAF80/ground.vert" },
	                                           { ShaderType::fragment, "EDAF80/ground.frag" } },
	                                         ground_shader);
	if (ground_shader == 0u) {
		LogError("Failed to load ground shader");
		return;
	}

	GLuint cake_shader = 0u;
	program_manager.CreateAndRegisterProgram("Cake",
	                                         { { ShaderType::vertex, "EDAF80/cake.vert" },
	                                           { ShaderType::fragment, "EDAF80/cake.frag" } },
	                                         cake_shader);
	if (cake_shader == 0u) {
		LogError("Failed to load cake shader");
		return;
	}

	// Create the game objects
	auto const light_direction = glm::normalize(glm::vec3(-0.4f, 1.0f, 0.3f));
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto camera_direction = mCamera.mWorld.GetFront();

	auto const set_uniforms = [&light_direction, &camera_position, &camera_direction, &night_lighting,
	                           &flashlight_color, &flashlight_range, &flashlight_inner_angle,
	                           &flashlight_outer_angle, &flashlight_brightness](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_direction"), 1, glm::value_ptr(light_direction));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "camera_direction"), 1, glm::value_ptr(camera_direction));
		glUniform1i(glGetUniformLocation(program, "night_lighting"), night_lighting);
		glUniform3fv(glGetUniformLocation(program, "flashlight_color"), 1, glm::value_ptr(flashlight_color));
		glUniform1f(glGetUniformLocation(program, "flashlight_range"), flashlight_range);
		glUniform1f(glGetUniformLocation(program, "flashlight_inner_angle"), flashlight_inner_angle);
		glUniform1f(glGetUniformLocation(program, "flashlight_outer_angle"), flashlight_outer_angle);
		glUniform1f(glGetUniformLocation(program, "flashlight_brightness"), flashlight_brightness);
	};

	game.setPrograms(&ground_shader, &cake_shader, set_uniforms);

	glClearDepthf(1.0f);
	glEnable(GL_DEPTH_TEST);


	// Set up some helper variables
	auto lastTime = std::chrono::high_resolution_clock::now();

	bool show_logs = false;
	bool show_gui = true;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool shader_reload_failed = false;
	bool show_basis = false;
	bool fly_mode = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;
	glm::quat capture_start_rotation(1.0f, 0.0f, 0.0f, 0.0f);
	glm::quat capture_target_rotation(1.0f, 0.0f, 0.0f, 0.0f);

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		auto const deltaTimeSec = std::chrono::duration_cast<std::chrono::duration<float>>(deltaTimeUs);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		// Handle input events and character movement
		glfwPollEvents();
		inputHandler.Advance();
		const bool controls_locked = game.getState() == game::GameState::Caught ||
		                             game.getState() == game::GameState::Lost;
		if (fly_mode && !controls_locked) {
			mCamera.Update(deltaTimeUs, inputHandler);
		} else {
			if (inputHandler.GetKeycodeState(GLFW_KEY_LEFT_SHIFT) & PRESSED) {
				character.setSpeed(character_run_speed);
			} else {
				character.setSpeed(character_walk_speed);
			}
			mCamera.Update(deltaTimeUs, inputHandler, true, controls_locked); // keyboard disabled and handled in game.update()
			const auto previous_state = game.getState();
			game.update(deltaTimeSec.count(), mCamera.mWorld.GetFront(), inputHandler);


			mCamera.mWorld.SetTranslate(game.getEyePosition());

			// turn towards the monster when caught
			if (previous_state == game::GameState::Playing && game.getState() == game::GameState::Caught) {
				capture_start_rotation = glm::quat_cast(mCamera.mWorld.GetRotation());
				auto target_transform = mCamera.mWorld;
				target_transform.LookAt(game.getMonster().getFacePosition());
				capture_target_rotation = glm::quat_cast(target_transform.GetRotation());
			}
			if (game.getState() == game::GameState::Caught || game.getState() == game::GameState::Lost) {
				const float progress = glm::clamp(game.getCaptureTime() / game::Game::capture_turn_duration,
				                                  0.0f, 1.0f);
				const auto rotation = glm::slerp(capture_start_rotation, capture_target_rotation, progress);
				mCamera.mWorld.LookTowards(rotation * glm::vec3(0.0f, 0.0f, -1.0f),
				                          rotation * glm::vec3(0.0f, 1.0f, 0.0f));
			}
		}
		if (game.getState() != game::GameState::Caught && !inputHandler.IsKeyboardCapturedByUI() &&
		    (inputHandler.GetKeycodeState(GLFW_KEY_ENTER) & JUST_PRESSED)) {
			random_seed = random_generator();
			game.reset(random_seed);
			fly_mode = false;
			mCamera.mWorld.SetTranslate(game.getEyePosition());
			mCamera.mWorld.LookTowards(glm::vec3(0.0f, 0.0f, -1.0f));
		}
		camera_position = mCamera.mWorld.GetTranslation();
		camera_direction = mCamera.mWorld.GetFront();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (game.getState() == game::GameState::Playing &&
		    (inputHandler.GetKeycodeState(GLFW_KEY_F) & JUST_PRESSED))
			fly_mode = !fly_mode;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		mWindowManager.NewImGuiFrame();

		if (night_lighting)
			glClearColor(0.02f, 0.0f, 0.08f, 1.0f); // night sky
		else
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		// Render the scene
		if (!shader_reload_failed) {
			bonobo::changePolygonMode(polygon_mode);
			game.render(mCamera.GetWorldToClipMatrix());
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// show the game state
		ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Once);
		ImGui::Begin("Game", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Cakes: %u / %u", game.getCollectedCount(), game::Game::cake_count);
		if (game.getState() == game::GameState::Won)
			ImGui::TextUnformatted("All cakes collected! You won.");
		else if (game.getState() == game::GameState::Caught)
			ImGui::TextUnformatted("Caught by the monster...");
		else if (game.getState() == game::GameState::Lost)
			ImGui::TextUnformatted("Game over. Try again!");
		else
			ImGui::TextUnformatted("Find all seven cakes.");
		ImGui::BeginDisabled(game.getState() == game::GameState::Caught);
		ImGui::TextUnformatted("Enter: restart");
		if (ImGui::Button("Restart")) {
			random_seed = random_generator();
			game.reset(random_seed);
			fly_mode = false;
			mCamera.mWorld.SetTranslate(game.getEyePosition());
			mCamera.mWorld.LookTowards(glm::vec3(0.0f, 0.0f, -1.0f));
		}
		ImGui::EndDisabled();
		ImGui::End();

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::BeginDisabled(game.getState() != game::GameState::Playing);
			ImGui::Checkbox("Fly mode", &fly_mode);
			ImGui::EndDisabled();
			ImGui::Checkbox("Night lighting", &night_lighting);
			ImGui::Separator();
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}

}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
