/*
*/

#include "dae2pbrt.h"
#include <iostream>

using namespace dae2pbrt;

void Usage(const char* error = nullptr)
{
    
}

// main program
int main(int argc, char *argv[])
{
	if (argc <= 1)
		return 0;
    
    Program program;
    
    // Process command-line arguments
    for (int i = 1; i < argc; ++i)
    {
        /*
         if (!strcmp(argv[i], "--outfile") || !strcmp(argv[i], "-outfile")) {
         if (i + 1 == argc)
         usage("missing value after --outfile argument");
         options.imageFile = argv[++i];
         } else if (!strncmp(argv[i], "--outfile=", 10)) {
         options.imageFile = &argv[i][10];
         } else if (!strcmp(argv[i], "--quick") || !strcmp(argv[i], "-quick")) {
         options.quickRender = true;
         } else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-quiet")) {
         options.quiet = true;
         } else if (!strcmp(argv[i], "--cat") || !strcmp(argv[i], "-cat")) {
         options.cat = true;
         } else if (!strcmp(argv[i], "--toply") || !strcmp(argv[i], "-toply")) {
         options.toPly = true;
         } else if (!strcmp(argv[i], "--v") || !strcmp(argv[i], "-v")) {
         if (i + 1 == argc)
         usage("missing value after --v argument");
         FLAGS_v = atoi(argv[++i]);
         } else if (!strncmp(argv[i], "--v=", 4)) {
         FLAGS_v = atoi(argv[i] + 4);
         }
         else if (!strcmp(argv[i], "--logtostderr")) {
         FLAGS_logtostderr = true;
         } else*/
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help") ||
            !strcmp(argv[i], "-h")) {
            Usage();
            return 0;
        } else
            program.options.filename = argv[i];
    }

    XMLDocument doc;
	XMLError result = doc.LoadFile( program.options.filename.c_str() );
	if (result != XML_SUCCESS)
		return 0;

	XMLNode* node_root = doc.FirstChildElement("COLLADA");
	if (!node_root)
	{
		// no node to process
		return 0;
	}

	XMLNode* node_lib_geom = node_root->FirstChildElement("library_geometries");
	if (node_lib_geom)
		program.ImportMeshes(node_lib_geom);

    XMLNode* node_lib_nodes = node_root->FirstChildElement("library_nodes");
	if (node_lib_nodes)
		program.ImportNodes(node_lib_nodes);
	else
	{
		XMLNode* node_lib_visual_scenes = node_root->FirstChildElement("library_visual_scenes");
		if (node_lib_visual_scenes)
			program.ImportVisualScene(node_lib_visual_scenes);
	}
    
    program.ExportPlyMeshes();
    program.ExportPbrtScene();
        
    return 0;
}


////////////////////////////////////////////////////////////////////////
Mesh::Mesh()
{
    Reset();
}
Mesh::~Mesh()
{
	
}

void Mesh::Reset()
{
    position_stride = 0;
    normal_stride = 0;
    texcoord_stride = 0;
    positions.clear();
    normals.clear();
    texcoords.clear();
    polycounts.clear();
    polys.clear();
}

bool Mesh::ImportFromXML(XMLNode* node_mesh)
{
    Reset();
    
	// extract source, vertices and polylist
	XMLElement* node_tri = node_mesh->FirstChildElement("triangles");
	if (node_tri)
	{
		int tri_count = node_tri->IntAttribute("count", 0);
		XMLElement* node_tri_idx = node_tri->FirstChildElement("p");
		if (node_tri_idx)
		{
			const char* idx_array = node_tri_idx->GetText();
			Utils::ConvertStringToArray(idx_array, polys);

			ImportVertices(node_mesh, node_tri);
		}
	}
	else
	{
		XMLElement* node_poly = node_mesh->FirstChildElement("polylist");
		if (node_poly)
		{
			int poly_count = node_poly->IntAttribute("count", 0);
			XMLElement* node_poly_vcount = node_poly->FirstChildElement("vcount");
			XMLElement* node_poly_idx = node_poly->FirstChildElement("p");
			if (node_poly_vcount && node_poly_idx)
			{
				const char* vcount_array = node_poly_vcount->GetText();
				const char* idx_array = node_poly_idx->GetText();
				Utils::ConvertStringToArray(vcount_array, polycounts);
				Utils::ConvertStringToArray(idx_array, polys);

				ImportVertices(node_mesh, node_poly);
			}
		}
	}

	return true;
}

bool Mesh::ImportVertices(XMLNode* node_mesh, XMLNode* node_poly)
{
	XMLElement* node_input = node_poly->FirstChildElement("input");
	while (node_input)
	{
		const char* semantic = node_input->Attribute("semantic");
		const char* source = node_input->Attribute("source");
		if (source && semantic)
		{
			const char* source_id = (source[0] == '#' ? source + 1 : source);
			if (0 == std::strcmp(semantic, "VERTEX"))
			{
				// find vertices
				XMLElement* node_vertices = Utils::FindNodeById(node_mesh, "vertices", source_id);
				if (node_vertices)
				{
					XMLElement* node_input_pos = node_vertices->FirstChildElement("input");
					const char* semantic_pos = node_input_pos->Attribute("semantic");
					const char* source_pos = node_input_pos->Attribute("source");
					if (source_pos && 0 == std::strcmp(semantic_pos, "POSITION"))
					{
						source_id = (source_pos[0] == '#' ? source_pos + 1 : source_pos);

                        position_stride = 3;
						ExtractSourceFloatArray(node_mesh, source_id, 3, positions);
					}
				}
			}
			else if (0 == std::strcmp(semantic, "NORMAL"))
			{
                normal_stride = 3;
				ExtractSourceFloatArray(node_mesh, source_id, 3, normals);
			}
			else if (0 == std::strcmp(semantic, "TEXCOORD"))
			{
                texcoord_stride = 2;
				ExtractSourceFloatArray(node_mesh, source_id, 2, texcoords);
			}
		}

		node_input = node_input->NextSiblingElement("input");
	}

	return true;
}

bool Mesh::ExtractSourceFloatArray(XMLNode* node_mesh, const char* source_id, const int stride, std::vector<float>& dst_array)
{
	XMLElement* node_source = Utils::FindNodeById(node_mesh, "source", source_id);
	if (node_source)
	{
		XMLElement* node_array = node_source->FirstChildElement("float_array");
		if (node_array)
		{
			int count = node_array->IntAttribute("count", 0);
			const char* str_array = node_array->GetText();
			Utils::ConvertStringToArray(str_array, dst_array);

			return true;
		}
	}

	return false;
}

void Mesh::Repair()
{
    // ensure that mesh is not degenerated
    size_t num_points = position_stride ? positions.size() / position_stride : 0;
	size_t position_size = num_points * position_stride;
    positions.resize(position_size);
    
	size_t num_normals = normal_stride ? normals.size() / normal_stride : 0;
	size_t normal_size = num_normals * normal_stride;
    normals.resize(normal_size);
    if (normal_stride && num_normals < num_points)
    {
        // warning: less normals than positions
        normals.resize(num_points * normal_stride, 0.577350259f);
    }
    
	size_t num_texcoords = texcoord_stride ? texcoords.size() / texcoord_stride : 0;
	size_t texcoord_size = num_texcoords * texcoord_stride;
    texcoords.resize(texcoord_size);
    if (texcoord_stride && num_texcoords < num_points)
    {
        // warning: less texcoords than positions
        texcoords.resize(num_points * texcoord_stride, 0.f);
    }
    
    // check for degenerate face (only triangles)
}

void Mesh::ExportToPly(std::ofstream& stream) const
{
    bool binary_data = false;
    
	size_t num_points = position_stride ? positions.size() / position_stride : 0;
	size_t num_faces = polycounts.size() ? polycounts.size() : polys.size() / 3;
    
    stream << "ply\n";
    if(binary_data)
        stream << "format binary_little_endian 1.0\n";
    else
        stream << "format ascii 1.0\n";
    
    stream << "comment " << name.c_str() << std::endl;
    stream << "comment exported from dae2pbrt\n";
    
    stream << "element vertex " << num_points << std::endl;
    
    stream << "property float x\n";
    stream << "property float y\n";
    stream << "property float z\n";
    
    if (normal_stride)
    {
        stream << "property float nx\n";
        stream << "property float ny\n";
        stream << "property float nz\n";
    }
    
    stream << "element face " << num_faces << std::endl;
    
    stream << "property list uchar int vertex_indices\n";
    stream << "end_header\n";
    
    /*if (binary_data)
    {

    }
    else*/
    {
        for (int p_idx = 0; p_idx < num_points; ++p_idx)
        {
            for (int c_idx = 0; c_idx < position_stride; ++c_idx)
                stream << positions[position_stride*p_idx + c_idx] << " ";
            for (int c_idx = 0; c_idx < normal_stride; ++c_idx)
                stream << normals[normal_stride*p_idx + c_idx] << " ";
            stream << std::endl;
        }
        
        if (polycounts.size())
        {
            int v_offset = 0;
            for (int face_idx = 0; face_idx < polycounts.size(); ++face_idx)
            {
                int poly_size = polycounts[face_idx];
                stream << poly_size << " ";
                
                for (int v_idx = 0; v_idx < poly_size; ++v_idx, ++v_offset)
                    stream << polys[v_offset] << " ";
                
                stream << std::endl;
            }
        }
        else
        {
            for (int face_idx = 0; face_idx < num_faces; ++face_idx)
            {
                stream << "3 " << polys[3*face_idx] << " " << polys[3*face_idx + 1] << " " << polys[3*face_idx + 2] << std::endl;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
MeshInstance::MeshInstance()
{
    Reset();
}
MeshInstance::~MeshInstance()
{
    
}

void MeshInstance::Reset()
{
    matrix.clear();
}

bool MeshInstance::ImportFromXML(XMLNode* node_sub)
{
    XMLElement* node_mat = node_sub->FirstChildElement("matrix");
    if (node_mat)
    {
        const char* str_array = node_mat->GetText();
        Utils::ConvertStringToArray(str_array, matrix);
    }
    
    XMLElement* node_inst = node_sub->FirstChildElement("instance_geometry");
    if (node_inst)
    {
        const char* url_name = node_inst->Attribute("url");
        mesh_name = (url_name && url_name[0] == '#' ? url_name + 1 : url_name);
        
        XMLElement* node_mat = node_inst->FirstChildElement("bind_material");
        if (node_mat)
        {
            XMLElement* node_tech = node_mat->FirstChildElement("technique_common");
            if (node_tech)
            {
                XMLElement* node_inst_mat = node_tech->FirstChildElement("instance_material");
                if (node_inst_mat)
                {
                    const char* target_name = node_inst_mat->Attribute("target");
                    material_name = (target_name && target_name[0] == '#' ? target_name + 1 : target_name);
                }
            }
        }
    }

	return true;
}

////////////////////////////////////////////////////////////////////////
Program::Program()
{

}
Program::~Program()
{
    std::map<std::string, Mesh*>::iterator it_mesh;
	for (it_mesh = meshes.begin(); it_mesh != meshes.end(); it_mesh++)
		delete it_mesh->second;
    
    for (int mesh_idx = 0; mesh_idx < mesh_instances.size(); mesh_idx++)
        delete mesh_instances[mesh_idx];
	
	meshes.clear();
    mesh_instances.clear();
}

void Program::ImportMeshes(XMLNode* node_lib)
{
	XMLElement* node_geom = node_lib->FirstChildElement("geometry");
	while (node_geom)
	{
		const char* id = node_geom->Attribute("id");
		const char* name = node_geom->Attribute("name");

		XMLElement* node_mesh = node_geom->FirstChildElement("mesh");
		if (node_mesh)
		{
            Mesh* mesh = new Mesh();
            mesh->name = id;
			mesh->ImportFromXML(node_mesh);
            mesh->Repair();
			meshes[id] = mesh;
		}

		node_geom = node_geom->NextSiblingElement("geometry");
	}
    
    
#if 0
<library_geometries>
    <geometry id="3004.dat" name="3004.dat">
        <mesh>
            <source id="3004.dat-pos" name="3004.dat-pos">
                <float_array id="3004.dat-pos-array" count="1449">
                    -20 0 -10 -20 0 -10 -20 0 -10 -20 0 10 -20 0 10 -20 0 10 -20 24 -10 -20 24 -10 -20 24 -10 -20 24 10 -20 24 10 -20 24 10 -16 -4 0 -16 -4 0 -16 0 0 -16 4 -6
                </float_array>
                <technique_common>
                    <accessor count="483" source="#3004.dat-pos-array" stride="3">
                        <param name="X" type="float" /><param name="Y" type="float" /><param name="Z" type="float" />
                    </accessor>
                </technique_common>
            </source>
            <source id="3004.dat-norm" name="3004.dat-norm">
                <float_array id="3004.dat-norm-array" count="1449">
                    0 0 -1 -1 0 0 0 -1 0 0 -1 0 -1 0 0 0 0 1 0 1 0 0 0 -1 -1 0 0 0 0 1 -1 0 0 0 1 0 0 -1 0 -0.9997588 0 -0.02196128 -0.9997588 0 0.02196128 0 1 0 0 0 1 1 0 0 1
                </float_array>
                <technique_common>
                    <accessor count="483" source="#3004.dat-norm-array" stride="3">
                        <param name="X" type="float" /><param name="Y" type="float" /><param name="Z" type="float" />
                    </accessor>
                </technique_common>
            </source>
            <vertices id="3004.dat-vert">
                <input semantic="POSITION" source="#3004.dat-pos" />
            </vertices>
            <triangles count="460" material="Base">
                <input semantic="VERTEX" source="#3004.dat-vert" offset="0" />
                <input semantic="NORMAL" source="#3004.dat-norm" offset="0" />
                <p>
                242 307 316 242 301 307 242 296 301 242 290 296 242 284 290 242 278 284 242 271 278 242 266 271 242 260 266 242 253 260 242 247 253 242 240 247 242 235 240
                </p>
            </triangles>
        </mesh>
    </geometry>
</library_geometries>
#endif
}

void Program::ImportNodes(XMLNode* node_lib)
{
    XMLElement* node = node_lib->FirstChildElement("node");
    while (node)
    {
        const char* id = node->Attribute("id");
        
        // parse sub-nodes
        XMLElement* node_sub = node->FirstChildElement("node");
        while (node_sub)
        {
            const char* instance_id = node_sub->Attribute("id");

            MeshInstance* mesh_instance = new MeshInstance();
            mesh_instance->instance_name = instance_id;
            mesh_instance->ImportFromXML(node_sub);
            mesh_instances.push_back(mesh_instance);
            
            node_sub = node_sub->NextSiblingElement("node");
        }
        
        node = node->NextSiblingElement("node");
    }
    
#if 0
<library_nodes>
    <node id="Node-UntitledModel.io">
        <node id="Inner-Node-UntitledModel.io-0">
            <matrix>1.00000    0.00000    0.00000    0.00000
                0.00000    1.00000    0.00000    -24.00000
                0.00000    0.00000    1.00000    -10.00000
                0.00000    0.00000    0.00000    1.00000
            </matrix>
            <matrix>0.99375 0 0 0
                    0 0.9910714 0 0.08928572
                    0 0 0.9875 0
                    0 0 0 1</matrix>
            <instance_geometry url="#3004.dat">
                <bind_material>
                    <technique_common>
                        <instance_material symbol="Base" target="#Color-1"/>
                    </technique_common>
                </bind_material>
            </instance_geometry>
        </node>
        <node id="Inner-Node-UntitledModel.io-1">
            <matrix>1.00000    0.00000    0.00000    60.00000
                0.00000    1.00000    0.00000    -24.00000
                0.00000    0.00000    1.00000    -10.00000
                0.00000    0.00000    0.00000    1.00000
            </matrix>
            <matrix>0.99375 0 0 0
                    0 0.9910714 0 0.08928572
                    0 0 0.9875 0
                    0 0 0 1</matrix>
            <instance_geometry url="#3004.dat">
                <bind_material>
                    <technique_common>
                        <instance_material symbol="Base" target="#Color-5"/>
                    </technique_common>
                </bind_material>
            </instance_geometry>
        </node>
    </node>
</library_nodes>
#endif
}

void Program::ImportVisualScene(XMLNode* node_lib)
{
	XMLElement* node = node_lib->FirstChildElement("visual_scene");
	if (node)
	{
		const char* id = node->Attribute("id");

		// parse sub-nodes
		XMLElement* node_sub = node->FirstChildElement("node");
		while (node_sub)
		{
			const char* instance_id = node_sub->Attribute("id");

			MeshInstance* mesh_instance = new MeshInstance();
			mesh_instance->instance_name = instance_id;
			mesh_instance->ImportFromXML(node_sub);
			mesh_instances.push_back(mesh_instance);

			node_sub = node_sub->NextSiblingElement("node");
		}

		node = node->NextSiblingElement("node");
	}

#if 0
<library_visual_scenes>
	<visual_scene id = "VisualSceneNode" name = "untitled">
		<node id = "LOD3sp" name = "LOD3sp">
			<rotate sid = "rotateZ">0 0 1 0< / rotate>
			<rotate sid = "rotateY">0 1 0 0< / rotate>
			<rotate sid = "rotateX">1 0 0 0< / rotate>
			<instance_geometry url = "#LOD3spShape-lib">
				<bind_material>
					<technique_common>
						<instance_material symbol = "blinn3SG" target = "#blinn3">
							<bind_vertex_input semantic = "TEX0" input_semantic = "TEXCOORD" input_set = "0" / >
						< / instance_material>
					< / technique_common>
				< / bind_material>
			< / instance_geometry>
		< / node>
		<node id = "camera1" name = "camera1">
			<translate sid = "translate">400.113 463.264 - 431.078< / translate>
			<rotate sid = "rotateZ">0 0 1 0< / rotate>
			<rotate sid = "rotateY">0 1 0 - 223.2< / rotate>
			<rotate sid = "rotateX">1 0 0 - 38.4< / rotate>
			<instance_camera url = "#cameraShape1" / >
		< / node>
		<node id = "directionalLight1" name = "directionalLight1">
			<translate sid = "translate">148.654 183.672 - 292.179< / translate>
			<rotate sid = "rotateZ">0 0 1 - 12.8709< / rotate>
			<rotate sid = "rotateY">0 1 0 - 191.679< / rotate>
			<rotate sid = "rotateX">1 0 0 - 45.6358< / rotate>
			<instance_light url = "#directionalLightShape1-lib" / >
		< / node>
	< / visual_scene>
< / library_visual_scenes>
#endif
}

void Program::ExportPlyMeshes() const
{
    std::string path, plyname;
    Utils::ExtractFilePath(options.filename, path);
    
    std::map<std::string, Mesh*>::const_iterator it_mesh;
    for (it_mesh = meshes.begin(); it_mesh != meshes.end(); it_mesh++)
    {
        Mesh* mesh = it_mesh->second;
        std::ofstream ply;
        plyname = path + mesh->name + ".ply";
        ply.open(plyname, std::ios::out | std::ios::binary);
        
        if (ply.is_open())
            mesh->ExportToPly(ply);
        else
        {
            //std::cerr << "open failed: " << strerror(errno) << '\n';
        }
    }
}

void Program::ExportPbrtScene() const
{
    std::string path, filename, extname, pbrtname;
    Utils::ExtractFilePath(options.filename, path, filename, extname);
    
    std::ofstream pbrt;
    pbrtname = path + filename + ".pbrt";
    pbrt.open(pbrtname, std::ios::out | std::ios::binary);
    
    if (pbrt.is_open())
    {
        std::ofstream& stream = pbrt;
        stream << "WorldBegin\n";
        
        std::map<std::string, Mesh*>::const_iterator it_mesh;
        for (it_mesh = meshes.begin(); it_mesh != meshes.end(); it_mesh++)
        {
            Mesh* mesh = it_mesh->second;
            
            stream << "\tObjectBegin \"" << mesh->name.c_str() << "\"\n";
            stream << "\tShape \"plymesh\" \"string filename\" \"" << mesh->name.c_str() << ".ply" << "\"\n";
            stream << "\tObjectEnd\n";
        }
        
        for (int mesh_idx = 0; mesh_idx < mesh_instances.size(); mesh_idx++)
        {
             MeshInstance* mesh_instance = mesh_instances[mesh_idx];
			 if (DoesMeshExist(mesh_instance))
			 {
				 stream << "\tTransformBegin\n";
				 if (!mesh_instance->matrix.empty())
				 {
					 stream << "\tTransform [";
					 for (int c_idx = 0; c_idx < mesh_instance->matrix.size(); c_idx++)
						 stream << mesh_instance->matrix[c_idx] << " ";
					 stream << "]\n";
				 }
				 stream << "\tObjectInstance \"" << mesh_instance->mesh_name.c_str() << "\"\n";
				 stream << "\tTransformEnd\n";
			 }
        }
        
        stream << "WorldEnd" << std::endl;
    }
}

bool Program::DoesMeshExist(MeshInstance* instance) const
{
	if (!instance)
		return false;

	std::map<std::string, Mesh*>::const_iterator it_mesh = meshes.find(instance->mesh_name);
	return it_mesh != meshes.end();
}

//////////////////////////////////////////////////////////////////////////////////////////
void Utils::ConvertStringToArray(const char* str, std::vector<int>& out_vector)
{
	const char* p = str;
	char* end = nullptr;
	for (int i = std::strtol(p, &end, 10); p != end; i = std::strtol(p, &end, 10))
	{
		p = end;
		out_vector.push_back(i);
	}
}

void Utils::ConvertStringToArray(const char* str, std::vector<float>& out_vector)
{
	const char* p = str;
	char* end = nullptr;
	for (float t = std::strtof(p, &end); p != end; t = std::strtof(p, &end))
	{
		p = end;
		out_vector.push_back(t);
	}
}

XMLElement* Utils::FindNodeById(XMLNode* node_parent, const char* node_name, const char* node_id)
{
	XMLElement* node = node_parent->FirstChildElement(node_name);
	while (node)
	{
		const char* str_id = node->Attribute("id");
		if (0 == std::strcmp(str_id, node_id))
		{
			return node;
		}

		node = node->NextSiblingElement(node_name);
	}
	return nullptr;
}

void Utils::ExtractFilePath(const std::string& fullname, std::string& out_path)
{
    size_t delimiter_idx = fullname.rfind('/');
    if (std::string::npos == delimiter_idx)
        delimiter_idx = fullname.rfind('\\');
    
    if (std::string::npos != delimiter_idx)
    {
        out_path = fullname.substr(0, delimiter_idx+1);
    }
    else
        out_path = "";
}

void Utils::ExtractFilePath(const std::string& fullname, std::string& out_path, std::string& out_filename, std::string& out_extension)
{
	size_t delimiter_idx = fullname.rfind('/');
    if (std::string::npos == delimiter_idx)
        delimiter_idx = fullname.rfind('\\');
    
    if (std::string::npos != delimiter_idx)
    {
        out_path = fullname.substr(0, delimiter_idx+1);
    }
    else
        out_path = "";
    
	size_t ext_idx = fullname.rfind('.');
    if (std::string::npos != ext_idx && ext_idx > delimiter_idx)
    {
        out_filename = fullname.substr(delimiter_idx+1, ext_idx - delimiter_idx - 1);
        out_extension = fullname.substr(ext_idx+1);
    }
    else
    {
        out_filename = fullname.substr(delimiter_idx+1);
        out_extension = "";
    }
}
