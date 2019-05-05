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



void Mesh::updateAnimation(float t, AnimationState* a)
{	// FIXME: Support Animation Here
	if(a == nullptr)
		return;
	if (t != -1 && skeleton.keyframes.size() > 0) {

		a->current_time = t;
		//float fps = 1.0;
		bool interpolate = true;
		KeyFrame result;
		if(a->current_time > a->old_time){
			//forwards
			float length = skeleton.keyframes[a->next_keyframe].time - skeleton.keyframes[a->current_keyframe].time;
			
			if (a->current_keyframe == a->end_keyframe){
		
					interpolate = false;

			} else if (a->current_time - a->old_time > length) {

				a->old_time = a->current_time;

				if (a->next_keyframe < a->end_keyframe) {
					a->prev_keyframe = a->current_keyframe;
					a->current_keyframe = a->next_keyframe;
					a->next_keyframe++;
					interpolate = true;

				} else {
					a->prev_keyframe = a->current_keyframe;
					a->current_keyframe = a->next_keyframe;
				}
			}
			
			float interp =  (a->current_time - a->old_time)/length;
			
			if (interpolate){
				int cp1 = glm::clamp<int>(a->current_keyframe, 0, skeleton.keyframes.size() - 1);
				int cp2 = glm::clamp<int>(a->next_keyframe, 0, skeleton.keyframes.size() - 1);
				int cp3 = glm::clamp<int>(a->next_keyframe +1, 0, skeleton.keyframes.size() - 1);

				KeyFrame::interpolate(skeleton.keyframes[cp1], skeleton.keyframes[cp2], skeleton.keyframes[cp3], interp, result);

			} else {
				result = skeleton.keyframes[a->current_keyframe];
			}
		} else {
			//backwards wooo
			float length = skeleton.keyframes[a->current_keyframe].time - skeleton.keyframes[a->prev_keyframe].time;
			if (a->current_keyframe == 0){
		
					interpolate = false;
			} else if (a->old_time - a->current_time > length && a->current_time < skeleton.keyframes[a->current_keyframe].time ) {
			

				a->old_time = a->current_time;

				if (a->prev_keyframe > 0) {
					a->next_keyframe = a->current_keyframe;
					a->current_keyframe = a->prev_keyframe;
					a->prev_keyframe--;
					interpolate = true;

				} else {
					a->next_keyframe = a->current_keyframe;
					a->current_keyframe = a->prev_keyframe;
				}
			}
			float interp = (a->old_time - a->current_time)/length;
			
			if (interpolate){
				int cp1 = glm::clamp<int>(a->current_keyframe, 0, skeleton.keyframes.size() - 1);
				int cp2 = glm::clamp<int>(a->prev_keyframe, 0, skeleton.keyframes.size() - 1);
				int cp3 = glm::clamp<int>(a->prev_keyframe-1, 0, skeleton.keyframes.size() - 1);

				KeyFrame::interpolate(skeleton.keyframes[cp1], skeleton.keyframes[cp2], skeleton.keyframes[cp3], interp, result);

			} else {
				result = skeleton.keyframes[a->current_keyframe];
			}

		}
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
