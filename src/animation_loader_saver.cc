#include "bone_geometry.h"
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


void Mesh::saveAnimationTo(const std::string& fn)
{
	// FIXME: Save keyframes to json file.
	json js; 
	js["size"] = skeleton.keyframes.size();
	js["bones"] = skeleton.joints.size();
	for(int i = 0; i <(int) skeleton.keyframes.size(); ++i) {
		KeyFrame current_keyframe = skeleton.keyframes[i];
		js["keyframe" + to_string(i)] = json::array();
		for (int j = 0; j < (int) current_keyframe.rel_rot.size(); ++j) {
			glm::fquat current_quat = current_keyframe.rel_rot[j];
			js["keyframe" + to_string(i)] += {current_quat[0], current_quat[1], current_quat[2], current_quat[3]};

		}

	}
	ofstream file(fn);
	file << js;
}

void Mesh::loadAnimationFrom(const std::string& fn)
{
	// FIXME: Load keyframes from json file.
	ifstream ifs(fn);
	json j = json::parse(ifs);
	int num_keyframes = j["size"];
	int num_bones = j["bones"];

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
		skeleton.keyframes.push_back(k);

	}
}
