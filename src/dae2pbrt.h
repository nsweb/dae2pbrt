/*
*/

#include "tinyxml2/tinyxml2.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <fstream>

using namespace std;
using namespace tinyxml2;

namespace dae2pbrt
{
	namespace Utils
	{
		static void ConvertStringToArray(const char* str, vector<int>& out_vector);
		static void ConvertStringToArray(const char* str, vector<float>& out_vector);
		static XMLElement* FindNodeById(XMLNode* node_parent, const char* node_name, const char* node_id);
        static void ExtractFilePath(const string& fullname, string& out_path);
        static void ExtractFilePath(const string& fullname, string& out_path, string& out_filename, string& out_extension);
	};

	class Material
	{
	public:
		Material();
		~Material();

        void Reset();
        bool ImportFromXML(XMLNode* node);
        
		string name;
		string fx_name;

        vector<float> diffuse;
        vector<float> specular;
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
		bool ExtractSourceFloatArray(XMLNode* node_mesh, const char* source_id, const int stride, vector<float>& dst_array);
        void ExportToPly(ofstream& stream) const;
        void Repair();

		string name;

        int position_stride;
        int normal_stride;
        int texcoord_stride;
		vector<float> positions;
		vector<float> normals;
		vector<float> texcoords;

		vector<int> polycounts;
		vector<int> polys;
	};
    
    class SceneNode
    {
    public:
		SceneNode();
        ~SceneNode();
        
        void Reset();
        bool ImportFromXML(XMLNode* node_sub);

        bool visual_node;
		string node_name;
        string mesh_name;
        string material_name;
        vector<float> matrix;

		vector<string> child_nodes;
    };

	struct Options
	{
		string filename;
	};

	class Program
	{
	public:
		Program();
		~Program();

        void ImportMaterials(XMLNode* node_lib_mat, XMLNode* node_lib_effect);
		void ImportMeshes(XMLNode* node_lib);
		void ImportSceneNodes(XMLNode* node_root);
        void ImportSubNodes(XMLNode* node_parent, bool visual_node = false, SceneNode* parent = nullptr);
		//void ImportVisualScene(XMLNode* node_lib);
        void ExportPlyMeshes() const;
        void ExportPbrtScene() const;
        void ExportPbrtNode(ofstream& stream, SceneNode* scene_node, int indent_level) const;
		bool DoesMeshExist(SceneNode* scene_node) const;

        Options options;
        map<string, shared_ptr<Material> > materials;
        map<string, shared_ptr<Mesh> > meshes;
		map<string, shared_ptr<SceneNode> > scene_nodes;
	};

};
