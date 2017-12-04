/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <string>

using namespace tinyxml2;

struct vec3
{
    float x, y, z;
};

class Material
{
public:
    Material();
    ~Material();
    
    std::string name;
};

class Mesh
{
public:
	Mesh();
	~Mesh();

    std::string name;
    std::string material_name;
    
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<int> triangles;
};

struct Options
{
    std::string filename;
};

class Program
{
public:
	Program();
	~Program();

	void ExtractMeshes(XMLNode* node_lib);

	std::vector<Mesh*> meshes;
};
