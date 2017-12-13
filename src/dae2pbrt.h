/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>

using namespace tinyxml2;

namespace dae2pbrt
{
	namespace Utils
	{
		static void ConvertStringToArray(const char* str, std::vector<int>& out_vector);
		static void ConvertStringToArray(const char* str, std::vector<float>& out_vector);
		static XMLElement* FindNodeById(XMLNode* node_parent, const char* node_name, const char* node_id);
        static void ExtractFilePath(const std::string& fullname, std::string& out_path);
        static void ExtractFilePath(const std::string& fullname, std::string& out_path, std::string& out_filename, std::string& out_extension);
	};

	class Material
	{
	public:
		Material();
		~Material();

        void Reset();
        bool ImportFromXML(XMLNode* node);
        
		std::string name;
		std::string fx_name;

        std::vector<float> diffuse;
        std::vector<float> specular;
        float shininess;
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

        int position_stride;
        int normal_stride;
        int texcoord_stride;
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> texcoords;

		std::vector<int> polycounts;
		std::vector<int> polys;
	};
    
    class MeshInstance
    {
    public:
        MeshInstance();
        ~MeshInstance();
        
        void Reset();
        bool ImportFromXML(XMLNode* node_sub);

		std::string instance_name;
        std::string mesh_name;
        std::string material_name;

        std::vector<float> matrix;
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

        void ImportMaterials(XMLNode* node_lib_mat, XMLNode* node_lib_effect);
		void ImportMeshes(XMLNode* node_lib);
        void ImportNodes(XMLNode* node_lib);
		void ImportVisualScene(XMLNode* node_lib);
        void ExportPlyMeshes() const;
        void ExportPbrtScene() const;
		bool DoesMeshExist(MeshInstance* instance) const;

        Options options;
        std::map<std::string, Material*> materials;
        std::map<std::string, Mesh*> meshes;
        std::vector<MeshInstance*> mesh_instances;
	};

};
