#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include <ostream>
#include <vector>
#include <string>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <mmdadapter.h>
#include <iostream>

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
	std::vector<glm::fquat> rel_rot;

	static void interpolate(const KeyFrame& from,
	                        const KeyFrame& to,
	                        float tau,
	                        KeyFrame& target);
};

struct LineMesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::uvec2> indices;
};

struct Skeleton {
	std::vector<Joint> joints;

	Configuration cache;

	void refreshCache(Configuration* cache = nullptr);
	const glm::vec3* collectJointTrans() const;
	const glm::fquat* collectJointRot() const;

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
	void updateAnimation(float t = -1.0);

	void saveAnimationTo(const std::string& fn);
	void loadAnimationFrom(const std::string& fn);

	vector<glm::mat4> load_u() {
		vector<glm::mat4> u;
		u.push_back(skeleton.joints[0].b);

		for (int bone = 1; bone < getNumberOfBones(); ++bone) {
			u.push_back(skeleton.joints[bone-1].d * skeleton.joints[bone].b);
		}
		return u;
	}

	vector<glm::mat4> load_d() {
		vector<glm::mat4> d;

		for (int bone = 0; bone < getNumberOfBones(); ++bone) {
			d.push_back(skeleton.joints[bone].d);
		}
		return d;
	}

private:
	void computeBounds();
	void computeNormals();
	Configuration currentQ_;
};


#endif
