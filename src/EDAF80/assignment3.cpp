#include "assignment3.hpp"
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

edaf80::Assignment3::Assignment3()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 3", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment3::~Assignment3()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment3::run()
{
	// Load the sphere geometry
	auto sphere_shape = parametric_shapes::createSphere(20u, 20u, 2.0f);
	auto cube_map_shape = parametric_shapes::createSphere(20u, 20u, 20.0f);
	if (cube_map_shape.vao == 0u || sphere_shape.vao == 0u) {
		LogError("Failed to retrieve the sphere mesh");
		return;
	}

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto phong_shader = bonobo::createProgram("phong.vert", "phong.frag");
	if (phong_shader == 0u) {
		LogError("Failed to load phong shader");
		return;
	}
	GLuint cube_shader = 0u, bump_shader = 0u, texture_shader = 0u;
	auto const reload_shaders = [&cube_shader,&bump_shader,&texture_shader](){
		if (cube_shader != 0u)
			glDeleteProgram(cube_shader);
		cube_shader = bonobo::createProgram("cube.vert", "cube.frag");
		if (cube_shader == 0u)
			LogError("Failed to load cube shader");

		if (bump_shader != 0u)
			glDeleteProgram(bump_shader);
		bump_shader = bonobo::createProgram("bump.vert", "bump.frag");
		if (bump_shader == 0u)
			LogError("Failed to load bump shader");

		if (texture_shader != 0u)
			glDeleteProgram(texture_shader);
		texture_shader = bonobo::createProgram("default.vert", "default.frag");
		if (texture_shader == 0u)
			LogError("Failed to load texture shader");
	};
	reload_shaders();

	auto light_position = glm::vec3(20.0f, 20.0f, 20.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	auto diffuse = glm::vec3(0.8f, 0.6f, 0.2f);
	auto specular = glm::vec3(0.3f, 0.3f, 0.3f);
	auto shininess = 100.0f;
	auto const phong_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

	auto polygon_mode = polygon_mode_t::fill;

	auto node = Node();
	node.set_geometry(sphere_shape);
	node.set_program(phong_shader, phong_set_uniforms);

//	auto texture = bonobo::loadTexture2D("earth_diffuse.png");
	std::string cubemap = "cloudyhills";
	auto texture = bonobo::loadTextureCubeMap(cubemap + "/posx.png", cubemap + "/negx.png", cubemap + "/posy.png", cubemap + "/negy.png", cubemap + "/posz.png", cubemap + "/negz.png");
	node.add_texture("cube_map_texture", texture, GL_TEXTURE_CUBE_MAP);
	auto earth_texture = bonobo::loadTexture2D("earth_diffuse.png");
	node.add_texture("diffuse_texture", earth_texture, GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
//	glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
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
			node.set_program(phong_shader, phong_set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_2) & JUST_PRESSED) {
			node.set_program(cube_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_3) & JUST_PRESSED) {
			node.set_program(bump_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_4) & JUST_PRESSED) {
			node.set_program(texture_shader, set_uniforms);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_Z) & JUST_PRESSED) {
			polygon_mode = get_next_mode(polygon_mode);
		}
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
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

		camera_position = mCamera.mWorld.GetTranslation();

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		node.render(mCamera.GetWorldToClipMatrix(), node.get_transform());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
			ImGui::SliderFloat("Shininess", &shininess, 0.0f, 1000.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
		ImGui::End();

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	glDeleteProgram(texture_shader);
	texture_shader = 0u;
	glDeleteProgram(bump_shader);
	bump_shader = 0u;
	glDeleteProgram(cube_shader);
	cube_shader = 0u;
	glDeleteProgram(phong_shader);
	cube_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment3 assignment3;
		assignment3.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
