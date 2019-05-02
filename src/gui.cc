#include "gui.h"
#include "config.h"

#include <iostream>
#include <algorithm>
#include <debuggl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


#include <glm/gtx/string_cast.hpp>


namespace {
	// FIXME: Implement a function that performs proper
	//        ray-cylinder intersection detection
	// TIPS: The implement is provided by the ray-tracer starter code.
}
using namespace std;

bool sphereIntersect(glm::dvec3 p, glm::dvec3 dir, double& t)
{
	
	glm::dvec3 oc = -p;
	double a = glm::dot(dir,dir);
	double b = 2.0* glm::dot(oc, dir);
	double c = glm::dot(oc,oc)-1;
	double discriminant = b * b - 4 * a * c;
	// double b = glm::dot(v, dir);
	// double discriminant = b*b - glm::dot(v,v) + 1;

	if( discriminant < 0.0 ) {
		return false;
	}

	discriminant = sqrt( discriminant );
	double t2 = b + discriminant;

	if( t2 <= 0.0001 ) {
		return false;
	}

	
	double t1 = b - discriminant;

	if( t1 > 0.0001) {
		t = t1;
	} else {
		t = t2;
	}

	return true;
}




bool intersectBody(glm::dvec3& p, glm::dvec3& dir, double& t, double height){
	double x0 = p[0];
	double y0 = p[1];
	double x1 = dir[0];
	double y1 = dir[1];

	double a = x1*x1+y1*y1;
	double b = 2.0*(x0*x1 + y0*y1);
	double c = x0*x0 + y0*y0 - pow(kCylinderRadius,2);

	if( 0.0 == a ) {
		// This implies that x1 = 0.0 and y1 = 0.0, which further
		// implies that the ray is aligned with the body of the cylinder,
		// so no intersection.
		return false;
	}

	double discriminant = b*b - 4.0*a*c;

	if( discriminant < 0.0 ) {
		return false;
	}

	discriminant = sqrt( discriminant );

	double t2 = (-b + discriminant) / (2.0 * a);

	if( t2 <= 0.00001 ) {
		return false;
	}

	double t1 = (-b - discriminant) / (2.0 * a);

	if( t1 > 0.00001 ) {
		// Two intersections.
		glm::dvec3 P = p + (t1 * dir);
		double z = P[2];
		if( z >= 0.0 && z <= height) {
			// It's okay
			t = t1;
			return true;
		}
	}

	glm::dvec3 P = p + (t2 * dir);
	double z = P[2];
	// change to length of cylinder
	if( z >= 0.0 && z <= height) {
		t = t2;
		return true;
	}

	return false;
}

bool intersectCaps(glm::dvec3& pos, glm::dvec3 dir, double& t){
	double pz = pos[2];
	double dz = dir[2];

	if( 0.0 == dz ) {
		return false;
	}

	double t1;
	double t2;

	if( dz > 0.0 ) {
		t1 = (-pz)/dz;
		t2 = (1.0-pz)/dz;
	} else {
		t1 = (1.0-pz)/dz;
		t2 = (-pz)/dz;
	}

	if( t2 < 0.00001 ) {
		return false;
	}

	if( t1 >= 0.00001 ) {
		glm::dvec3 p = pos + (t1 * dir);
		if( (p[0]*p[0] + p[1]*p[1]) <= pow(kCylinderRadius,2) ) {
			t = t1;
			return true;
		}
	}
	glm::dvec3 p =  pos + (t2 * dir);
	// change to radius ^2
	if( (p[0]*p[0] + p[1]*p[1]) <= pow(kCylinderRadius,2) ) {
		t = t2;
		return true;
	}
	return false;
}


bool intersectLocal(glm::dvec3 p, glm::dvec3 dir, double& t, double height){

	if( intersectCaps(p, dir, t) ) {
		double t2;
		if( intersectBody(p, dir, t2, height) ) {
			if( t2 < t ) {
				t = t2;
			}
		}
		return true;
	} else {
		return intersectBody(p, dir, t, height);
	}
}

//GUI gui(window, main_view_width, main_view_height, timeline_height, preview_height);

GUI::GUI(GLFWwindow* window, int view_width, int view_height, int timeline_height, int preview_height)
	:window_(window), preview_height_(preview_height), timeline_height_(timeline_height)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);
	glfwSetScrollCallback(window_, MouseScrollCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	if (view_width < 0 || view_height < 0) {
		view_width_ = window_width_;
		view_height_ = window_height_;
	} else {
		view_width_ = view_width;
		view_height_ = view_height;
	}
	float aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	state = new AnimationState();
	sceneState = new SceneState();

	cout << "view width " << view_width_ << " view height " << view_height_ << endl;
}

GUI::~GUI()
{
}

void GUI::assignMesh(Mesh* mesh)
{
	mesh_ = mesh;
	center_ = mesh_->getCenter();
}

void GUI::computeColor(){
	double intensity = intensity_;
	switch(color) {
		case WHITE:
			light_color_ = glm::vec4(intensity,intensity,intensity,1);
			break;
		case RED:
			light_color_ = glm::vec4(intensity,0.0,0.0,1);
			break;
		case ORANGE:
			light_color_ = glm::vec4(intensity,intensity/2.0,0.0,1);
			break;
		case YELLOW:
			light_color_ = glm::vec4(intensity,intensity,0.0,1);
			break;
		case GREEN:
			light_color_ = glm::vec4(0.0,intensity,0.0,1);
			break;
		case BLUE:
			light_color_ = glm::vec4(0.0,0.0,intensity,1);
			break;
		case PURPLE:
			light_color_ = glm::vec4(intensity/2.0,0.0,intensity,1);
			break;
		default:
			break;
	}
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
#if 0
	if (action != 2)
		std::cerr << "Key: " << key << " action: " << action << std::endl;
#endif
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
		//FIXME save out a screenshot using SaveJPEG
		save_screen_ = true;
	}
	if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
		if (action == GLFW_RELEASE)
			mesh_->saveAnimationTo("animation.json");
		return ;
	}

	if (mods == 0 && captureWASDUPDOWN(key, action))
		return ;
	if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
		float roll_speed;
		if (key == GLFW_KEY_RIGHT)
			roll_speed = -roll_speed_;
		else
			roll_speed = roll_speed_;
		// FIXME: actually roll the bone here
		glm::mat4 r = glm::rotate(roll_speed, glm::normalize(mesh_->skeleton.joints[current_bone_].init_rel_position));
		mesh_->skeleton.rotate(mesh_->skeleton.joints[current_bone_].parent_index, current_bone_, r);
		pose_changed_ = true;

	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		//fps_mode_ = !fps_mode_;
		color++; 
		color = color % NUMCOLORS;
		computeColor();
		
	} else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
		current_bone_--;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
		current_bone_++;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
		transparent_ = !transparent_;
	} else if (key == GLFW_KEY_F && action == GLFW_RELEASE) {
		//create a keyframe
		KeyFrame k;
		for(int bone = 0; bone < mesh_->getNumberOfBones(); ++bone) {
			k.rel_rot.push_back(glm::quat_cast(mesh_->skeleton.joints[bone].t));
		}
		k.time = pause_time;

		if (cursor && selected_frame != -1 && selected_frame < getNumKeyframes()) {
			//insert before
			mesh_->skeleton.keyframes.insert(mesh_->skeleton.keyframes.begin() + selected_frame, k);
		} else {
			mesh_->skeleton.keyframes.push_back(k);
		}
		state->end_keyframe = mesh_->skeleton.keyframes.size()-1;
		save_texture_ = true;

	}else if (key == GLFW_KEY_L && action == GLFW_RELEASE) {
		LightKeyFrame lk;
		lk.light_pos = light_position_;
		lk.light_color = light_color_;
		lightKeyframes.push_back(lk);
		sceneState->end_light_keyframe = lightKeyframes.size() - 1;
	
	}else if (key == GLFW_KEY_V && action == GLFW_RELEASE) {
		CameraKeyFrame ck;
		ck.camera_pos = eye_;
		ck.camera_rot = orientation_;
		ck.camera_dist = camera_distance_;
		cameraKeyframes.push_back(ck);
		cout << "size after " << cameraKeyframes.size() << endl;
		sceneState->end_camera_keyframe = cameraKeyframes.size() - 1;
	
	}else if (key == GLFW_KEY_P && action == GLFW_RELEASE) {
		//either play/pause animation
		
		play_ = !play_;
		if (!play_) {
			pause_time = getCurrentPlayTime();
		}
		play_start = chrono::steady_clock::now();

		state->old_time = pause_time;
		sceneState->old_time = pause_time;
		sceneState->old_time2 = pause_time;

	} else if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
		//rewind animation
		state->current_time = 0;
		state->current_keyframe = 0;
		state->next_keyframe = 1;
		state->old_time = 0;

		sceneState->current_time = 0;
		
		sceneState->current_light_keyframe = 0;
		sceneState->next_light_keyframe = 1;
		sceneState->old_time = 0;

		sceneState->current_camera_keyframe = 0;
		sceneState->next_camera_keyframe = 1;
		sceneState->old_time2 = 0;

		pause_time = 0;
		play_start = chrono::steady_clock::now();

	} else if (key == GLFW_KEY_PAGE_UP && action == GLFW_RELEASE) {
		if( selected_frame > 0) {
			selected_frame--;
		}
	} else if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_RELEASE) {
		if (selected_frame != -1 && selected_frame < getNumKeyframes() -1){
			selected_frame++;
		}
	} else if (key == GLFW_KEY_U && action == GLFW_RELEASE) {
		//update
		if(selected_frame != -1 && selected_frame < getNumKeyframes()) {
			KeyFrame k;
			for(int bone = 0; bone < mesh_->getNumberOfBones(); ++bone) {
				k.rel_rot.push_back(glm::quat_cast(mesh_->skeleton.joints[bone].t));
			}
			//k.light_pos = light_position_;
			//k.camera_pos = eye_;
			//k.camera_pos = rel_pos;
			// k.camera_rot = orientation_;
			// k.light_color = light_color_;
			// k.camera_dist = camera_distance_;
			mesh_->skeleton.keyframes[selected_frame] = k;
			replace_texture = selected_frame;
			save_texture_ = true;
		}

	} else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
		//reset skeleton
		if(selected_frame >= getNumKeyframes()){
			selected_frame = -1;
		}
		if(selected_frame != -1){
			mesh_->changeSkeleton(mesh_->skeleton.keyframes[selected_frame]);
			pose_changed_ = true;
		}
	} else if (key == GLFW_KEY_DELETE && action == GLFW_RELEASE) {
		//delete keyframe
		if(selected_frame != -1 && selected_frame < getNumKeyframes()) {
			mesh_->skeleton.keyframes.erase(mesh_->skeleton.keyframes.begin() + selected_frame);
			texture_locations.erase(texture_locations.begin() + selected_frame);
			state->end_keyframe = mesh_->skeleton.keyframes.size()-1;
			if(state->next_keyframe > state->end_keyframe) {
				state->current_keyframe = state->end_keyframe;
				state->next_keyframe = state->end_keyframe;
			}

		}
		selected_frame = -1;
	} else if (key == GLFW_KEY_I && action == GLFW_RELEASE) {
		//toggle cursor
		cursor = !cursor;
	}else if (key == GLFW_KEY_EQUAL && action != GLFW_RELEASE) {
		if(intensity_< 1.0){
			intensity_+=.05;
		}
		if(intensity_> 1.0){
			intensity_ = 1.0;
		}
		computeColor();
	}else if (key == GLFW_KEY_MINUS && action != GLFW_RELEASE) {
		if(intensity_> 0.0){
			intensity_-=.05;
		}
		if(intensity_< 0.0){
			intensity_ = 0.0;
		}
		computeColor();
	}

	// FIXME: implement other controls here.
}

glm::mat4 GUI::boneTransform(){
	Joint j = mesh_->skeleton.joints[current_bone_];
	glm::vec3 parentpos =  mesh_->skeleton.joints[j.parent_index].position;
	glm::vec3 tangent = glm::normalize( j.init_rel_position);
		glm::vec3 n;
		if(tangent[0] <= tangent[1] && tangent[0] <= tangent[2]){
				n = glm::vec3(1,0,0);
		} else if (tangent[1] <= tangent[0] && tangent[1] <= tangent[2]) {
				n = glm::vec3(0,1,0);
		} else {
				n = glm::vec3(0,0,1);
		}
		glm::vec3 normal = glm::normalize(glm::cross(tangent, n));
		glm::vec3 bitan = glm::normalize(glm::cross(tangent,normal));
		double height = glm::distance(glm::vec3(0),j.init_rel_position);
		glm::vec3 pos = (parentpos + j.position) * glm::vec3(.5, .5, .5);
		glm::mat4 scale = glm::mat4(kCylinderRadius, 0, 0, 0, 0, height, 0, 0, 0, 0, kCylinderRadius, 0, 0, 0, 0, 1);
		glm::mat4 toworld = (glm::mat4(glm::vec4(normal,0),glm::vec4(tangent,0),glm::vec4(bitan,0),glm::vec4(0,0,0,1)));
		return mesh_->skeleton.joints[j.parent_index].d * toworld *  scale;
}

glm::mat4 GUI::lightTransform(){
	glm::mat4 trans = glm::mat4(1.0);
	return glm::translate(trans,glm::vec3(light_position_));
	
}

glm::vec3 GUI::lightSpline(int curframe, float t){
	int cp0 = glm::clamp<int>(curframe - 1, 0, lightKeyframes.size() - 1);
    int cp1 = glm::clamp<int>(curframe,     0, lightKeyframes.size()- 1);
    int cp2 = glm::clamp<int>(curframe + 1, 0, lightKeyframes.size() - 1);
    int cp3 = glm::clamp<int>(curframe + 2, 0, lightKeyframes.size() - 1);
	float local_t = glm::fract(t);
	return glm::catmullRom(lightKeyframes[cp0].light_pos, lightKeyframes[cp1].light_pos, lightKeyframes[cp2].light_pos, lightKeyframes[cp3].light_pos, local_t);
}
glm::vec3 GUI::cameraPosSpline(int curframe,float t){
	int cp0 = glm::clamp<int>(curframe - 1, 0, cameraKeyframes.size() - 1);
    int cp1 = glm::clamp<int>(curframe,     0, cameraKeyframes.size() - 1);
    int cp2 = glm::clamp<int>(curframe + 1, 0, cameraKeyframes.size() - 1);
    int cp3 = glm::clamp<int>(curframe + 2, 0, cameraKeyframes.size() - 1);
	float local_t = glm::fract(t);
	return glm::catmullRom(cameraKeyframes[cp0].camera_pos, cameraKeyframes[cp1].camera_pos, cameraKeyframes[cp2].camera_pos, cameraKeyframes[cp3].camera_pos, local_t);
}

glm::mat3 GUI::cameraRotSpline(int curframe, float t){
	int cp0 = glm::clamp<int>(curframe - 1, 0, cameraKeyframes.size() - 1);
    int cp1 = glm::clamp<int>(curframe,     0, cameraKeyframes.size() - 1);
    int cp2 = glm::clamp<int>(curframe + 1, 0, cameraKeyframes.size() - 1);
    int cp3 = glm::clamp<int>(curframe + 2, 0, cameraKeyframes.size() - 1);
	float local_t = glm::fract(t);

	glm::mat3 m1 = cameraKeyframes[cp0].camera_rot;
	glm::mat3 m2 = cameraKeyframes[cp1].camera_rot;
	glm::mat3 m3 = cameraKeyframes[cp2].camera_rot;
	glm::mat3 m4 = cameraKeyframes[cp3].camera_rot;

	// glm::fquat prev = glm::quat_cast(cameraKeyframes[cp1].camera_rot);
	// glm::fquat next = glm::quat_cast(cameraKeyframes[cp2].camera_rot);

	// glm::fquat cur = glm::slerp(prev, next, t);

	// glm::fquat a = glm::intermediate(prev,cur,next);
	// glm::fquat b = glm::intermediate(a,cur,next);


	glm::mat3 result = glm::mat3(1.0);

	//cout << "m1 " << glm::to_string(m1) << endl;
	result[1] = glm::normalize(glm::catmullRom(glm::column(m1, 1), glm::column(m2, 1), glm::column(m3, 1), glm::column(m4, 1), local_t));
	glm::vec3 temp = glm::normalize(glm::catmullRom(glm::column(m1, 2), glm::column(m2, 2), glm::column(m3, 2), glm::column(m4, 2), local_t));

	temp-= glm::dot(temp, glm::column(result, 1))*glm::column(result, 1);
	temp = glm::normalize(temp);
	result[2] = temp;

	result[0] = glm::normalize(glm::cross(glm::column(result, 1), glm::column(result, 2)));

	return result;
	//return glm::squad(skeleton.keyframes[cp0].camera_rot, skeleton.keyframes[cp3].camera_rot, skeleton.keyframes[cp1].camera_rot, skeleton.keyframes[cp2].camera_rot,  local_t);
	//return glm::toMat3(glm::squad(prev, next, a, b, t));
	//return glm::toMat3(KeyFrame::boneSquad(prev, cur, next, local_t));
}


void GUI::updateScene(float t){
	if (t != -1 ) {
		if ( lightKeyframes.size()>0){
			//LIGHT
			sceneState->current_time = t;
			float fps = 1.0;
			bool interpolate = true;
			int cur = sceneState->current_light_keyframe;

			if ( cur == sceneState->end_light_keyframe){

				interpolate = false;

			} else if (sceneState->current_time - sceneState->old_time > (1/fps)) {

				sceneState->old_time = sceneState->current_time;

				if (sceneState->next_light_keyframe < sceneState->end_light_keyframe) {
					sceneState->current_light_keyframe = sceneState->next_light_keyframe;
					sceneState->next_light_keyframe++;
					interpolate = true;
				} else {
					sceneState->current_light_keyframe = sceneState->next_light_keyframe;
				}
			}
			float interp = fps * (sceneState->current_time - sceneState->old_time);
			if (interpolate){
				light_position_ = glm::vec4(lightSpline(sceneState->current_light_keyframe, interp),1);
				light_color_ = glm::mix(lightKeyframes[sceneState->current_light_keyframe].light_color,lightKeyframes[sceneState->next_light_keyframe].light_color,interp);	
			}
		}
		if(cameraKeyframes.size()>0){
			//CAMERA
			sceneState->current_time = t;
			float fps = 1.0;
			bool interpolate = true;
			int cur = sceneState->current_camera_keyframe;
			
			if ( cur == sceneState->end_camera_keyframe){

				interpolate = false;

			} else if (sceneState->current_time - sceneState->old_time2 > (1/fps)) {

				sceneState->old_time2 = sceneState->current_time;

				if (sceneState->next_camera_keyframe < sceneState->end_camera_keyframe) {
					sceneState->current_camera_keyframe = sceneState->next_camera_keyframe;
					sceneState->next_camera_keyframe++;
					interpolate = true;
				} else {
					sceneState->current_camera_keyframe = sceneState->next_camera_keyframe;
				}
			}
			float interp = fps * (sceneState->current_time - sceneState->old_time2);
			if (interpolate){
				glm::vec3 temp_eye = cameraPosSpline(sceneState->current_camera_keyframe, interp);
				float camera_dist = glm::mix(cameraKeyframes[sceneState->current_camera_keyframe].camera_dist, cameraKeyframes[sceneState->next_camera_keyframe].camera_dist,interp);
				glm::mat3 temp_rot = cameraRotSpline(sceneState->current_camera_keyframe, interp);	
				changeCamera(temp_eye,temp_rot,camera_dist);
			}
		}
	}
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;

	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	if (mouse_x > view_width_)
		return ;
	glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, view_width_, view_height_);

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;
	bool drag_bone = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT && transparent_;
	bool drag_light = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_MIDDLE && chosen_axis != none;
	bool drag_scrub = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;


	
	if(current_y_ >= view_height_){
		//on the timeline
		 glm::mat4 proj = glm::ortho(-1.0f,1.0f,-1.0f,1.0f);
		glm::vec4 coords = glm::vec4(pause_time *0.0452 + -1,0,0,1);
		glm::vec4 coords2 = glm::vec4(pause_time *0.0452+ -0.985,0,0,1);
		coords = proj  * coords;
		coords2 = proj  * coords2;
		
		float comparex = current_x_/view_width_ * 2 - 1;
		if(!play_){
			if(comparex >= coords[0] && comparex <= coords2[0] && drag_scrub){
				
				pause_time += (delta_x * .04);
				pause_time = glm::clamp(pause_time,0.0f,45.0f);
				move_scrub = true;
				scrubbing_ = true;
			} else if (move_scrub) {
				pause_time += (delta_x * .04);
				pause_time = glm::clamp(pause_time,0.0f,45.0f);
				scrubbing_ = true;
			}else{
				scrubbing_ = false;
			}
		}
		return;
	}


	if (drag_camera) {
		glm::vec3 axis = glm::normalize(
				orientation_ *
				glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
				);
		glm::mat4 rot =	glm::rotate(rotation_speed_, axis);	
		orientation_ =
			glm::mat3(rot * glm::mat4(orientation_));
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
	} else if (drag_bone && current_bone_ != -1) {
		// FIXME: Handle bone rotation

		// if(current_bone_ == 0) {
		// 	//root joint translation
		// 	glm::mat4 t = glm::mat4(1.0);
		// 	t[3][0] = delta_x;
		// 	t[3][1] = delta_y;
		// 	mesh_->skeleton.translate(t);
		// 	pose_changed_ = true;
		// }
		// else{
			glm::vec4 parentpos =  glm::vec4(mesh_->skeleton.joints[mesh_->skeleton.joints[current_bone_].parent_index].position, 1);
			parentpos = projection_matrix_ * view_matrix_ * parentpos;
			parentpos = parentpos / glm::vec4(parentpos.w,parentpos.w,parentpos.w,parentpos.w);
			glm::vec2 ndc_coords = glm::vec2((parentpos.x+1)*view_width_/2, (parentpos.y+1)*(view_height_ )/2);
			
			glm::vec2 a = mouse_start - ndc_coords;
			glm::vec2 b = mouse_end - ndc_coords;
			
			glm::mat4 parentcoords = mesh_->skeleton.joints[mesh_->skeleton.joints[current_bone_].parent_index].d;
			double det = a.x*b.y - a.y*b.x;
			// float angle = atan2(det, glm::dot(a,b)) * 180 / 3.14;
			float angle = atan2(det, glm::dot(a,b));
	
			//change look to local coordinates

			glm::mat4 r = glm::rotate(-angle, glm::vec3(glm::inverse(parentcoords)*glm::vec4(look_,0)));
			mesh_->skeleton.rotate(mesh_->skeleton.joints[current_bone_].parent_index,current_bone_, r);
			pose_changed_ = true;
		// }
		return;
	} else if(drag_light){
			glm::vec4 lightpos =  light_position_;
			lightpos = projection_matrix_ * view_matrix_ * lightpos;
			lightpos = lightpos / glm::vec4(lightpos.w,lightpos.w,lightpos.w,lightpos.w);
			glm::vec2 ndc_coords = glm::vec2((lightpos.x+1)*view_width_/2, (lightpos.y+1)*view_height_/2);
			glm::vec2 drag = mouse_end-mouse_start;
			glm::vec4 lightend;
			glm::vec2 ndc_coords2;
			glm::vec2 axis_dir;
			float drag_speed = 0.001;
		switch(chosen_axis){
			case x_axis:
				lightend =  light_position_+glm::vec4(4,0,0,0);
				lightend = projection_matrix_ * view_matrix_ * lightend;
				lightend = lightend / glm::vec4(lightend.w,lightend.w,lightend.w,lightend.w);
				ndc_coords2 = glm::vec2((lightend.x+1)*view_width_/2, (lightend.y+1)*view_height_/2);
				axis_dir = ndc_coords2 - ndc_coords;
				//cout<<"dot product "<<glm::dot(axis_dir,drag)*0.01<<endl;
				light_position_.x+=glm::dot(axis_dir,drag)*drag_speed;
				break;
			case y_axis:
				lightend =  light_position_+glm::vec4(0,4,0,0);
				lightend = projection_matrix_ * view_matrix_ * lightend;
				lightend = lightend / glm::vec4(lightend.w,lightend.w,lightend.w,lightend.w);
				ndc_coords2 = glm::vec2((lightend.x+1)*view_width_/2, (lightend.y+1)*view_height_/2);
				axis_dir = ndc_coords2 - ndc_coords;
				light_position_.y+=glm::dot(axis_dir,drag)*drag_speed;
				break;
			case z_axis:
				lightend =  light_position_+glm::vec4(0,0,4,0);
				lightend = projection_matrix_ * view_matrix_ * lightend;
				lightend = lightend / glm::vec4(lightend.w,lightend.w,lightend.w,lightend.w);
				ndc_coords2 = glm::vec2((lightend.x+1)*view_width_/2, (lightend.y+1)*view_height_/2);
				axis_dir = ndc_coords2 - ndc_coords;
				light_position_.z+=glm::dot(axis_dir,drag)*drag_speed;
				break;
			default:
				break;
		}
	}

	// FIXME: highlight bones that have been moused over
	//go from screen coords to ndc (divide by width, multiply by 2 and subtract 1)
	//z is at 1
	// multiply by mvp inverse (wmouse)
	//position is eye
	// direction is wmouse - eye
	//loop through joints and transform ray to be in local coord
	// call cylinderintersect
	//set current bone index and stop
	double ndc_x = mouse_x * 2/ view_width_ -1;
	double ndc_y = (window_height_ - mouse_y) * 2 / view_height_ -1;
	glm::vec4 ndc_coords = glm::vec4(ndc_x,ndc_y, 1, 1);
	//cout<<"ndc coords "<< glm::to_string(ndc_coords)<<endl;
	//glm::mat4 vp =  view_matrix_ * projection_matrix_;
	glm::vec4 world_coords = glm::inverse(view_matrix_)*glm::inverse(projection_matrix_) * ndc_coords;
	world_coords = world_coords/world_coords[3];
	//cout<<"world coords "<< glm::to_string(world_coords)<<endl;
	glm::vec4 dir = world_coords - glm::vec4(eye_,1);
	//cout<<"ray direction "<< glm::to_string(dir)<<endl;

	if(!drag_state_){
		glm::mat4 light_coords = glm::mat4(1.0);
		light_coords[3]= light_position_;
		light_coords = glm::inverse(light_coords);
		// cout<<"light pos "<<glm::to_string(light_position_)<<endl;
		double foo = 400;
		chosen_axis = none;

		if (intersectLocal(glm::dvec3(light_coords*glm::vec4(eye_,1)), glm::dvec3(light_coords*dir),foo,4.0)){
			//Z axis!!
			chosen_axis = z_axis;
		}
		double xt;
		light_coords = glm::inverse(glm::mat4(glm::vec4(0,1,0,0),glm::vec4(0,0,1,0),glm::vec4(1,0,0,0), light_position_));
		if (intersectLocal(glm::dvec3(light_coords*glm::vec4(eye_,1)), glm::dvec3(light_coords*dir),xt,4.0)){
			//x axis!!
				
			if(chosen_axis != none && xt<foo){
				foo = xt;
				chosen_axis = x_axis;
			}else if(chosen_axis == none){
				chosen_axis = x_axis;
			}
		}
		light_coords = glm::inverse(glm::mat4(glm::vec4(0,0,1,0),glm::vec4(1,0,0,0),glm::vec4(0,1,0,0), light_position_-glm::vec4(0,4,0,0)));
		if (intersectLocal(glm::dvec3(light_coords*glm::vec4(eye_,1)), glm::dvec3(light_coords*dir),xt,4.0)){
			//y axis!!
			if(chosen_axis != none && xt<foo){
				foo = xt;
				chosen_axis = y_axis;
			}else if(chosen_axis == none){
				chosen_axis = y_axis;
			}
		}
		if(chosen_axis != none){
			on_light_ = true;
		}else{
			on_light_= false;
		}
	}


	int min_bone = -1;
	double min_time = 0;
	for(int bone = 1; bone < mesh_->getNumberOfBones(); ++bone){
		Joint j = mesh_->skeleton.joints[bone];
		glm::vec3 parentpos =  mesh_->skeleton.joints[j.parent_index].position;
		//glm::vec3  = j.init_rel_position

		//mat4 t: tangent is j. init_position - parent init init_position
		// normal: find smallest value of tangent, set to 1 then cross
		//binormal: cross tangent and normal
		//relative pos is init position in parents coord system multiply by inverse of parent's D
		//origin is the init_position
		glm::vec3 tangent = glm::normalize(parentpos - j.position );
		glm::vec3 n;
		glm::vec3 abstan = glm::abs(tangent);
		if(abstan[0] <= abstan[1] && abstan[0] <= abstan[2]){
				n = glm::vec3(1,0,0);
		} else if (abstan[1] <= abstan[0] && abstan[1] <= abstan[2]) {
				n = glm::vec3(0,1,0);
		} else {
				n = glm::vec3(0,0,1);
		}


		glm::vec3 normal = glm::normalize(glm::cross(tangent, n));
		glm::vec3 bitan = glm::normalize(glm::cross(tangent,normal));
		glm::mat4 tolocal = glm::inverse(glm::mat4(glm::vec4(bitan,0),glm::vec4(normal,0),glm::vec4(tangent,0),glm::vec4(j.position,1)));
		double time;
		glm::mat4 inverse = glm::inverse(j.d);
		double cyl_len = glm::distance(j.position,parentpos);

		if (intersectLocal(glm::dvec3(tolocal*glm::vec4(eye_,1)), glm::dvec3(tolocal*dir),time,cyl_len)){
			
			if (min_bone == -1 || time < min_time ) {
				min_time = time;
				min_bone = bone;
			}
		}
	}

	current_bone_ = min_bone;
}

void GUI::mouseButtonCallback(int button, int action, int mods)
{	
	if(action == GLFW_RELEASE){	
		move_scrub = false;
	}
	if (current_x_ <= view_width_) {
		//cout << "current x" << endl;
		drag_state_ = (action == GLFW_PRESS);
		current_button_ = button;
		return ;
	}
	// FIXME: Key Frame Selection
	if(current_x_ <= window_width_ && current_y_ <= window_height_){
		if(action == GLFW_PRESS){
			int temp = floor((window_height_-current_y_)/preview_height_-0.5*scroll_offset);
			if (temp < getNumKeyframes()) {
				selected_frame = temp;
			}
		}
	}
	

	
}

void GUI::mouseScrollCallback(double dx, double dy)
{
	if (current_x_ < view_width_)
		return;
	// FIXME: Mouse Scrolling
	if(scroll_offset >= 0 && dy > 0)
		return;
	//limit downward scrolling
	// if(scroll_offset <=  -(int) mesh_->skeleton.keyframes.size()   && dy < 0)
	// 	return;
	scroll_offset += 0.5 * dy;
}

void GUI::updateMatrices()
{
	// Compute our view, and projection matrices.
	if (fps_mode_)
		center_ = eye_ + camera_distance_ * look_;
	else
		eye_ = center_ - camera_distance_ * look_;
	view_matrix_ = glm::lookAt(eye_, center_, up_);


	aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_;
	ret.model= &model_matrix_;
	ret.view = &view_matrix_;
	return ret;
}

bool GUI::setCurrentBone(int i)
{
	if (i < 0 || i >= mesh_->getNumberOfBones())
		return false;
	current_bone_ = i;
	return true;
}

float GUI::getCurrentPlayTime() const
{
	// FIXME: return actual time???
	// save start time when p is pressed
	// reset start time when r is pressed
	// save amount of time run when p (pause) pressed and add when 
	// it resumes

	chrono::time_point<chrono::steady_clock> current = chrono::steady_clock::now();
	chrono::duration<float> elapsed_time = current - play_start;
	return elapsed_time.count() + pause_time;
}


bool GUI::captureWASDUPDOWN(int key, int action)
{
	if (key == GLFW_KEY_W) {
		if (fps_mode_){
			eye_ += zoom_speed_ * look_;
			//rel_pos += zoom_speed_ * look_;
		}
		else{
			camera_distance_ -= zoom_speed_;
		}
		//rel_pos += zoom_speed_ * look_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_){
			eye_ -= zoom_speed_ * look_;
		}
		else{
			camera_distance_ += zoom_speed_;
		}
		//rel_pos -= zoom_speed_ * look_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_)
			eye_ -= pan_speed_ * tangent_;
		else
			center_ -= pan_speed_ * tangent_;
		//rel_pos -= pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_)
			eye_ += pan_speed_ * tangent_;
		else
			center_ += pan_speed_ * tangent_;
		//rel_pos += pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			eye_ -= pan_speed_ * up_;
		else
			center_ -= pan_speed_ * up_;
		//rel_pos -= pan_speed_ * up_;
		return true;
	} else if (key == GLFW_KEY_UP) {
		if (fps_mode_)
			eye_ += pan_speed_ * up_;
		else
			center_ += pan_speed_ * up_;
		//rel_pos += pan_speed_ * up_;
		return true;
	}
	return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}

void GUI::MouseScrollCallback(GLFWwindow* window, double dx, double dy)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseScrollCallback(dx, dy);
}
