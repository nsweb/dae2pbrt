/*
 MIT License
 
 This file is part of dae2pbrt
 Copyright (c) 2017-2018 Nicolas Sérouart
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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

	struct EPbrtMaterial
	{
		enum Type
		{
			Matte = 0,
			Plastic,
			Disney,
			Max,
		};
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
		bool ImportFromXML(XMLNode* node_mesh, bool skip_vertices);
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
		/** Name of the DAE to import */
		string filename = "";
		/** Default material used when exporting the pbrt file */
		string default_material = "matte";
		/** Default material converted to enum */
		EPbrtMaterial::Type material_type = (EPbrtMaterial::Type)0;
		/** Whether to import/export mesh geometry */
		bool skip_mesh = false;
		
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
