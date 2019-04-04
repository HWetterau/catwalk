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
	for (int i = 0; i < weights.size(); ++i) {
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

void Mesh::updateAnimation(float t, AnimationState* a)
{	// FIXME: Support Animation Here
	if (t != -1) {
		a->current_time = t;
		float fps = 2.0;
		//cout << "curr - old " << a->current_time - a->old_time  << endl;
		if (a->current_time - a->old_time > (1/fps)) {
			a->old_time = a->current_time;
			// cout << "current keyframe " << (int)a->current_keyframe << endl;
			// cout << "next keyframe " << (int)a->next_keyframe << endl;
 			if (a->next_keyframe < a->end_keyframe) {
				a->current_keyframe = a->next_keyframe;
				a->next_keyframe++;

				// cout <<"new current keyframe " << (int)a->current_keyframe << endl;
				// cout << "new next keyframe " << (int)a->next_keyframe << endl;
			} else {
				a->current_keyframe = a->next_keyframe;
			}

		}
		float interp = fps * (a->current_time - a->old_time);
		KeyFrame result;
 		KeyFrame::interpolate(skeleton.keyframes[a->current_keyframe], skeleton.keyframes[a->next_keyframe], interp, result);
		changeSkeleton(result);
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
