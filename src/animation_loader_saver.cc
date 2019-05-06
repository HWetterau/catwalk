#include "gui.h"
#include "texture_to_render.h"
#include <fstream>
#include <iostream>
#include <glm/gtx/io.hpp>
#include <unordered_map>
#include <string>
#include <glm/gtx/string_cast.hpp>
/*
 * We put these functions to a separated file because the following jumbo
 * header consists of 20K LOC, and it is very slow to compile.
 */
#include "json.hpp"



using json = nlohmann::json;
using namespace std;
namespace {
	const glm::fquat identity(1.0, 0.0, 0.0, 0.0);
}


void GUI::saveAnimationTo(const std::string& fn)
{
	// FIXME: Save keyframes to json file.
	json js; 
	js["model_size"] = mesh_->skeleton.keyframes.size();
	js["bones"] = mesh_->skeleton.joints.size();
	for(int i = 0; i <(int) mesh_->skeleton.keyframes.size(); ++i) {
		KeyFrame current_keyframe = mesh_->skeleton.keyframes[i];
		js["keyframe" + to_string(i)] = json::array();
		for (int j = 0; j < (int) current_keyframe.rel_rot.size(); ++j) {
			glm::fquat current_quat = current_keyframe.rel_rot[j];
			js["keyframe" + to_string(i)] += {current_quat[0], current_quat[1], current_quat[2], current_quat[3]};
			
		}
		js["model_time" + to_string(i)] = current_keyframe.time;	
	}
	js["light_size"] = lightKeyframes.size();
	for (int i = 0; i<(int)lightKeyframes.size(); ++i){
		js["light_pos" + to_string(i)] = {lightKeyframes[i].light_pos[0], lightKeyframes[i].light_pos[1], lightKeyframes[i].light_pos[2], lightKeyframes[i].light_pos[3]};
		js["light_color" + to_string(i)] = {lightKeyframes[i].light_color[0], lightKeyframes[i].light_color[1], lightKeyframes[i].light_color[2], lightKeyframes[i].light_color[3]};
		js["light_time"+ to_string(i)] = lightKeyframes[i].time;
	}
	js["camera_size"]= cameraKeyframes.size();
	for (int i = 0; i<(int)cameraKeyframes.size(); ++i){
		js["camera_pos" + to_string(i)] = {cameraKeyframes[i].camera_pos[0], cameraKeyframes[i].camera_pos[1], cameraKeyframes[i].camera_pos[2]};
		js["camera_rot" + to_string(i)] = {cameraKeyframes[i].camera_rot[0][0], cameraKeyframes[i].camera_rot[0][1], cameraKeyframes[i].camera_rot[0][2],
										  cameraKeyframes[i].camera_rot[1][0], cameraKeyframes[i].camera_rot[1][1], cameraKeyframes[i].camera_rot[1][2],
										  cameraKeyframes[i].camera_rot[2][0], cameraKeyframes[i].camera_rot[2][1], cameraKeyframes[i].camera_rot[2][2]
										 };
		js["camera_time"+ to_string(i)] = cameraKeyframes[i].time;
	}
	
		
		
	ofstream file(fn);
	file << js;
}

void GUI::loadAnimationFrom(const std::string& fn)
{
	// FIXME: Load keyframes from json file.
	ifstream ifs(fn);
	json j = json::parse(ifs);
	int num_keyframes = j["model_size"];
	int num_bones = j["bones"];
	int num_light_keyframes = j["light_size"];
	int num_camera_keyframes = j["camera_size"];

	for (int i = 0; i < num_keyframes; ++i) {
		KeyFrame k;
		
		for (int bone = 0; bone < num_bones; ++bone){
			json arr = j["keyframe"+to_string(i)][bone];
			glm::fquat quat;
			quat[0] = arr[0];
			quat[1] = arr[1];
			quat[2] = arr[2];
			quat[3] = arr[3];
			k.rel_rot.push_back(quat);
		}
		k.time = j["model_time"+to_string(i)];
		mesh_->skeleton.keyframes.push_back(k);

	}
	for (int i = 0; i< num_light_keyframes; ++i){
		LightKeyFrame lk;
		json light = j["light_pos"+to_string(i)];
		lk.light_pos = glm::vec4(light[0], light[1], light[2], light[3]);
		json color = j["light_color"+to_string(i)];
		lk.light_color = glm::vec4(color[0], color[1], color[2], color[3]);
		lk.time = j["light_time" +to_string(i)];
		lightKeyframes.push_back(lk);
	}
	for (int i = 0; i< num_camera_keyframes; ++i){
		CameraKeyFrame k;
		json camera = j["camera_pos"+to_string(i)];
		k.camera_pos = glm::vec3(camera[0], camera[1], camera[2]);
		json rot = j["camera_rot"+to_string(i)];
		k.camera_rot = glm::mat3(rot[0], rot[1], rot[2], rot[3], rot[4], rot[5], rot[6], rot[7], rot[8]);
		k.time = j["camera_time" + to_string(i)];
		cameraKeyframes.push_back(k);
	}

	sceneState->end_light_keyframe = lightKeyframes.size() - 1;
	sceneState->end_camera_keyframe = cameraKeyframes.size() - 1;
}
