#include "config.h"
#include "bone_geometry.h"
#include "texture_to_render.h"
#include <fstream>
#include <queue>
#include <iostream>
#include <stdexcept>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/string_cast.hpp>

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	size_t count = std::min(v.size(), static_cast<size_t>(10));
	for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
	os << "size = " << v.size() << "\n";
	return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds)
{
	os << "min = " << bounds.min << " max = " << bounds.max;
	return os;
}



const glm::vec3* Skeleton::collectJointTrans() const
{
	return cache.trans.data();
}

const glm::fquat* Skeleton::collectJointRot() const
{
	return cache.rot.data();
}

// FIXME: Implement bone animation.

void Skeleton::refreshCache(Configuration* target)
{
	if (target == nullptr)
		target = &cache;
	target->rot.resize(joints.size());
	target->trans.resize(joints.size());
	for (size_t i = 0; i < joints.size(); i++) {
		//target->rot[i] = joints[i].orientation;
		target->rot[i] = glm::quat_cast(glm::mat3(joints[i].d));
		target->trans[i] = joints[i].position;

	}
}


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::loadPmd(const std::string& fn)
{
	MMDReader mr;
	mr.open(fn);
	mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
	computeBounds();
	mr.getMaterial(materials);

	// FIXME: load skeleton and blend weights from PMD file,
	//        initialize std::vectors for the vertex attributes,
	//        also initialize the skeleton as needed

	vector<SparseTuple> weights;
	mr.getJointWeights(weights);
	for (int i = 0; i < (int)weights.size(); ++i) {
		SparseTuple current = weights[i];
		joint0.push_back(current.jid0);
		joint1.push_back(current.jid1);
		weight_for_joint0.push_back(current.weight0);
	}

	int id = 0;
	glm::vec3 wcoord;
	int pid = -1;
	while(mr.getJoint(id, wcoord, pid)){
		Joint j(id, wcoord, pid);
		skeleton.add_joint(j);
		++id;
	}

}

int Mesh::getNumberOfBones() const
{
	return skeleton.joints.size();
}

void Mesh::computeBounds()
{
	bounds.min = glm::vec3(std::numeric_limits<float>::max());
	bounds.max = glm::vec3(-std::numeric_limits<float>::max());
	for (const auto& vert : vertices) {
		bounds.min = glm::min(glm::vec3(vert), bounds.min);
		bounds.max = glm::max(glm::vec3(vert), bounds.max);
	}
}
glm::vec3 Mesh::lightSpline(float t){
	int cp0 = glm::clamp<int>(t - 1, 0, skeleton.keyframes.size() - 1);
    int cp1 = glm::clamp<int>(t,     0, skeleton.keyframes.size() - 1);
    int cp2 = glm::clamp<int>(t + 1, 0, skeleton.keyframes.size() - 1);
    int cp3 = glm::clamp<int>(t + 2, 0, skeleton.keyframes.size() - 1);
	float local_t = glm::fract(t);
	return glm::catmullRom(skeleton.keyframes[cp0].light_pos, skeleton.keyframes[cp1].light_pos, skeleton.keyframes[cp2].light_pos, skeleton.keyframes[cp3].light_pos, local_t);
}

glm::vec3 Mesh::cameraPosSpline(float t){
	int cp0 = glm::clamp<int>(t - 1, 0, skeleton.keyframes.size() - 1);
    int cp1 = glm::clamp<int>(t,     0, skeleton.keyframes.size() - 1);
    int cp2 = glm::clamp<int>(t + 1, 0, skeleton.keyframes.size() - 1);
    int cp3 = glm::clamp<int>(t + 2, 0, skeleton.keyframes.size() - 1);
	float local_t = glm::fract(t);
	return glm::catmullRom(skeleton.keyframes[cp0].camera_pos, skeleton.keyframes[cp1].camera_pos, skeleton.keyframes[cp2].camera_pos, skeleton.keyframes[cp3].camera_pos, local_t);
}

glm::fquat Mesh::cameraRotSpline(float t){
	int cp0 = glm::clamp<int>(t - 1, 0, skeleton.keyframes.size() - 1);
    int cp1 = glm::clamp<int>(t,     0, skeleton.keyframes.size() - 1);
    int cp2 = glm::clamp<int>(t + 1, 0, skeleton.keyframes.size() - 1);
    int cp3 = glm::clamp<int>(t + 2, 0, skeleton.keyframes.size() - 1);
	float local_t = glm::fract(t);

	glm::mat3 m1 = glm::toMat3(skeleton.keyframes[cp0].camera_rot);
	glm::mat3 m2 = glm::toMat3(skeleton.keyframes[cp1].camera_rot);
	glm::mat3 m3 = glm::toMat3(skeleton.keyframes[cp2].camera_rot);
	glm::mat3 m4 = glm::toMat3(skeleton.keyframes[cp3].camera_rot);

	glm::mat3 result = glm::mat3(1.0);

	//cout << "m1 " << glm::to_string(m1) << endl;
	result[1] = glm::normalize(glm::catmullRom(glm::column(m1, 1), glm::column(m2, 1), glm::column(m3, 1), glm::column(m4, 1), local_t));
	glm::vec3 temp = glm::normalize(glm::catmullRom(glm::column(m1, 2), glm::column(m2, 2), glm::column(m3, 2), glm::column(m4, 2), local_t));

	temp-= glm::dot(temp, glm::column(result, 1))*glm::column(result, 1);
	temp = glm::normalize(temp);
	result[2] = temp;

	result[0] = glm::normalize(glm::cross(glm::column(result, 1), glm::column(result, 2)));

	//cout << "spline mat " << glm::to_string(result) << endl;

	return glm::quat_cast(result);
	//return glm::squad(skeleton.keyframes[cp0].camera_rot, skeleton.keyframes[cp3].camera_rot, skeleton.keyframes[cp1].camera_rot, skeleton.keyframes[cp2].camera_rot,  local_t);
}

void Mesh::updateAnimation(float t, AnimationState* a, LightCam& lc)
{	// FIXME: Support Animation Here
	if (t != -1) {
		a->current_time = t;
		float fps = 1.0;
		bool interpolate = true;
		//bool test = false;
		if (a->current_keyframe == a->end_keyframe){
		//	if(test)
				interpolate = false;
		//	test = true;
		} else if (a->current_time - a->old_time > (1/fps)) {

			a->old_time = a->current_time;

 			if (a->next_keyframe < a->end_keyframe) {
				a->current_keyframe = a->next_keyframe;
				a->next_keyframe++;
				interpolate = true;

			} else {
				a->current_keyframe = a->next_keyframe;

			}

		}
		float interp = fps * (a->current_time - a->old_time);
		KeyFrame result;
		if (interpolate){
 			KeyFrame::interpolate(skeleton.keyframes[a->current_keyframe], skeleton.keyframes[a->next_keyframe], interp, result);
			lc.camera_pos = cameraPosSpline(t);
			lc.light_pos = glm::vec4(lightSpline(t),1);
			lc.camera_rot = cameraRotSpline(t);
			//cout<<"light pos "<<glm::to_string(lc.light_pos)<<endl;
		} else {
			result = skeleton.keyframes[a->current_keyframe];
			lc.light_pos = result.light_pos;
			lc.camera_pos = result.camera_pos;
			lc.camera_rot = result.camera_rot;
		}
		changeSkeleton(result);
		//lc.light_pos = result.light_pos;
		//lc.camera_pos = result.camera_pos;

		//lc.camera_rot = result.camera_rot;
	//	lc.camera_rot = skeleton.keyframes[0].camera_rot;
		lc.camera_dist = result.camera_dist;
		lc.light_color = result.light_color;
	}

	skeleton.refreshCache(&currentQ_);

}

void Mesh::updateAnimation()
{
	skeleton.refreshCache(&currentQ_);
}

void Mesh::changeSkeleton(KeyFrame& k)
{
	skeleton.joints[0].t = glm::toMat4(k.rel_rot[0]);
	skeleton.joints[0].d = skeleton.joints[0].b * skeleton.joints[0].t;
	skeleton.joints[0].position = glm::vec3(skeleton.joints[0].d * glm::vec4(0,0,0,1));

	for (int i = 1; i < getNumberOfBones(); ++i) {
		skeleton.joints[i].t = glm::toMat4(k.rel_rot[i]);
		skeleton.joints[i].d = skeleton.joints[skeleton.joints[i].parent_index].d * skeleton.joints[i].b * skeleton.joints[i].t;
		skeleton.joints[i].position = glm::vec3(skeleton.joints[i].d * glm::vec4(0,0,0,1));
	}
}
const Configuration*
Mesh::getCurrentQ() const
{
	return &currentQ_;
}
