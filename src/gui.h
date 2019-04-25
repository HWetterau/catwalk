#ifndef SKINNING_GUI_H
#define SKINNING_GUI_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include "bone_geometry.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <iostream>
#include <string>
#include <glm/gtx/string_cast.hpp>

struct Mesh;

/*
 * Hint: call glUniformMatrix4fv on thest pointers
 */
struct MatrixPointers {
	const glm::mat4 *projection, *model, *view;
};

class GUI {
public:
	GUI(GLFWwindow*, int view_width = -1, int view_height = -1, int preview_height = -1);
	~GUI();
	void assignMesh(Mesh*);

	void keyCallback(int key, int scancode, int action, int mods);
	void mousePosCallback(double mouse_x, double mouse_y);
	void mouseButtonCallback(int button, int action, int mods);
	void mouseScrollCallback(double dx, double dy);
	void updateMatrices();
	MatrixPointers getMatrixPointers() const;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void MouseScrollCallback(GLFWwindow* window, double dx, double dy);

	glm::mat4 boneTransform();

	glm::vec3 getCenter() const { return center_; }
	const glm::vec3& getCamera() const { return eye_; }
	bool isPoseDirty() const { return pose_changed_; }
	void clearPose() { pose_changed_ = false; }
	const float* getLightPositionPtr() const { return &light_position_[0]; }

	
	int getCurrentBone() const { return current_bone_; }
	const int* getCurrentBonePointer() const { return &current_bone_; }
	bool setCurrentBone(int i);

	bool saveScreenshot() const { return save_screen_; }
	void resetScreenshot() { save_screen_ = false; }

	bool isTransparent() const { return transparent_; }
	bool isPlaying() const { return play_; }
	float getCurrentPlayTime() const;

	AnimationState* getAnimationState() { return state; }
	bool saveTexture() const { return save_texture_; }
	void resetTexture() { save_texture_ = false; }

	void addTexture(GLuint loc) { 
		if (replace_texture != -1){
			texture_locations[replace_texture] = loc;

		} else if (cursor && selected_frame != -1 && selected_frame < getNumKeyframes()){
		
			texture_locations.insert(texture_locations.begin() + selected_frame, loc);
		}else {
			texture_locations.push_back(loc);
		}
	}
	vector<GLuint> getTextureLocs() const { return texture_locations;}
	double getScrollOffset() const {return scroll_offset;}
	int getSelectedFrame() const {return selected_frame;}
	int getNumKeyframes() const { return mesh_->skeleton.keyframes.size(); }

	int replaceTexture() const { return replace_texture; }
	void resetReplaceTexture() { replace_texture = -1; }
	bool getCursor() const {return cursor;}

	glm::mat4 getProjection() const { return projection_matrix_; }
	glm::mat4 getView() const { return view_matrix_; }
	glm::mat4 lightTransform();
	void changeCamera(glm::vec3 eye, glm::fquat rot, float camera_dist) {
		//cout <<"eye "<<glm::to_string(eye)<<" eye_ "<<glm::to_string(eye_)<<endl;
		//cout<<"use eye "<<glm::to_string(eye)<<endl;
		eye_ = eye; 
		rel_rot = glm::mat4_cast(rot);
		glm::mat4 trans = glm::mat4(1.0);
		trans = glm::translate(trans,eye_);
		orientation_ = glm::mat3(rel_rot * glm::mat4(start_orientation_) * trans);
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
		center_ = eye_ + camera_distance_ * look_;
		view_matrix_ = glm::lookAt(eye_, center_, up_);
		
	 }
	glm::vec4 getLightPosition() {return light_position_;}
	void setLightPosition(glm::vec4 lightpos) {light_position_ = lightpos ;}
	bool getOnLight(){return on_light_;}

	glm::vec4 getLightColor(){return light_color_;}
	void setLightColor(glm::vec4 color){ light_color_ = color;}
		
	enum {x_axis,z_axis,y_axis, none};
	enum {WHITE,RED,ORANGE,YELLOW,GREEN,BLUE,PURPLE,NUMCOLORS};
		

private:
	GLFWwindow* window_;
	Mesh* mesh_;

	int window_width_, window_height_;
	int view_width_, view_height_;
	int preview_height_;

	bool drag_state_ = false;
	bool fps_mode_ = false;
	bool pose_changed_ = true;
	bool transparent_ = false;
	bool on_light_ = false;
	int current_bone_ = -1;
	int current_button_ = -1;
	float roll_speed_ = M_PI / 64.0f;
	float last_x_ = 0.0f, last_y_ = 0.0f, current_x_ = 0.0f, current_y_ = 0.0f;
	float camera_distance_ = 30.0;
	float pan_speed_ = 0.1f;
	float rotation_speed_ = 0.02f;
	float zoom_speed_ = 0.1f;
	float aspect_;

	bool save_screen_ = false;

	glm::vec3 eye_ = glm::vec3(0.0f, 0.1f, camera_distance_);
	glm::vec3 rel_pos = eye_;
	glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 tangent_ = glm::cross(look_, up_);
	glm::vec3 center_ = eye_ - camera_distance_ * look_;
	glm::mat3 orientation_ = glm::mat3(tangent_, up_, look_);
	glm::mat3 start_orientation_ = glm::mat3(tangent_, up_, look_);
	glm::vec4 light_position_ = glm::vec4(0.0f, 50.0f, 0.0f, 1.0f);
	glm::vec4 light_color_= glm::vec4(1,1,1,1);
	glm::mat4 rel_rot = glm::mat4(1.0);

	glm::mat4 view_matrix_ = glm::lookAt(eye_, center_, up_);
	glm::mat4 projection_matrix_;
	glm::mat4 model_matrix_ = glm::mat4(1.0f);

	int chosen_axis = none;

	bool captureWASDUPDOWN(int key, int action);

	bool play_ = false;

	//store keyframes in gui?

	AnimationState* state;
	chrono::time_point<chrono::steady_clock> play_start;
	float pause_time = 0;
	bool save_texture_ = false;
	vector<GLuint> texture_locations;
	double scroll_offset = 0;
	int selected_frame = -1;
	int replace_texture = -1;
	bool cursor = false;
	int color = WHITE;
};

#endif
