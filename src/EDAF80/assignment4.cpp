#include "assignment4.hpp"
#include "parametric_shapes.hpp"

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
#include "core/utils.h"
#include "core/Window.h"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include "external/glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>

edaf80::Assignment4::Assignment4()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 4", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment4::~Assignment4()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment4::run()
{
	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 6.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}
	GLuint water_shader = 0u, cube_shader = 0u;
	auto const reload_shaders = [&water_shader, &cube_shader](){
		//
		// Todo: Insert the creation of other shader programs.
		//       (Check how it was done in assignment 3.)
		//
		if (water_shader != 0u)
			glDeleteProgram(water_shader);
		water_shader = bonobo::createProgram("water.vert", "water.frag");
		if (water_shader == 0u)
			LogError("Failed to load water shader");

		if (cube_shader != 0u)
			glDeleteProgram(cube_shader);
		cube_shader = bonobo::createProgram("cube.vert", "cube.frag");
		if (cube_shader == 0u)
			LogError("Failed to load cube shader");
	};
	reload_shaders();

	auto light_position = glm::vec3(20.0f, 20.0f, 20.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();
	
	const int nbrWaves = 2;

	struct wave {
		float amplitude;
		glm::vec3 direction;
		float frequency;
		float phase;
		float sharpness;
	} waves[nbrWaves];

	waves[0].amplitude = 1.0f;
	waves[0].direction = glm::vec3(-1, 0, 0);
	waves[0].frequency = 0.2f;
	waves[0].phase = 0.5f;
	waves[0].sharpness = 2.0f;

	waves[1].amplitude = 0.5f;
	waves[1].direction = glm::vec3(-7, 0, 0.7);
	waves[1].frequency = 0.4f;
	waves[1].phase = 1.3f;
	waves[1].sharpness = 2.0f;

	auto speed = 10.0f;
	auto time = 0.0f;

	auto const set_uniforms = [&light_position,&camera_position,&waves,&time,&nbrWaves](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		for (int i = 0; i < nbrWaves; i++) {
			glUniform1f(glGetUniformLocation(program, ("A_"+std::to_string(i+1)).c_str()), waves[i].amplitude);
			glUniform3fv(glGetUniformLocation(program, ("D_"+std::to_string(i+1)).c_str()), 1, glm::value_ptr(waves[i].direction));
			glUniform1f(glGetUniformLocation(program, ("f_"+std::to_string(i+1)).c_str()), waves[i].frequency);
			glUniform1f(glGetUniformLocation(program, ("p_"+std::to_string(i+1)).c_str()), waves[i].phase);
			glUniform1f(glGetUniformLocation(program, ("k_"+std::to_string(i+1)).c_str()), waves[i].sharpness);
		}
		glUniform1f(glGetUniformLocation(program, "t"), time);
	};

	//
	// Todo: Load your geometry
	//
	int size = 100;
	auto quad_shape = parametric_shapes::createQuad(size, size, size, size);
	auto cube_map_shape = parametric_shapes::createSphere(100u, 100u, 100.0f);

	auto node = Node();
	node.set_geometry(quad_shape);
	node.set_program(water_shader, set_uniforms);
	node.set_translation(glm::vec3(-size/2.0, 0, -size/2.0));

	auto water_texture = bonobo::loadTexture2D("waves.png");
	node.add_texture("bump_texture", water_texture, GL_TEXTURE_2D);

	auto skybox = Node();
	skybox.set_geometry(cube_map_shape);
	skybox.set_program(cube_shader, set_uniforms);

	std::string cubemap = "cloudyhills";
	auto texture_cubemap = bonobo::loadTextureCubeMap(cubemap + "/posx.png", cubemap + "/negx.png", cubemap + "/posy.png", cubemap + "/negy.png", cubemap + "/posz.png", cubemap + "/negz.png");
	node.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	while (!glfwWindowShouldClose(window->GetGLFW_Window())) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		time += speed * ddeltatime / 1000.0f;
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

		//
		// Todo: If you need to handle inputs, you can do it here
		//
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

		camera_position = mCamera.mWorld.GetTranslation();

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//
		// Todo: Render all your geometry here.
		//
		node.render(mCamera.GetWorldToClipMatrix(), node.get_transform());
		skybox.render(mCamera.GetWorldToClipMatrix(), skybox.get_transform());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool opened;
		for (int i = 0; i < nbrWaves; i++) {
			auto title = "Wave Control ";
			auto index = std::to_string(i+1);
			opened = ImGui::Begin((title + index).c_str(), &opened, ImVec2(300, 200), -1.0f, 0);
			if (opened) {
				ImGui::SliderFloat("Amplitude", &waves[i].amplitude, 0.0f, 5.0f);
				ImGui::SliderFloat3("Direction", glm::value_ptr(waves[i].direction), -1.0f, 1.0f);
				ImGui::SliderFloat("Frequency", &waves[i].frequency, 0.0f, 5.0f);
				ImGui::SliderFloat("Phase", &waves[i].phase, 0.0f, 5.0f);
				ImGui::SliderFloat("Sharpness", &waves[i].sharpness, 0.0f, 5.0f);
			}
			ImGui::End();
		}
		
		ImGui::Begin("Enviroment Control", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened) {
			ImGui::SliderFloat("Speed", &speed, 0.0f, 50.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened) {
			ImGui::Text("Time: %.3f s", time);
			ImGui::Text("dT: %.3f ms", ddeltatime);
		}
		ImGui::End();

		ImGui::Render();

		window->Swap();
		lastTime = nowTime;
	}

	//
	// Todo: Do not forget to delete your shader programs, by calling
	//       `glDeleteProgram($your_shader_program)` for each of them.
	//
	glDeleteProgram(water_shader);
	water_shader = 0u;
	glDeleteProgram(cube_shader);
	cube_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment4 assignment4;
		assignment4.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
