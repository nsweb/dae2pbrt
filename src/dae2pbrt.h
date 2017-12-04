/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>

using namespace tinyxml2;

class Mesh
{
public:
	Mesh();
	~Mesh();

};

class Program
{
public:
	Program();
	~Program();

	void ExtractMeshes(XMLNode* node_lib);

	std::vector<Mesh*> meshes;
};