/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <string>

using namespace tinyxml2;

namespace dae2pbrt
{
	namespace Utils
	{
		static void ConvertStringToArray(const char* str, std::vector<int>& out_vector);
		static void ConvertStringToArray(const char* str, std::vector<float>& out_vector);
		static XMLElement* FindNodeById(XMLNode* node_parent, const char* node_name, const char* node_id);
	};

	struct vec2
	{
		float x, y;
	};

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

		bool CreateFromXML(XMLNode* node_mesh);
		bool ExtractVertices(XMLNode* node_poly, XMLNode* node_mesh);
		bool ExtractSourceFloatArray(XMLNode* node_mesh, const char* source_id, const int stride, std::vector<float>& dst_array);

		std::string name;
		std::string material_name;

		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texcoords;

		std::vector<int> polycounts;
		std::vector<int> polys;
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

};