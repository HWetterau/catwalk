#include <GL/glew.h>

#include "bone_geometry.h"
#include "procedure_geometry.h"
#include "render_pass.h"
#include "config.h"
#include "gui.h"
#include <jpegio.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>

using namespace std;

int window_width = 1280;
int window_height = 720;
int main_view_width = 960;
int main_view_height = 720;
int preview_width = window_width - main_view_width; // 320
int preview_height = preview_width / 4 * 3; // 320 / 4 * 3 = 240
int preview_bar_width = preview_width;
int preview_bar_height = main_view_height;
const std::string window_title = "Animation";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* blending_shader =
#include "shaders/blending.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* bone_vertex_shader =
#include "shaders/bone.vert"
;

const char* bone_fragment_shader =
#include "shaders/bone.frag"
;

// FIXME: Add more shaders here.
const char* cylinder_vertex_shader =
#include "shaders/cylinder.vert"
;

const char* cylinder_fragment_shader =
#include "shaders/cylinder.frag"
;

const char* axes_vertex_shader =
#include "shaders/axes.vert"
;

const char* axes_fragment_shader =
#include "shaders/axes.frag"
;

const char* quad_vertex_shader =
#include "shaders/quad.vert"
;

const char* quad_fragment_shader =
#include "shaders/quad.frag"
;



void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Input model file is missing" << std::endl;
		std::cerr << "Usage: " << argv[0] << " <PMD file>" << std::endl;
		return -1;
	}
	GLFWwindow *window = init_glefw();
	GUI gui(window, main_view_width, main_view_height, preview_height);

	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	create_floor(floor_vertices, floor_faces);

	LineMesh cylinder_mesh;
	LineMesh axes_mesh;

	std::vector<glm::vec4> quad_vertices;
	std::vector<glm::uvec3> quad_faces;
	create_quad(quad_vertices, quad_faces);

	// FIXME: we already created meshes for cylinders. Use them to render
	//        the cylinder and axes if required by the assignment.
	create_cylinder_mesh(cylinder_mesh);
	create_axes_mesh(axes_mesh);

	Mesh mesh;
	mesh.loadPmd(argv[1]);
	std::cout << "Loaded object  with  " << mesh.vertices.size()
		<< " vertices and " << mesh.faces.size() << " faces.\n";

	glm::vec4 mesh_center = glm::vec4(0.0f);
	for (size_t i = 0; i < mesh.vertices.size(); ++i) {
		mesh_center += mesh.vertices[i];
	}
	mesh_center /= mesh.vertices.size();

	/*
	 * GUI object needs the mesh object for bone manipulation.
	 */
	gui.assignMesh(&mesh);

	glm::vec4 light_position = glm::vec4(0.0f, 100.0f, 0.0f, 1.0f);
	MatrixPointers mats; // Define MatrixPointers here for lambda to capture

	/*
	 * In the following we are going to define several lambda functions as
	 * the data source of GLSL uniforms
	 *
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 *
	 * Note: lambda expressions cannot be converted to std::function directly
	 *       Hence we need to declare the data function explicitly.
	 *
	 * CAVEAT: DO NOT RETURN const T&, which compiles but causes
	 *         segfaults.
	 *
	 * Do not worry about the efficient issue, copy elision in C++ 17 will
	 * minimize the performance impact.
	 *
	 * More details about copy elision:
	 *      https://en.cppreference.com/w/cpp/language/copy_elision
	 */

	// FIXME: add more lambdas for data_source if you want to use RenderPass.
	//        Otherwise, do whatever you like here
	std::function<const glm::mat4*()> model_data = [&mats]() {
		return mats.model;
	};
	std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
	std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };
	std::function<glm::mat4()> identity_mat = [](){ return glm::mat4(1.0f); };
	std::function<glm::vec3()> cam_data = [&gui](){ return gui.getCamera(); };
	std::function<glm::vec4()> lp_data = [&light_position]() { return light_position; };
	
	
	auto std_model = std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
	auto floor_model = make_uniform("model", identity_mat);
	auto std_view = make_uniform("view", view_data);
	auto std_camera = make_uniform("camera_position", cam_data);
	auto std_proj = make_uniform("projection", proj_data);
	auto std_light = make_uniform("light_position", lp_data);

	std::function<float()> alpha_data = [&gui]() {
		static const float transparet = 0.5; // Alpha constant goes here
		static const float non_transparet = 1.0;
		if (gui.isTransparent())
			return transparet;
		else
			return non_transparet;
	};
	auto object_alpha = make_uniform("alpha", alpha_data);

	std::function<std::vector<glm::vec3>()> trans_data = [&mesh](){ return mesh.getCurrentQ()->transData(); };
	std::function<std::vector<glm::fquat>()> rot_data = [&mesh](){ return mesh.getCurrentQ()->rotData(); };
	auto joint_trans = make_uniform("joint_trans", trans_data);
	auto joint_rot = make_uniform("joint_rot", rot_data);
	// FIXME: define more ShaderUniforms for RenderPass if you want to use it.
	//        Otherwise, do whatever you like here
	std::function<glm::mat4()> bone_transform = [&gui](){ return gui.boneTransform(); };
	
	auto bone_trans = make_uniform("bone_transform", bone_transform);

	std::function<vector<glm::mat4>()> d_u_matrix  = [&mesh](){ return mesh.load_d_u(); };
	auto blend_d_u = make_uniform("blend_d_u", d_u_matrix);

	// std::function<vector<glm::mat4>()> d_matrix  = [&mesh](){ return mesh.load_d(); };
	// auto blend_d = make_uniform("blend_d", d_matrix);



	// Floor render pass
	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);

	// PMD Model render pass
	// FIXME: initialize the input data at Mesh::loadPmd
	std::vector<glm::vec2>& uv_coordinates = mesh.uv_coordinates;
	RenderDataInput object_pass_input;
	object_pass_input.assign(0, "jid0", mesh.joint0.data(), mesh.joint0.size(), 1, GL_INT);
	object_pass_input.assign(1, "jid1", mesh.joint1.data(), mesh.joint1.size(), 1, GL_INT);
	object_pass_input.assign(2, "w0", mesh.weight_for_joint0.data(), mesh.weight_for_joint0.size(), 1, GL_FLOAT);
	object_pass_input.assign(3, "vector_from_joint0", mesh.vector_from_joint0.data(), mesh.vector_from_joint0.size(), 3, GL_FLOAT);
	object_pass_input.assign(4, "vector_from_joint1", mesh.vector_from_joint1.data(), mesh.vector_from_joint1.size(), 3, GL_FLOAT);
	object_pass_input.assign(5, "normal", mesh.vertex_normals.data(), mesh.vertex_normals.size(), 4, GL_FLOAT);
	object_pass_input.assign(6, "uv", uv_coordinates.data(), uv_coordinates.size(), 2, GL_FLOAT);

	// TIPS: You won't need vertex position in your solution.
	//       This only serves the stub shader.
	object_pass_input.assign(7, "vert", mesh.vertices.data(), mesh.vertices.size(), 4, GL_FLOAT);
	object_pass_input.assignIndex(mesh.faces.data(), mesh.faces.size(), 3);
	object_pass_input.useMaterials(mesh.materials);
	//cout << " OBJECT PASS" << endl;
	RenderPass object_pass(-1,
			object_pass_input,
			{
			  blending_shader,
			  geometry_shader,
			  fragment_shader
			},
			{ std_model, std_view, std_proj,
			  std_light,
			  std_camera, object_alpha,
			  joint_trans, joint_rot,
			  blend_d_u
			},
			{ "fragment_color" }
			);

	// Setup the render pass for drawing bones
	// FIXME: You won't see the bones until Skeleton::joints were properly
	//        initialized
	std::vector<int> bone_vertex_id;
	std::vector<glm::uvec2> bone_indices;
	for (int i = 0; i < (int)mesh.skeleton.joints.size(); i++) {
		bone_vertex_id.emplace_back(i);
	}
	for (const auto& joint: mesh.skeleton.joints) {
		if (joint.parent_index < 0)
			continue;
		bone_indices.emplace_back(joint.joint_index, joint.parent_index);
	}
	RenderDataInput bone_pass_input;
	bone_pass_input.assign(0, "jid", bone_vertex_id.data(), bone_vertex_id.size(), 1, GL_UNSIGNED_INT);
	bone_pass_input.assignIndex(bone_indices.data(), bone_indices.size(), 2);
	RenderPass bone_pass(-1, bone_pass_input,
			{ bone_vertex_shader, nullptr, bone_fragment_shader},
			{ std_model, std_view, std_proj, joint_trans },
			{ "fragment_color" }
			);

	// FIXME: Create the RenderPass objects for bones here.
	//        Otherwise do whatever you like.

	RenderDataInput cylinder_pass_input;
	// questionable second to last argument (size of element)
	cylinder_pass_input.assign(0, "vertex_position", cylinder_mesh.vertices.data(), cylinder_mesh.vertices.size(), 4, GL_FLOAT);
	cylinder_pass_input.assignIndex(cylinder_mesh.indices.data(), cylinder_mesh.indices.size(), 2);

	RenderPass cylinder_pass(-1, cylinder_pass_input,
		{ cylinder_vertex_shader, nullptr, cylinder_fragment_shader},
		{ std_model, std_view, std_proj, bone_trans},
		{ "fragment_color" }
		);

	RenderDataInput axes_pass_input;
	// questionable second to last argument (size of element)
	axes_pass_input.assign(0, "vertex_position", axes_mesh.vertices.data(), axes_mesh.vertices.size(), 4, GL_FLOAT);
	axes_pass_input.assignIndex(axes_mesh.indices.data(), axes_mesh.indices.size(), 2);

	RenderPass axes_pass(-1, axes_pass_input,
		{ axes_vertex_shader, nullptr, axes_fragment_shader},
		{ std_model, std_view, std_proj, bone_trans},
		{ "fragment_color" }
		);


	GLuint quad_vertex_shader_id = 0;
	CHECK_GL_ERROR(quad_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(quad_vertex_shader_id, 1, &quad_vertex_shader, nullptr));
	glCompileShader(quad_vertex_shader_id);
	CHECK_GL_SHADER_ERROR(quad_vertex_shader_id);

	GLuint quad_fragment_shader_id = 0;
	CHECK_GL_ERROR(quad_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(quad_fragment_shader_id, 1, &quad_fragment_shader, nullptr));
	glCompileShader(quad_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(quad_fragment_shader_id);

	GLuint g_buffer_objects[2]; 

	GLuint quad_vao;
	glGenVertexArrays(1, (GLuint*)&quad_vao);
	CHECK_GL_ERROR(glBindVertexArray(quad_vao));
	CHECK_GL_ERROR(glGenBuffers(2, &g_buffer_objects[0]));
	
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[0]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * quad_vertices.size() * 4, quad_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[1]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * quad_faces.size() * 3,
				quad_faces.data(), GL_STATIC_DRAW));

	GLuint quad_program_id = 0;
	GLint quad_texture_location = 0;
	GLint quad_ortho_location = 0;
	GLint quad_offset_location = 0;

	CHECK_GL_ERROR(quad_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(quad_program_id, quad_vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(quad_program_id, quad_fragment_shader_id));

	CHECK_GL_ERROR(glBindAttribLocation(quad_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(quad_program_id, 0, "fragment_color"));

	glLinkProgram(quad_program_id);
	CHECK_GL_PROGRAM_ERROR(quad_program_id);

	CHECK_GL_ERROR(quad_texture_location =
		glGetUniformLocation(quad_program_id, "renderedTexture"));
	CHECK_GL_ERROR(quad_ortho_location =
		glGetUniformLocation(quad_program_id, "ortho"));
	CHECK_GL_ERROR(quad_offset_location =
		glGetUniformLocation(quad_program_id, "offset"));



	// RenderDataInput quad_pass_input;
	// quad_pass_input.assign(0, "vertex_position", quad_vertices.data(), quad_vertices.size(), 4, GL_FLOAT);
	// quad_pass_input.assignIndex(quad_faces.data(), quad_faces.size(), 3);

	// RenderPass quad_pass(-1, quad_pass_input,
	// 	{ quad_vertex_shader, nullptr, quad_fragment_shader},
	// 	{ std_model, std_view, std_proj},
	// 	{ "fragment_color" }
	// 	);



	float aspect = 0.0f;
	std::cout << "center = " << mesh.getCenter() << "\n";

	bool draw_floor = true;
	bool draw_skeleton = true;
	bool draw_object = true;
	bool draw_cylinder = true;

	
	if (argc >= 3) {
		mesh.loadAnimationFrom(argv[2]);
	}

	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, main_view_width, main_view_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();
#if 0
		std::cerr << model_data() << '\n';
		std::cerr << "call from outside: " << std_model->data_source() << "\n";
		std_model->bind(0);
#endif

		if (gui.isPlaying()) {
			std::stringstream title;
			float cur_time = gui.getCurrentPlayTime();
			title << window_title << " Playing: "
			      << std::setprecision(2)
			      << std::setfill('0') << std::setw(6)
			      << cur_time << " sec";
			glfwSetWindowTitle(window, title.str().data());
			//pass in animation state to updateAnimation
			mesh.updateAnimation(cur_time, gui.getAnimationState());
		} else if (gui.isPoseDirty()) {
			mesh.updateAnimation();
			gui.clearPose();
		}
		// FIXME: update the preview textures here

		if(gui.saveTexture()) {
			GLuint FramebufferName = 0;
			glGenFramebuffers(1, &FramebufferName);
			glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

			GLuint renderedTexture;
			glGenTextures(1, &renderedTexture);
			glBindTexture(GL_TEXTURE_2D, renderedTexture);
			
			gui.addTexture(renderedTexture);
			
			glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, main_view_width, main_view_height, 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			GLuint depthrenderbuffer;
			glGenRenderbuffers(1, &depthrenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, main_view_width, main_view_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
			GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
			glDrawBuffers(1, DrawBuffers);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
			glViewport(0,0,main_view_width,main_view_height);

			int current_bone = gui.getCurrentBone();

			// Draw bones first.
			if (draw_skeleton && gui.isTransparent()) {
				bone_pass.setup();
				// Draw our lines.
				// FIXME: you need setup skeleton.joints properly in
				//        order to see the bones.
				CHECK_GL_ERROR(glDrawElements(GL_LINES,
											bone_indices.size() * 2,
											GL_UNSIGNED_INT, 0));
			}
			draw_cylinder = (current_bone != -1 && gui.isTransparent());
			if (draw_cylinder) {
				cylinder_pass.setup();
				CHECK_GL_ERROR(glDrawElements(GL_LINES,
											cylinder_mesh.indices.size() * 2,
											GL_UNSIGNED_INT, 0));
				axes_pass.setup();
				CHECK_GL_ERROR(glDrawElements(GL_LINES,
											axes_mesh.indices.size() * 2,
											GL_UNSIGNED_INT, 0));
			}

			// Then draw floor.
			if (draw_floor) {
				floor_pass.setup();
				// Draw our triangles.
				CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
											floor_faces.size() * 3,
											GL_UNSIGNED_INT, 0));
			}

			// Draw the model
			if (draw_object) {
				object_pass.setup();
				int mid = 0;
				while (object_pass.renderWithMaterial(mid))
					mid++;

			}

			glBindFramebuffer(GL_FRAMEBUFFER,0);
			glClear(GL_COLOR_BUFFER_BIT);
			gui.resetTexture();

			
		}

		int current_bone = gui.getCurrentBone();

		// Draw bones first.
		if (draw_skeleton && gui.isTransparent()) {
			bone_pass.setup();
			// Draw our lines.
			// FIXME: you need setup skeleton.joints properly in
			//        order to see the bones.
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
			                              bone_indices.size() * 2,
			                              GL_UNSIGNED_INT, 0));
		}
		draw_cylinder = (current_bone != -1 && gui.isTransparent());
		if (draw_cylinder) {
			cylinder_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
			                              cylinder_mesh.indices.size() * 2,
			                              GL_UNSIGNED_INT, 0));
			axes_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
			                              axes_mesh.indices.size() * 2,
			                              GL_UNSIGNED_INT, 0));
		}

		// Then draw floor.
		if (draw_floor) {
			floor_pass.setup();
			// Draw our triangles.
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
			                              floor_faces.size() * 3,
			                              GL_UNSIGNED_INT, 0));
		}

		// Draw the model
		if (draw_object) {
			object_pass.setup();
			int mid = 0;
			while (object_pass.renderWithMaterial(mid))
				mid++;
#if 0
			// For debugging also
			if (mid == 0) // Fallback
				CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, mesh.faces.size() * 3, GL_UNSIGNED_INT, 0));
#endif
		}
		//glViewport(main_view_width, main_view_height - preview_height, preview_width, preview_height);
		glViewport(main_view_width, 0, preview_width, main_view_height);
		vector<GLuint> texture_locs = gui.getTextureLocs();
		for (int quad = 0; quad <  texture_locs.size(); ++quad) {
		//glViewport(main_view_width, main_view_height - (quad + 1) *preview_height, preview_width, preview_height);

			GLuint text0 = texture_locs[quad];
			glm::mat4 proj = glm::ortho(-1.0f,1.0f,-3.0f,3.0f);
			glm::vec2 offset = glm::vec2(0,-2*quad + 2 - gui.getScrollOffset());
			//cout<<"offset "<<glm::to_string(offset)<<endl;
			//draw a quad
			CHECK_GL_ERROR(glBindVertexArray(quad_vao));
			CHECK_GL_ERROR(glUseProgram(quad_program_id));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, text0);
			CHECK_GL_ERROR(	glUniform1i(quad_texture_location, 0));
			CHECK_GL_ERROR(	glUniformMatrix4fv(quad_ortho_location, 1, GL_FALSE, &proj[0][0]));
			CHECK_GL_ERROR(	glUniform2fv(quad_offset_location, 1, &offset[0]));
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, quad_faces.size() * 3, GL_UNSIGNED_INT, 0));


		}
		glViewport(0, 0, main_view_width, main_view_height);

		if (gui.saveScreenshot()) {
			unsigned char* pixels = new unsigned char[window_width * window_height * 3];
			glReadPixels(0, 0, window_width, window_height, GL_RGB, GL_UNSIGNED_BYTE,pixels);
			string name = "capture.jpg";
			bool success = SaveJPEG(name, window_width, window_height, pixels);
			gui.resetScreenshot();
		}

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
