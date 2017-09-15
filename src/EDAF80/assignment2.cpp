#include "assignment2.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "external/glad/glad.h"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/InputHandler.h"
#include "core/Log.h"
#include "core/LogView.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

enum class polygon_mode_t : unsigned int {
	fill = 0u,
	line,
	point
};

static polygon_mode_t get_next_mode(polygon_mode_t mode)
{
	return static_cast<polygon_mode_t>((static_cast<unsigned int>(mode) + 1u) % 3u);
}

edaf80::Assignment2::Assignment2()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 2", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment2::~Assignment2()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment2::run()
{
	// Load the sphere geometry
	auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	if (shape.vao == 0u)
		return;

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	auto diffuse_shader = bonobo::createProgram("diffuse.vert", "diffuse.frag");
	if (diffuse_shader == 0u) {
		LogError("Failed to load diffuse shader");
		return;
	}
	auto normal_shader = bonobo::createProgram("normal.vert", "normal.frag");
	if (normal_shader == 0u) {
		LogError("Failed to load normal shader");
		return;
	}
	auto texcoord_shader = bonobo::createProgram("texcoord.vert", "texcoord.frag");
	if (texcoord_shader == 0u) {
		LogError("Failed to load texcoord shader");
		return;
	}

	auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.5f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = false;

    std::vector<glm::vec3> cp = {
        glm::vec3( 0,	-5,		-5),
        glm::vec3( 3,	-3,		-5),
        glm::vec3( 0,	-1,		-2),
        glm::vec3(-3,	-0.75f,	-5),
        glm::vec3( 0,	-0.50f,	-8),
        glm::vec3( 3,	 0.50f,	-5),
        glm::vec3( 0,	 0.75f,	-2),
        glm::vec3(-3,	 1,		-5),
        glm::vec3( 0,	 3,		-8),
        glm::vec3( 0,	 5,		-5)
    }; // N control points
    float path_pos = 0.0f;
    float pos_velocity = 0.05f;

	auto circle_rings = Node();
	circle_rings.set_geometry(shape);
	circle_rings.set_program(fallback_shader, set_uniforms);


	//! \todo Create a tesselated sphere and a tesselated torus
	auto const quad_shape = parametric_shapes::createQuad(2u, 2u);
	auto const sphere_shape = parametric_shapes::createSphere(10u, 10u, 2.0f);

	auto polygon_mode = polygon_mode_t::fill;

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


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


		if (inputHandler->GetKeycodeState(GLFW_KEY_1) & JUST_PRESSED) {
			circle_rings.set_program(fallback_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			circle_rings.set_program(diffuse_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			circle_rings.set_program(normal_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			circle_rings.set_program(texcoord_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_X) & JUST_PRESSED) {
			circle_rings.set_geometry(shape);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_C) & JUST_PRESSED) {
			circle_rings.set_geometry(sphere_shape);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_V) & JUST_PRESSED) {
			circle_rings.set_geometry(quad_shape);
		}
		switch (polygon_mode) {
			case polygon_mode_t::fill:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;
			case polygon_mode_t::line:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;
			case polygon_mode_t::point:
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				break;
		}

		circle_rings.rotate_y(0.05f);

		//! \todo Interpolate the movement of a shape between various
		//!        control points

		// Main loop:
		int i = floor(path_pos); // floor returns closest lower integer
		// use indices (e.g.) i, i+1, i+2, i+3 to retrieve control
		// points from cp[].
		// make sure indices wrap: 0, 1 … N-1, 0, 1…
		// run interpolation
		// update the animated object
		path_pos += pos_velocity; // step forward

		float x = path_pos - i;

		if(use_linear)
			circle_rings.set_translation(interpolation::evalLERP(cp[i%10], cp[(i+1)%10], x));
		else
			circle_rings.set_translation(interpolation::evalCatmullRom(cp[i%10], cp[(i+1)%10], cp[(i+2)%10], cp[(i+3)%10], catmull_rom_tension, x));


		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		circle_rings.render(mCamera.GetWorldToClipMatrix(), circle_rings.get_transform());

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Log::View::Render();
		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texcoord_shader);
	normal_shader = 0u;
	glDeleteProgram(normal_shader);
	normal_shader = 0u;
	glDeleteProgram(diffuse_shader);
	diffuse_shader = 0u;
	glDeleteProgram(fallback_shader);
	diffuse_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment2 assignment2;
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
