/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <string>
#include <fstream>

using namespace tinyxml2;

namespace dae2pbrt
{
	namespace Utils
	{
		static void ConvertStringToArray(const char* str, std::vector<int>& out_vector);
		static void ConvertStringToArray(const char* str, std::vector<float>& out_vector);
		static XMLElement* FindNodeById(XMLNode* node_parent, const char* node_name, const char* node_id);
        static void GetFilePath(const std::string& filename, std::string& out_path);
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

        void Reset();
		bool ImportFromXML(XMLNode* node_mesh);
		bool ImportVertices(XMLNode* node_poly, XMLNode* node_mesh);
		bool ExtractSourceFloatArray(XMLNode* node_mesh, const char* source_id, const int stride, std::vector<float>& dst_array);
        void ExportToPly(std::ofstream& stream) const;
        void Repair();

		std::string name;
		std::string material_name;

        int position_stride;
        int normal_stride;
        int texcoord_stride;
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

		void ImportMeshes(XMLNode* node_lib);
        void ExportPlyMeshes() const;

        Options options;
		std::vector<Mesh*> meshes;
	};

};
