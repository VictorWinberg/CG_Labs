#include "assignment1.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/opengl.hpp"
#include "core/utils.h"
#include "core/various.hpp"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <vector>


edaf80::Assignment1::Assignment1()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 1", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment1::~Assignment1()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment1::run()
{
	// Load the sphere geometry
	auto const objects = bonobo::loadObjects("sphere.obj");
	if (objects.empty())
		return;
	auto const& sphere = objects.front();

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 50.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader program
	auto shader = bonobo::createProgram("default.vert", "default.frag");
	if (shader == 0u) {
		LogError("Failed to load shader");
		return;
	}

	auto sun = Node();
	// Load the sun's texture
	auto sun_texture = bonobo::loadTexture2D("sunmap.png");
	sun.set_geometry(sphere);
	sun.set_program(shader, [](GLuint /*program*/){});
	// Todo: Attach a texture to the sun
	sun.add_texture("diffuse_texture", sun_texture, GL_TEXTURE_2D);
	sun.set_scaling(glm::vec3(15, 15, 15));


	auto world = Node();
	world.add_child(&sun);


	//
	// Todo: Create an Earth node
	//
	auto earth = Node();
	auto earth_texture = bonobo::loadTexture2D("earth_diffuse.png");
	earth.set_geometry(sphere);
	earth.set_program(shader, [](GLuint /*program*/){});
	earth.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);

	earth.set_scaling(glm::vec3(0.2f, 0.2f, 0.2f));
	earth.set_translation(glm::vec3(3.5f, 0, 0));

	auto earth_pivot = Node();
	sun.add_child(&earth_pivot);

	earth_pivot.add_child(&earth);

	//
	// Create a Moon node
	//
	auto moon = Node();
	auto moon_texture = bonobo::loadTexture2D("moon.png");
	moon.set_geometry(sphere);
	moon.set_program(shader, [](GLuint /*program*/){});
	moon.add_texture("diffuse_texture", moon_texture, GL_TEXTURE_2D);

	moon.set_scaling(glm::vec3(0.3f, 0.3f, 0.3f));
	moon.set_translation(glm::vec3(2.0f, 0.0f, 0.0f));

	auto moon_pivot = Node();
	earth.add_child(&moon_pivot);

	moon_pivot.add_child(&moon);

	//
	// Create planets
	//
	std::vector<Node> planets(7);
	std::vector<glm::vec3> translations(7);
	std::vector<glm::vec3> scalings(7);
	std::vector<std::string> textures(7);
	std::vector<int> orbit_speeds(7);
	std::vector<float> rotation_period(7);

	translations[0] = glm::vec3(1.5f, 0, 0);
	scalings[0] = glm::vec3(0.1f, 0.1f, 0.1f);
	textures[0] = "mercury.png";
	orbit_speeds[0] = 48;
	rotation_period[0] = 58.65f;
	translations[1] = glm::vec3(2.5f, 0, 0);
	scalings[1] = glm::vec3(0.2f, 0.2f, 0.2f);
	textures[1] = "venus.png";
	orbit_speeds[1] = 35;
	rotation_period[1] = -243.0f;
	translations[2] = glm::vec3(4.5f, 0, 0);
	scalings[2] = glm::vec3(0.1f, 0.1f, 0.1f);
	textures[2] = "mars.png";
	orbit_speeds[2] = 24;
	rotation_period[2] = 1.03f;
	translations[3] = glm::vec3(6.0f, 0, 0);
	scalings[3] = glm::vec3(0.5f, 0.5f, 0.5f);
	textures[3] = "jupiter.png";
	orbit_speeds[3] = 13;
	rotation_period[3] = 0.41f;
	translations[4] = glm::vec3(8.0f, 0, 0);
	scalings[4] = glm::vec3(0.5f, 0.5f, 0.5f);
	textures[4] = "saturn.png";
	orbit_speeds[4] = 10;
	rotation_period[4] = 0.44f;
	translations[5] = glm::vec3(10.0f, 0, 0);
	scalings[5] = glm::vec3(0.3f, 0.3f, 0.3f);
	textures[5] = "uranus.png";
	orbit_speeds[5] = 7;
	rotation_period[5] = -0.72f;
	translations[6] = glm::vec3(12.0f, 0, 0);
	scalings[6] = glm::vec3(0.3f, 0.3f, 0.3f);
	textures[6] = "neptune.png";
	orbit_speeds[6] = 5;
	rotation_period[6] = 0.72f;

	for (int i = 0; i < planets.size(); i++) {
		planets[i].set_geometry(sphere);
		planets[i].set_program(shader, [](GLuint /*program*/){});
		planets[i].set_translation(translations[i]);
		planets[i].set_scaling(scalings[i]);
        planets[i].add_texture("diffuse_texture", bonobo::loadTexture2D(textures[i]), GL_TEXTURE_2D);
	}

	//
	// Create planet pivots
	//
	std::vector<Node> planet_pivots(7);

	for (int i = 0; i < planet_pivots.size(); i++) {
		planet_pivots[i].set_rotation_y(1 / rotation_period[i]);
		planet_pivots[i].add_child(&planets[i]);
		sun.add_child(&planet_pivots[i]);
	}


	glEnable(GL_DEPTH_TEST);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler->SetUICapture(io.WantCaptureMouse, io.WantCaptureMouse);

		glfwPollEvents();
		inputHandler->Advance();
		mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();


		//
		// How-To: Translate the sun
		//
		double orbitSpeed = 0.0001;
		double rotationSpeed = 0.01;

		sun.rotate_y(- 0.0001f);

		earth_pivot.rotate_y(- 30 * orbitSpeed);
		earth.rotate_y(- rotationSpeed / 1);

		moon_pivot.rotate_y(rotationSpeed - orbitSpeed * 0.03f);
		moon.rotate_y(- rotationSpeed / 27);

		for (int i = 0; i < planets.size(); i++) {
			planets[i].rotate_y(- rotationSpeed / rotation_period[i]);
			planet_pivots[i].rotate_y(- orbit_speeds[i] * orbitSpeed);
		}

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		// Traverse the scene graph and render all the nodes
		auto node_stack = std::stack<Node const*>();
		auto matrix_stack = std::stack<glm::mat4>();
		node_stack.push(&world);
		matrix_stack.push(glm::mat4());
		do {
			auto const* const current_node = node_stack.top();
			node_stack.pop();

			auto const parent_matrix = matrix_stack.top();
			matrix_stack.pop();

			auto const current_node_matrix = current_node->get_transform();

			//
			// Todo: Compute the current node's world matrix
			//
			auto const current_node_world_matrix = parent_matrix * current_node_matrix;
			current_node->render(mCamera.GetWorldToClipMatrix(), current_node_world_matrix);

			for (int i = static_cast<int>(current_node->get_children_nb()) - 1; i >= 0; --i) {
				node_stack.push(current_node->get_child(static_cast<size_t>(i)));
				matrix_stack.push(current_node_world_matrix);
			}
		} while (!node_stack.empty());

		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(shader);
	shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment1 assignment1;
		assignment1.run();
	}
	catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
