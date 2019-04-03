#include "bone_geometry.h"
#include "texture_to_render.h"
#include <fstream>
#include <iostream>
#include <glm/gtx/io.hpp>
#include <unordered_map>
/*
 * We put these functions to a separated file because the following jumbo
 * header consists of 20K LOC, and it is very slow to compile.
 */
#include "json.hpp"

using json = nlohmann::json;
namespace {
	const glm::fquat identity(1.0, 0.0, 0.0, 0.0);
}

void Mesh::saveAnimationTo(const std::string& fn)
{
	// FIXME: Save keyframes to json file.
}

void Mesh::loadAnimationFrom(const std::string& fn)
{
	// FIXME: Load keyframes from json file.
}
