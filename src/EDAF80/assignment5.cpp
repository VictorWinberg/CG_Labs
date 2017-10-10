#include "assignment5.hpp"
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
#include <stack>

edaf80::Assignment5::Assignment5()
{
	Log::View::Init();

	window = Window::Create("EDAF80: Assignment 5", config::resolution_x,
	                        config::resolution_y, config::msaa_rate, false);
	if (window == nullptr) {
		Log::View::Destroy();
		throw std::runtime_error("Failed to get a window: aborting!");
	}
	inputHandler = new InputHandler();
	window->SetInputHandler(inputHandler);
}

edaf80::Assignment5::~Assignment5()
{
	delete inputHandler;
	inputHandler = nullptr;

	Window::Destroy(window);
	window = nullptr;

	Log::View::Destroy();
}

void
edaf80::Assignment5::run()
{
	// Load the sphere geometry
	auto const objects = bonobo::loadObjects("spaceship.obj");
	auto sphere_shape = parametric_shapes::createSphere(100u, 100u, 1.0f);
	if (objects.empty() || sphere_shape.vao == 0u) {
		LogError("Failed to retrieve the objects");
		return;
	}
	auto const& ship_shape = objects.front();

	// Set up the camera
	FPSCameraf mCamera(bonobo::pi / 4.0f,
	                   static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	                   0.01f, 1000.0f);
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 5.0f, 15.0f));
	mCamera.mWorld.SetRotateX(-bonobo::pi/20.0f);
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;
	window->SetCamera(&mCamera);

	// Create the shader programs
	auto fallback_shader = bonobo::createProgram("fallback.vert", "fallback.frag");
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint water_shader = 0u, cube_shader = 0u, bump_shader = 0u, texture_shader = 0u, phong_shader = 0u;
	auto const reload_shaders = [&water_shader, &cube_shader, &bump_shader, &texture_shader, &phong_shader](){
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

		if (phong_shader != 0u)
			glDeleteProgram(phong_shader);
		phong_shader = bonobo::createProgram("phong.vert", "phong.frag");
		if (phong_shader == 0u)
			LogError("Failed to load phong shader");
	};
	reload_shaders();

	srand(time(NULL));

	auto light_position = glm::vec3(-100.0f, 200.0f, -100.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();

	const int nbrWaves = 2;

	struct wave {
		float amplitude;
		glm::vec3 direction;
		float frequency;
		float phase;
		float sharpness;
	} waves[nbrWaves];

	waves[0].amplitude = 0.25f;
	waves[0].direction = glm::vec3(-1, 0, 0);
	waves[0].frequency = 2.0f;
	waves[0].phase = 0.5f;
	waves[0].sharpness = 2.0f;

	waves[1].amplitude = 0.25;
	waves[1].direction = glm::vec3(-0.5f, 0.0f, -0.5f);
	waves[1].frequency = 2.0f;
	waves[1].phase = 0.3f;
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

	auto const phong_set_uniforms = [&light_position,&camera_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(glm::vec3(0.8f, 0.6f, 0.2f)));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(glm::vec3(0.3f, 0.3f, 0.3f)));
		glUniform1f(glGetUniformLocation(program, "shininess"), 1);
	};

	//
	// Todo: Load your geometry
	//
	auto game = Node();

	int size = 100;
	auto quad_shape = parametric_shapes::createQuad(size, size, size, size);
	auto cube_map_shape = parametric_shapes::createSphere(100u, 100u, size/2.0f);

	auto water = Node();
	water.set_geometry(quad_shape);
	water.set_program(water_shader, set_uniforms);
	water.set_translation(glm::vec3(-size/2.0, 0, -size/2.0));

	auto water_texture = bonobo::loadTexture2D("waves.png");
	water.add_texture("bump_texture", water_texture, GL_TEXTURE_2D);
	
	game.add_child(&water);

	auto skybox = Node();
	skybox.set_geometry(cube_map_shape);
	skybox.set_program(cube_shader, set_uniforms);

	std::string cubemap = "opensea";
	auto texture_cubemap = bonobo::loadTextureCubeMap(cubemap + "/posx.png", cubemap + "/negx.png", cubemap + "/posy.png", cubemap + "/negy.png", cubemap + "/posz.png", cubemap + "/negz.png");
	water.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);
	
	game.add_child(&skybox);

	auto ship = Node();
	ship.set_geometry(ship_shape);
	ship.set_program(phong_shader, phong_set_uniforms);
	ship.set_translation(glm::vec3(0, 0.5f, 0));
	ship.set_rotation_y(bonobo::pi);

	auto ship_diffuse_texture = bonobo::loadTexture2D("metal-surface.png");
	ship.add_texture("diffuse_texture", ship_diffuse_texture, GL_TEXTURE_2D);
	
	game.add_child(&ship);
	
	std::vector<Node> rocks(10);
	auto stone_diffuse_texture = bonobo::loadTexture2D("stone47_diffuse.png");
	auto stone_bump_texture = bonobo::loadTexture2D("stone47_bump.png");
	
	for (int i = 0; i < rocks.size(); i++) {
		rocks[i].set_geometry(sphere_shape);
		rocks[i].set_program(phong_shader, phong_set_uniforms);
		rocks[i].set_translation(glm::vec3(rand() % 50 - 25, 0, -40));
		rocks[i].set_scaling(glm::vec3((rand() % 20) / 10.0f + 1));
		rocks[i].add_texture("diffuse_texture", stone_diffuse_texture, GL_TEXTURE_2D);
		rocks[i].add_texture("bump_texture", stone_bump_texture, GL_TEXTURE_2D);
		game.add_child(&rocks[i]);
	}

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	auto velocity = glm::vec3(0, 0, 0);

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
		//mCamera.Update(ddeltatime, *inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		//
		// Todo: If you need to handle inputs, you can do it here
		//
		if (inputHandler->GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			reload_shaders();
		}

		glm::mat4 T = ship.get_transform();
		float posx = T[3][0], posz = -T[3][2];
		float x = velocity[0], z = -velocity[2];
		float dx = 0.0, dz = 0.0;
		float acceleration = 0.001f * ddeltatime;

		if ((inputHandler->GetKeycodeState(GLFW_KEY_W) & PRESSED) && posz + z * 20.0f <  4) dz += acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_S) & PRESSED) && posz + z * 20.0f > -4) dz -= acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_A) & PRESSED) && posx + x * 20.0f > -8) dx -= acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_D) & PRESSED) && posx + x * 20.0f <  8) dx += acceleration;

		velocity += glm::vec3(dx, 0, -dz);
		velocity *= 0.95f;

		ship.translate(velocity);

		auto const window_size = window->GetDimensions();
		glViewport(0, 0, window_size.x, window_size.y);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		//
		// Todo: Render all your geometry here.
		//
		auto node_stack = std::stack<Node const*>();
		node_stack.push(&game);
		
		while (!node_stack.empty()) {
			auto const current_node = node_stack.top();
			node_stack.pop();
			
			current_node->render(mCamera.GetWorldToClipMatrix(), current_node->get_transform());
			
			for (int i = 0; i < current_node->get_children_nb(); i++) {
				node_stack.push(current_node->get_child(i));
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		Log::View::Render();

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//

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
	glDeleteProgram(bump_shader);
	bump_shader = 0u;
	glDeleteProgram(texture_shader);
	texture_shader = 0u;
	glDeleteProgram(phong_shader);
	phong_shader = 0u;
}

int main()
{
	Bonobo::Init();
	try {
		edaf80::Assignment5 assignment5;
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
	Bonobo::Destroy();
}
