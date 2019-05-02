#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <mmdadapter.h>
#include <iostream>
#include <chrono>

using namespace std;

class TextureToRender;

struct BoundingBox {
	BoundingBox()
		: min(glm::vec3(-std::numeric_limits<float>::max())),
		max(glm::vec3(std::numeric_limits<float>::max())) {}
	glm::vec3 min;
	glm::vec3 max;
};

struct Joint {
	Joint()
		: joint_index(-1),
		  parent_index(-1),
		  position(glm::vec3(0.0f)),
		  init_position(glm::vec3(0.0f))
	{
	}
	Joint(int id, glm::vec3 wcoord, int parent)
		: joint_index(id),
		  parent_index(parent),
		  position(wcoord),
		  init_position(wcoord),
		  init_rel_position(init_position)
	{
	}

	int joint_index;
	int parent_index;
	glm::vec3 position;             // position of the joint
	glm::fquat orientation;         // rotation w.r.t. initial configuration
	glm::fquat rel_orientation;     // rotation w.r.t. it's parent. Used for animation.
	glm::vec3 init_position;        // initial position of this joint
	glm::vec3 init_rel_position;    // initial relative position to its parent
	//god help
	glm::mat4 t;
	glm::mat4 b;
	glm::mat4 d;
	std::vector<int> children;
};

struct Configuration {
	std::vector<glm::vec3> trans;
	std::vector<glm::fquat> rot;

	const auto& transData() const { return trans; }
	const auto& rotData() const { return rot; }
};


struct KeyFrame {
	// coming from the skeleton, from jointRot()
	std::vector<glm::fquat> rel_rot;
	float time;

	
	static bool quatEquals(glm::fquat a, glm::fquat b){
		//return (a[0]==b[0] && a[1]==b[1] && a[2] == b[2] && a[3]==b[3]);
		return abs(glm::dot(a,b)) > 1- 0.0001;
	}
	static glm::fquat boneSquad(glm::fquat prev, glm::fquat cur, glm::fquat next, float tau){
		//glm::fquat a = myBisect(doub(prev,cur),next);
		//glm::fquat b = doub(a,cur);
		if( quatEquals(prev,next) || quatEquals(prev,cur)){
			return prev;
		}

		 glm::fquat a = glm::intermediate(prev,cur,next);
		 glm::fquat b = glm::intermediate(a,cur,next);
		return glm::squad(prev,cur,a,b, tau);

}
	static void interpolate(const KeyFrame& from,
	                        const KeyFrame& to, const KeyFrame& next,
	                        float tau,
	                        KeyFrame& target) {
		for(int i = 0; i < (int)from.rel_rot.size(); ++i){
			//CHANGE ME
			//target.rel_rot.push_back(glm::slerp(from.rel_rot[i], to.rel_rot[i], tau));
			glm::fquat cur = glm::slerp(from.rel_rot[i], to.rel_rot[i], tau);
			target.rel_rot.push_back(boneSquad(from.rel_rot[i],cur,to.rel_rot[i], tau));
		}
	
		//target.camera_rot = glm::slerp(from.camera_rot, to.camera_rot, tau);
		

	}
};

// struct LightCam {
// 	glm::vec4 light_pos;
// 	glm::vec3 camera_pos;
// 	//if rotations dont work out use center and glm look at
// 	glm::mat3 camera_rot;
// 	glm::vec4 light_color;
// 		float camera_dist;

// };


struct AnimationState {
	int current_keyframe;
	int next_keyframe;
	int prev_keyframe;

	int end_keyframe;

	float current_time;
	float old_time;

	AnimationState(): current_keyframe(0), next_keyframe(1), prev_keyframe(0) {}

	//could store interpolation factor but also could calculate
	
};

struct LineMesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::uvec2> indices;
};

struct Skeleton {
	std::vector<Joint> joints;
	vector<KeyFrame> keyframes;

	Configuration cache;

	void refreshCache(Configuration* cache = nullptr);
	const glm::vec3* collectJointTrans() const;
	const glm::fquat* collectJointRot() const;

	glm::vec3* jointTrans();
	glm::fquat* jointRot();


	// FIXME: create skeleton and bone data structures

	void add_joint(Joint& j){
			j.t = glm::mat4(1.0f);
			if(j.joint_index > 0){

				glm::vec3 temp = j.init_position - joints[j.parent_index].init_position;
				j.init_rel_position = temp;
				j.b = glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,temp[0],temp[1],temp[2],1);
				j.d = joints[j.parent_index].d * j.b * j.t;
			} else {
					glm::vec3 temp = j.init_position;
					j.b = glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,temp[0],temp[1],temp[2],1);
					j.d = j.b * j.t;
			}
			joints.push_back(j);
			// add current joint to parent's child list
			if(j.parent_index >= 0){
				joints[j.parent_index].children.push_back(j.joint_index);
			}
	}
	void rotate(int index, int child, glm::mat4 R){
		Joint& j = joints[index];
		j.t = j.t * R;
		if(index > 0){
			j.d = joints[j.parent_index].d * j.b * j.t;
		} else {
			j.d = j.b * j.t;
		}
	//	for(int child: j.children){
			update_d(child);
		//}

	}
	void translate(glm::mat4 T){
		Joint& j = joints[0];
		j.t = j.t * T;
		update_d(0);
	}
	
	void update_d(int index){
		if(index < 0){
			//cout<<"index < 0"<<endl;
			return;
		}
		Joint& j = joints[index];
		if(index > 0){
			j.d = joints[j.parent_index].d * j.b * j.t;
		} else {
			j.d = j.b * j.t;
		}
	//	j.position = glm::vec3(joints[j.parent_index].d * glm::vec4(j.init_rel_position,1));
		j.position = glm::vec3(j.d * glm::vec4(0,0,0,1));
		for(int child: j.children){
			update_d(child);
		}
	}


};

struct Mesh {
	Mesh();
	~Mesh();
	std::vector<glm::vec4> vertices;
	/*
	 * Static per-vertex attrributes for Shaders
	 */
	std::vector<int32_t> joint0;
	std::vector<int32_t> joint1;
	std::vector<float> weight_for_joint0; // weight_for_joint1 can be calculated
	std::vector<glm::vec3> vector_from_joint0;
	std::vector<glm::vec3> vector_from_joint1;
	std::vector<glm::vec4> vertex_normals;
	std::vector<glm::vec4> face_normals;
	std::vector<glm::vec2> uv_coordinates;
	std::vector<glm::uvec3> faces;

	std::vector<Material> materials;
	BoundingBox bounds;
	Skeleton skeleton;

	void loadPmd(const std::string& fn);
	int getNumberOfBones() const;
	glm::vec3 getCenter() const { return 0.5f * glm::vec3(bounds.min + bounds.max); }
	const Configuration* getCurrentQ() const; // Configuration is abbreviated as Q
	void updateAnimation(float t,  AnimationState* a);
	void updateAnimation();

	void saveAnimationTo(const std::string& fn);
	void loadAnimationFrom(const std::string& fn);
	glm::vec3 lightSpline(float t);
	glm::vec3 cameraPosSpline(float t);
	glm::mat3 cameraRotSpline(float t);

	vector<glm::mat4> load_d_u() {
		vector<glm::mat4> u;
		u.push_back(skeleton.joints[0].d * glm::inverse(skeleton.joints[0].b));

		for (int bone = 1; bone < getNumberOfBones(); ++bone) {
			//u.push_back(glm::inverse(skeleton.joints[bone-1].d * skeleton.joints[bone].b));
			// u.push_back(skeleton.joints[bone].d*glm::inverse(skeleton.joints[bone-1].d * skeleton.joints[bone].b));
			u.push_back(skeleton.joints[bone].d*glm::inverse(glm::mat4(glm::vec4(1,0,0,0),glm::vec4(0,1,0,0),
										glm::vec4(0,0,1,0),glm::vec4(skeleton.joints[bone].init_position,1))));
		}

		return u;
	}

	vector<glm::mat2x4> load_dq() {
		vector<glm::mat2x4> dq;
		// const glm::vec3* collectJointTrans() const;
		// const glm::fquat* collectJointRot() const;

		for (int bone = 0; bone < getNumberOfBones(); ++bone) {
			//u.push_back(glm::inverse(skeleton.joints[bone-1].d * skeleton.joints[bone].b));
			// u.push_back(skeleton.joints[bone].d*glm::inverse(skeleton.joints[bone-1].d * skeleton.joints[bone].b));
			glm::vec3 trans = skeleton.joints[bone].position;
			glm::fquat rot = glm::quat_cast(glm::mat3(skeleton.joints[bone].d));

			glm::mat2x4 dq_mat;
			for (int i = 0; i < 4; i++) {
				dq_mat[0][i] = rot[i];
			}
			
			dq_mat[1][0] = -0.5*(trans[0]*rot[1] + trans[1]*rot[2] + trans[2]*rot[3]);
			dq_mat[1][1] = 0.5*( trans[0]*rot[0] + trans[1]*rot[3] - trans[2]*rot[2]);
			dq_mat[1][2] = 0.5*(-trans[0]*rot[3] + trans[1]*rot[0] + trans[2]*rot[1]);
			dq_mat[1][3] = 0.5*( trans[0]*rot[2] - trans[1]*rot[1] + trans[2]*rot[0]);


			dq.push_back(dq_mat);
		}

		return dq;
}
	//ADDED THIS, in .cc file
	void changeSkeleton(KeyFrame& k);

	// vector<glm::mat4> load_d() {
	// 	vector<glm::mat4> d;

	// 	for (int bone = 0; bone < getNumberOfBones(); ++bone) {
	// 		d.push_back(skeleton.joints[bone].d);
	// 	}
	// 	return d;
	// }

private:
	void computeBounds();
	void computeNormals();
	Configuration currentQ_;
};


#endif
