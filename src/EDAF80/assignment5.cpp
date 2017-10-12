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
	auto const ship_obj = bonobo::loadObjects("spaceship.obj");
	auto const heart_obj = bonobo::loadObjects("heart.obj");
	auto sphere_shape = parametric_shapes::createSphere(100u, 100u, 1.0f);
	auto coin_shape = parametric_shapes::createCircleRing(100u, 100u, 0.0f, 3.0f);
	if (ship_obj.empty() || heart_obj.empty() || sphere_shape.vao == 0u || coin_shape.vao == 0u) {
		LogError("Failed to retrieve the objects");
		return;
	}
	auto const& ship_shape = ship_obj.front();
	auto const& heart_shape = heart_obj.front();

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

	auto light_position = glm::vec3(-100.0f, 150.0f, 60.0f);
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
	waves[0].direction = glm::vec3(0, 0, -0.2f);
	waves[0].frequency = 1.0f;
	waves[0].phase = 1.0f;
	waves[0].sharpness = 2.0f;

	waves[1].amplitude = 0.25f;
	waves[1].direction = glm::vec3(-0.2f, 0, 0);
	waves[1].frequency = 2.0f;
	waves[1].phase = 0.2f;
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
	
	auto const lives_set_uniforms = [&phong_set_uniforms](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(glm::vec3(150/256.0f, 0, 0)));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(glm::vec3(0,0,0)));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(glm::vec3(0,0,0)));
	};
	
	auto const hill_set_uniforms = [&phong_set_uniforms](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(glm::vec3(236/256.0f, 217/256.0f, 171/256.0f)));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(glm::vec3(0,0,0)));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(glm::vec3(0,0,0)));
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

	std::string cubemap = "blue_sky";
	auto texture_cubemap = bonobo::loadTextureCubeMap(cubemap + "/posx.png", cubemap + "/negx.png", cubemap + "/posy.png", cubemap + "/negy.png", cubemap + "/posz.png", cubemap + "/negz.png");
	water.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.add_texture("cube_map_texture", texture_cubemap, GL_TEXTURE_CUBE_MAP);
	
	game.add_child(&skybox);

	auto ship = Node();
	ship.set_geometry(ship_shape);
	ship.set_program(phong_shader, phong_set_uniforms);
	ship.set_translation(glm::vec3(0, 0.5f, 0));
	ship.set_rotation_y(bonobo::pi);
	float ship_collision_radius = 1.5f, water_speed = 0.5f;
	int points = 0, deaths = 0;
	bool dead = false;

	auto ship_diffuse_texture = bonobo::loadTexture2D("metal-surface.png");
	ship.add_texture("diffuse_texture", ship_diffuse_texture, GL_TEXTURE_2D);
	
	game.add_child(&ship);
	
	std::vector<Node> rocks(5);
	auto stone_diffuse_texture = bonobo::loadTexture2D("stone47_diffuse.png");
	auto stone_bump_texture = bonobo::loadTexture2D("stone47_bump.png");
	
	int max_radius = 3;
	float res = 10;
	for (int i = 0; i < rocks.size(); i++) {
		rocks[i].set_geometry(sphere_shape);
		rocks[i].set_program(phong_shader, phong_set_uniforms);
		rocks[i].set_translation(glm::vec3(rand() % size / 4 - size / 8, 0, - size - max_radius - rand() % size));
		rocks[i].set_scaling(glm::vec3((rand() % (max_radius - 1) * res) / res + 1));
		rocks[i].add_texture("diffuse_texture", stone_diffuse_texture, GL_TEXTURE_2D);
		rocks[i].add_texture("bump_texture", stone_bump_texture, GL_TEXTURE_2D);
		game.add_child(&rocks[i]);
	}
	
	std::vector<Node> coins(2);
	
	for (int i = 0; i < coins.size(); i++) {
		coins[i].set_geometry(sphere_shape);
		coins[i].set_program(phong_shader, phong_set_uniforms);
		coins[i].set_translation(glm::vec3(rand() % size / 4 - size / 8, 1.5f, - size - max_radius - rand() % size));
		coins[i].set_scaling(glm::vec3(1, 1, 0.1f));
		game.add_child(&coins[i]);
	}
	
	std::vector<Node> lives(3);
	
	for (int i = 0; i < lives.size(); i++) {
		lives[i].set_geometry(heart_shape);
		lives[i].set_program(phong_shader, lives_set_uniforms);
		lives[i].set_translation(glm::vec3((i - 1) * 3, 8, -4));
		lives[i].set_scaling(glm::vec3(0.005f));
		game.add_child(&lives[i]);
	}
	
	auto left_hill = Node();
	left_hill.set_geometry(quad_shape);
	left_hill.set_program(phong_shader, hill_set_uniforms);
	left_hill.set_translation(glm::vec3(-15, 0, -50));
	left_hill.set_rotation_z(bonobo::pi * 7/8);
	left_hill.set_scaling(glm::vec3(0.1f, 1, 1));
	
	game.add_child(&left_hill);
	
	auto right_hill = Node();
	right_hill.set_geometry(quad_shape);
	right_hill.set_program(phong_shader, hill_set_uniforms);
	right_hill.set_translation(glm::vec3(15, 0, -50));
	right_hill.set_rotation_z(bonobo::pi * 1/8);
	right_hill.set_scaling(glm::vec3(0.1f, 1, 1));
	
	game.add_child(&right_hill);

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
			ship.set_translation(glm::vec3(0, 0.5f, 0));
			waves[0].frequency = 1.0f;
			points = 0, deaths = 0, water_speed = 0.5f;
			dead = false;
		}

		glm::mat4 T = ship.get_transform();
		float posx = T[3][0], posz = -T[3][2];
		float x = velocity[0], z = -velocity[2];
		float dx = 0.0, dz = 0.0;
		float acceleration = 0.001f * ddeltatime;

		if ((inputHandler->GetKeycodeState(GLFW_KEY_W) & PRESSED) && posz + z * 20.0f <  4) dz += acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_S) & PRESSED) && posz + z * 20.0f > -4) dz -= acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_A) & PRESSED) && posx + x * 20.0f > -10) dx -= acceleration;
		if ((inputHandler->GetKeycodeState(GLFW_KEY_D) & PRESSED) && posx + x * 20.0f <  10) dx += acceleration;

		velocity += glm::vec3(dx, 0, -dz);
		velocity *= 0.95f;

		ship.translate(velocity);
		
		for (int i = 0; i < rocks.size(); i++) {
			glm::mat4 T_rock = rocks[i].get_transform();
			glm::vec3 p1 = glm::vec3(T[3][0], T[3][1], T[3][2]);
			glm::vec3 p2 = glm::vec3(T_rock[3][0], T_rock[3][1], T_rock[3][2]);
			glm::vec3 diff = p2 - p1;
			float dist = sqrt(dot(diff, diff));
			
			float r1 = ship_collision_radius;
			float r2 = T_rock[0][0];
			
			bool collision = dist < r1 + r2;
			
			if(collision && !dead) {
				deaths++;
				if(deaths == lives.size())
					dead = true;
			}
			
			if(collision || -rocks[i].get_transform()[3][2] < -5)
				rocks[i].set_translation(glm::vec3(rand() % size / 4 - size / 8, 0, - size / 2 - max_radius));
			
			rocks[i].translate(glm::vec3(0, 0, water_speed));
		}
		
		for(int i = 0; i < coins.size(); i++) {
			glm::mat4 T_coin = coins[i].get_transform();
			glm::vec3 p1 = glm::vec3(T[3][0], T[3][1], T[3][2]);
			glm::vec3 p2 = glm::vec3(T_coin[3][0], T_coin[3][1], T_coin[3][2]);
			glm::vec3 diff = p2 - p1;
			float dist = sqrt(dot(diff, diff));
			
			float r1 = ship_collision_radius;
			float r2 = 1.5f;
			
			bool collision = dist < r1 + r2;
			
			if(collision && !dead) {
				points++;
				waves[0].frequency += 0.1f;
				if(points % 10 == 0) {
					water_speed += 3 / 20.0f;
				}
			}
			
			if(collision || -coins[i].get_transform()[3][2] < -5)
				coins[i].set_translation(glm::vec3(rand() % size / 4 - size / 8, 1.5f, - size / 2 - max_radius));
			
			coins[i].translate(glm::vec3(0, 0, water_speed));
			coins[i].rotate_y(0.05f);
		}
		
		float _deaths = deaths;
		for (int i = 0; i < lives.size(); i++) {
			if (_deaths > 0) {
				lives[i].set_scaling(glm::vec3(0));
				_deaths--;
			} else {
				lives[i].set_scaling(glm::vec3(0.005f));
			}
		}
		
		if(dead)
			ship.translate(glm::vec3(0, -0.01f, 0));

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

		//Log::View::Render();

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool opened = true;
		if(dead) {
			ImGui::Begin("Game Over!", &opened, ImVec2(120, 50), -1.0f, 0);
			ImGui::Text("Oh no, you died!");
			ImGui::Text(("Total score: " + std::to_string(points)).c_str());
			ImGui::End();
		} else {
			ImGui::Begin("Scoreboard", &opened, ImVec2(120, 50), -1.0f, 0);
			ImGui::Text(("Current score: " + std::to_string(points)).c_str());
			ImGui::End();
		}

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
