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

#include "dae2pbrt.h"
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

using namespace dae2pbrt;

void Usage(const char* error = nullptr)
{
    if (error)
        cerr << "dae2pbrt: " << error << "\n\n";
    
    cerr << "usage: dae2pbrt <filename.dae> [<options>]\n";
    cerr << "Options:\n";
    cerr << "--help                 Print this help text.\n";
    cerr << "--material <materail>  Use one of the following material for pbrt : matte, plastic, disney, uber.\n";
    cerr << "--skipmesh             Do not export PLY meshes.\n";
    cerr << "--noalphaglass         Do not extract texture alpha channel and treat as mask for glass material.\n";
    cerr << "--plyascii             Use ASCII encoding when exporting PLY meshes.\n";
    cerr << "--quiet                Suppress all text output other than error messages.\n";
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
		if (!strcmp(argv[i], "--material") || !strcmp(argv[i], "-material")) {
			if (i + 1 == argc)
				Usage("missing value after --material argument");
			program.options.default_material = argv[++i];
		}
		else if (!strcmp(argv[i], "--skipmesh") || !strcmp(argv[i], "-skipmesh")) {
			program.options.skip_mesh = true;
        }
        else if (!strcmp(argv[i], "--noalphaglass") || !strcmp(argv[i], "-noalphaglass")) {
            program.options.alpha_channel_to_glass = false;
        }
        else if (!strcmp(argv[i], "--plyascii") || !strcmp(argv[i], "-plyascii")) {
            program.options.ply_ascii = true;
        }
        else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-quiet")) {
            program.options.quiet = true;
		}
		else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-help") ||
            !strcmp(argv[i], "-h")) {
            Usage();
            return 0;
        } else
            program.options.filename = argv[i];
    }

	if (program.options.default_material == "matte")
		program.options.material_type = EPbrtMaterial::Matte;
	else if (program.options.default_material == "plastic")
		program.options.material_type = EPbrtMaterial::Plastic;
	else if (program.options.default_material == "disney")
		program.options.material_type = EPbrtMaterial::Disney;
    else if (program.options.default_material == "uber")
        program.options.material_type = EPbrtMaterial::Uber;

    XMLDocument doc;
	XMLError result = doc.LoadFile( program.options.filename.c_str() );
	if (result != XML_SUCCESS)
    {
        cerr << "Error: could not load file <" << program.options.filename.c_str() << ">\n";
		return -1;
    }

	XMLNode* node_root = doc.FirstChildElement("COLLADA");
	if (!node_root)
	{
		// no node to process
        cerr << "Error: did not find any COLLADA node to process\n";
		return -1;
	}

    XMLNode* node_lib_tex = node_root->FirstChildElement("library_images");
    if (node_lib_tex)
        program.ImportTextures(node_lib_tex);
    
    XMLNode* node_lib_mat = node_root->FirstChildElement("library_materials");
	XMLNode* node_lib_effect = node_root->FirstChildElement("library_effects");
    if (node_lib_mat && node_lib_effect)
        program.ImportMaterials(node_lib_mat, node_lib_effect);
    
	XMLNode* node_lib_geom = node_root->FirstChildElement("library_geometries");
	if (node_lib_geom)
		program.ImportMeshes(node_lib_geom);

	program.ImportSceneNodes(node_root);

    program.ExportTextureAlphaChannels();
    program.ExportPlyMeshes();
    program.ExportPbrtScene();
        
    return 0;
}

////////////////////////////////////////////////////////////////////////
bool Material::SourceColor::ImportFromXML(XMLNode* node_source, XMLElement* node_profile)
{
    XMLElement* node_col = node_source->FirstChildElement("color");
    if (node_col)
    {
        const char* idx_array = node_col->GetText();
        Utils::ConvertStringToArray(idx_array, col);
    }
    else
    {
        XMLElement* node_tex = node_source->FirstChildElement("texture");
        if (node_tex)
        {
            const char* texture_sampler = node_tex->Attribute("texture");
            const char* texcoord = node_tex->Attribute("texcoord");
            if (texture_sampler)
            {
                XMLElement* node_samp = Utils::FindNodeBySid(node_profile, "newparam", texture_sampler);
                if (node_samp)
                {
                    XMLElement* node_samp2D = node_samp->FirstChildElement("sampler2D");
                    if (node_samp2D)
                    {
                        XMLElement* node_source = node_samp2D->FirstChildElement("source");
                        if (node_source)
                        {
                            const char* surface_name = node_source->GetText();
                            if (surface_name)
                            {
                                XMLElement* node_surf = Utils::FindNodeBySid(node_profile, "newparam", surface_name);
                                if (node_surf)
                                {
                                    XMLElement* node_surf2D = node_surf->FirstChildElement("surface");
                                    if (node_surf2D)
                                    {
                                        XMLElement* node_init = node_surf2D->FirstChildElement("init_from");
                                        if (node_init)
                                        {
                                            tex_name = node_init->GetText();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

void Material::SourceColor::ExportPbrt(ofstream& stream, const char* field, EPbrtMaterial::Type material_type)
{
    if (tex_name != "")
    {
        if (material_type == EPbrtMaterial::Disney)
            stream << "\"texture color\" [ ";
        else
            stream << "\"texture " << field << "\" [ \"";
        stream << tex_name.c_str() << "\" ] ";
    }
    else
    {
        size_t col_size = min((size_t)3, col.size());
        if (col_size)
        {
            if (material_type == EPbrtMaterial::Disney)
                stream << "\"rgb color\" [ ";
            else
                stream << "\"rgb " << field << "\" [ ";
            for (int c_idx = 0; c_idx < col_size; c_idx++)
                stream << col[c_idx] << " ";
            stream << "] ";;
        }
    }
}

bool Material::ImportFromXML(XMLNode* node_lib_effect)
{
	XMLElement* node_effect = node_lib_effect->FirstChildElement("effect");
	while (node_effect)
	{
		const char* id = node_effect->Attribute("id");
		if (fx_name == id)
		{
			// we only manage common profile (no GLSL etc.)
			XMLElement* node_profile = node_effect->FirstChildElement("profile_COMMON");
			if (node_profile)
			{
				XMLElement* node_tech = node_profile->FirstChildElement("technique");
				if (node_tech)
				{
					XMLElement* node_shading = node_tech->FirstChildElement();
					if (node_shading)
					{
						string shading_name = node_shading->Name();
						// we only manage phong and blinn (no constant, lambert etc.)
						if (shading_name == "phong" || shading_name == "blinn")
						{
							XMLElement* node = node_shading->FirstChildElement("diffuse");
							if (node)
							{
                                diffuse.ImportFromXML(node, node_profile);
							}

							node = node_shading->FirstChildElement("specular");
							if (node)
							{
								specular.ImportFromXML(node, node_profile);
							}

							node = node_shading->FirstChildElement("shininess");
							if (node)
							{
								XMLElement* node_col = node->FirstChildElement("float");
								if (node_col)
								{
									shininess = node_col->FloatText();
								}
							}
                            node = node_shading->FirstChildElement("transparency");
                            if (node)
                            {
                                XMLElement* node_col = node->FirstChildElement("float");
                                if (node_col)
                                {
                                    transparency = node_col->FloatText();
                                }
                            }
						}
					}
				}
			}

			break;
		}

		node_effect = node_effect->NextSiblingElement("effect");
	}

    return true;
}

////////////////////////////////////////////////////////////////////////
bool Mesh::ImportFromXML(XMLNode* node_mesh, bool skip_vertices)
{
	// extract source, vertices and polylist
	XMLElement* node_tri = node_mesh->FirstChildElement("triangles");
	if (node_tri)
	{
		int tri_count = node_tri->IntAttribute("count", 0);
		XMLElement* node_tri_idx = node_tri->FirstChildElement("p");
		if (!skip_vertices && node_tri_idx)
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
			if (!skip_vertices && node_poly_vcount && node_poly_idx)
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
        int offset = node_input->IntAttribute("offset", 0);
        max_offset = max(max_offset, offset);
        
		if (source && semantic)
		{
			const char* source_id = (source[0] == '#' ? source + 1 : source);
			if (0 == strcmp(semantic, "VERTEX"))
			{
				// find vertices
				XMLElement* node_vertices = Utils::FindNodeById(node_mesh, "vertices", source_id);
				if (node_vertices)
				{
					XMLElement* node_input_pos = node_vertices->FirstChildElement("input");
					const char* semantic_pos = node_input_pos->Attribute("semantic");
					const char* source_pos = node_input_pos->Attribute("source");
					if (source_pos && 0 == strcmp(semantic_pos, "POSITION"))
					{
						source_id = (source_pos[0] == '#' ? source_pos + 1 : source_pos);

                        position_stride = 3;
                        position_offset = offset;
						ExtractSourceFloatArray(node_mesh, source_id, 3, positions);
					}
				}
			}
			else if (0 == strcmp(semantic, "NORMAL"))
			{
                normal_stride = 3;
                normal_offset = offset;
				ExtractSourceFloatArray(node_mesh, source_id, 3, normals);
			}
			else if (0 == strcmp(semantic, "TEXCOORD"))
			{
                texcoord_stride = 2;
                texcoord_offset = offset;
				ExtractSourceFloatArray(node_mesh, source_id, 2, texcoords);
			}
		}

		node_input = node_input->NextSiblingElement("input");
	}

	return true;
}

bool Mesh::ExtractSourceFloatArray(XMLNode* node_mesh, const char* source_id, const int stride, vector<float>& dst_array)
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

void Mesh::BuildVertices()
{
    // polylist can have multiple indices for each input source, we need to make a unique index list for PLY export
    size_t num_faces = polycounts.size();
    if (num_faces == 0)
    {
        all_triangles = true;
        num_faces = polys.size() / (3 * (max_offset + 1));
        polycounts.resize(num_faces, 3);
    }
    else
    {
        all_triangles = false;
    }
    
    int num_face_indices = 0;
    for (int face_idx = 0; face_idx < num_faces; ++face_idx)
    {
        int poly_size = polycounts[face_idx];
        num_face_indices += poly_size;
    }
    if (num_face_indices * (max_offset + 1) != polys.size())
    {
        // warning : should be the same
        int brk = 0;
    }

    int g_offset = 0;
    int v_offset = 0;
    vertices.clear();
    for (int face_idx = 0; face_idx < num_faces; ++face_idx)
    {
        int poly_size = polycounts[face_idx];
        for (int poly_idx = 0; poly_idx < poly_size; ++poly_idx, v_offset += (max_offset + 1))
        {
            int position_index = polys[v_offset + position_offset];
            int normal_index = polys[v_offset + normal_offset];
            int texcoord_index = polys[v_offset + texcoord_offset];
            auto ret = vertices.insert(pair<tuple<int, int, int>, int>(make_tuple(position_index, normal_index, texcoord_index), g_offset));
            if (ret.second == true)
                ++g_offset;
        }
    }
}

void Mesh::Repair()
{
 
}

void Mesh::ExportToPly(ofstream& stream, bool binary_encoding) const
{
	size_t num_points = vertices.size();
	size_t num_faces = polycounts.size();
    
    stream << "ply\n";
    if (binary_encoding)
        stream << "format binary_little_endian 1.0\n";
    else
        stream << "format ascii 1.0\n";
    
    stream << "comment " << name.c_str() << endl;
    stream << "comment exported from dae2pbrt\n";
    
    stream << "element vertex " << num_points << endl;
    
    stream << "property float x\n";
    stream << "property float y\n";
    stream << "property float z\n";
    
    if (normal_stride)
    {
        stream << "property float nx\n";
        stream << "property float ny\n";
        stream << "property float nz\n";
    }
    
    if (texcoord_stride)
    {
        stream << "property float u\n";
        stream << "property float v\n";
    }
    
    stream << "element face " << num_faces << endl;
    
    stream << "property list int int vertex_indices\n";
    stream << "end_header\n";
    

    // write vertices
    {
        int g_offset = 0;
        int v_offset = 0;
        for (int face_idx = 0; face_idx < num_faces; ++face_idx)
        {
            int poly_size = polycounts[face_idx];
            for (int poly_idx = 0; poly_idx < poly_size; ++poly_idx, v_offset += (max_offset + 1))
            {
                int position_index = polys[v_offset + position_offset];
                int normal_index = polys[v_offset + normal_offset];
                int texcoord_index = polys[v_offset + texcoord_offset];
                tuple<int, int, int> vertex_index = make_tuple(position_index, normal_index, texcoord_index);
                auto ret = vertices.find(vertex_index);
                if (ret != vertices.end())
                {
                    if (g_offset == ret->second)
                    {
                        if (binary_encoding)
                        {
                            for (int c_idx = 0; c_idx < position_stride; ++c_idx)
                                stream.write( (const char*)&positions[position_stride*position_index + c_idx], sizeof(float));
                            for (int c_idx = 0; c_idx < normal_stride; ++c_idx)
                                stream.write( (const char*)&normals[normal_stride*normal_index + c_idx], sizeof(float));
                            for (int c_idx = 0; c_idx < texcoord_stride; ++c_idx)
                                stream.write( (const char*)&texcoords[texcoord_stride*texcoord_index + c_idx], sizeof(float));
                        }
                        else
                        {
                            for (int c_idx = 0; c_idx < position_stride; ++c_idx)
                                stream << positions[position_stride*position_index + c_idx] << " ";
                            for (int c_idx = 0; c_idx < normal_stride; ++c_idx)
                                stream << normals[normal_stride*normal_index + c_idx] << " ";
                            for (int c_idx = 0; c_idx < texcoord_stride; ++c_idx)
                                stream << texcoords[texcoord_stride*texcoord_index + c_idx] << " ";
                            stream << endl;
                        }
                        
                        ++g_offset;
                    }
                    else
                    {
                        // vertex was already written
                    }
                }
                else
                {
                    int brk = 0;
                }
            }
        }
    }
    
    // write faces
    {
        int g_offset = 0;
        int v_offset = 0;
        for (int face_idx = 0; face_idx < num_faces; ++face_idx)
        {
            int poly_size = polycounts[face_idx];
            if (binary_encoding)
            {
                stream.write((const char*)&poly_size, sizeof(int));
            }
            else
            {
                stream << poly_size << " ";
            }
            
            for (int poly_idx = 0; poly_idx < poly_size; ++poly_idx, v_offset += (max_offset + 1))
            {
                int position_index = polys[v_offset + position_offset];
                int normal_index = polys[v_offset + normal_offset];
                int texcoord_index = polys[v_offset + texcoord_offset];
                tuple<int, int, int> vertex_index = make_tuple(position_index, normal_index, texcoord_index);
                auto ret = vertices.find(vertex_index);
                if (ret != vertices.end())
                {
                    if (binary_encoding)
                    {
                        stream.write((const char*)&ret->second, sizeof(int));
                    }
                    else
                    {
                        stream << ret->second << " ";
                    }
                }
                else
                {
                    int brk = 0;
                }
            }
            
            if (!binary_encoding)
                stream << endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////
bool SceneNode::ImportFromXML(XMLNode* node_sub)
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

	node_inst = node_sub->FirstChildElement("instance_node");
	if (node_inst)
	{
		const char* url_name = node_inst->Attribute("url");
		string inst_name = (url_name && url_name[0] == '#' ? url_name + 1 : url_name);
		child_nodes.push_back(inst_name);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////
Program::Program()
{

}
Program::~Program()
{
	materials.clear();
	meshes.clear();
	scene_nodes.clear();
}

void Program::ImportTextures(XMLNode* node_lib)
{
    XMLElement* node_img = node_lib->FirstChildElement("image");
    while (node_img)
    {
        const char* id = node_img->Attribute("id");
        shared_ptr<Texture> tex( new Texture() );
        tex->name = id;
        
        XMLElement* node_init = node_img->FirstChildElement("init_from");
        if (node_init)
        {
            tex->path_name = node_init->GetText();
        }
        
        textures[id] = tex;
        
        node_img = node_img->NextSiblingElement("image");
    }
}

void Program::ExportTextureAlphaChannels()
{
    if (!options.alpha_channel_to_glass)
        return;
    
    string base_path, tex_filename;
    Utils::ExtractFilePath(options.filename, base_path);
    
    for (auto it_tex = textures.begin(); it_tex != textures.end(); ++it_tex)
    {
        shared_ptr<Texture> texture = it_tex->second;
        tex_filename = base_path + texture->path_name;
        
        if (stbi_is_hdr(tex_filename.c_str()))
            continue;
        
        int img_width = 0, img_height = 0, img_comp = 0;
        unsigned char* img_data = stbi_load(tex_filename.c_str(), &img_width, &img_height, &img_comp, 0);
        
        // only consider image with alpha
        if (img_comp == 4 && img_data)
        {
            vector<unsigned char> alpha_data(img_width * img_height, 0);
            
            unsigned char min_alpha = 255;
            for (int y = 0; y < img_height; ++y)
            {
                for (int x = 0; x < img_width; ++x)
                {
                    unsigned char alpha = img_data[img_comp*(img_width * y + x) + 3];
                    alpha_data[img_width * y + x] = alpha;
                    min_alpha = min(min_alpha, alpha);
                }
            }
            
            if (min_alpha == 255)
                continue;
            
            string img_path, img_filename, img_ext, alpha_filename;
            Utils::ExtractFilePath(texture->path_name, img_path, img_filename, img_ext);
            texture->alpha_name = img_filename + "_alpha_png";
            texture->alpha_path_name = img_path + img_filename + "_alpha.png";
            alpha_filename = base_path + texture->alpha_path_name;
            
            if (!stbi_write_png(alpha_filename.c_str(), img_width, img_height, 1, &alpha_data[0], img_width))
            {
                texture->alpha_name = "";
                cerr << "Error: could not write <" << alpha_filename.c_str() << ">\n";
            }
        }
        
        stbi_image_free(img_data);
    }
}

void Program::ImportMaterials(XMLNode* node_lib_mat, XMLNode* node_lib_effect)
{
	// parse material lib, and look for a corresponding fx
	XMLElement* node_mat = node_lib_mat->FirstChildElement("material");
	while (node_mat)
	{
		const char* id = node_mat->Attribute("id");
		shared_ptr<Material> mat( new Material() );
		mat->name = id;
		
		XMLElement* node_inst = node_mat->FirstChildElement("instance_effect");
		if (node_inst)
		{
			const char* url_name = node_inst->Attribute("url");
			mat->fx_name = (url_name && url_name[0] == '#' ? url_name + 1 : url_name);
			mat->ImportFromXML(node_lib_effect);
		}
		
		materials[id] = mat;

		node_mat = node_mat->NextSiblingElement("material");
	}
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
            shared_ptr<Mesh> mesh( new Mesh() );
            mesh->name = id;
			mesh->ImportFromXML(node_mesh, options.skip_mesh);
            mesh->BuildVertices();
            mesh->Repair();
			meshes[id] = mesh;
		}

		node_geom = node_geom->NextSiblingElement("geometry");
	}
    
    
}

void Program::ImportSceneNodes(XMLNode* node_root)
{
	XMLNode* node_scene = node_root->FirstChildElement("scene");
	if (node_scene)
	{
		XMLElement* node_inst_visual_scene = node_scene->FirstChildElement("instance_visual_scene");
		if (node_inst_visual_scene)
		{
			const char* url_name = node_inst_visual_scene->Attribute("url");
			string visual_scene_id = (url_name && url_name[0] == '#' ? url_name + 1 : url_name);

			XMLNode* node_lib_visual_scenes = node_root->FirstChildElement("library_visual_scenes");
			if (node_lib_visual_scenes)
			{
				XMLElement* node_visual_scene = node_lib_visual_scenes->FirstChildElement("visual_scene");
				while (node_visual_scene)
				{
					const char* id = node_visual_scene->Attribute("id");
					if (visual_scene_id == id)
					{
						ImportSubNodes(node_visual_scene, true);
						break;
					}
					node_visual_scene = node_lib_visual_scenes->NextSiblingElement("visual_scene");
				}
			}
		}
	}

	XMLNode* node_lib_nodes = node_root->FirstChildElement("library_nodes");
	if (node_lib_nodes)
	{
		ImportSubNodes(node_lib_nodes);
	}
}

void Program::ImportSubNodes(XMLNode* node_parent, bool visual_node, SceneNode* parent)
{
    XMLElement* node = node_parent->FirstChildElement("node");
    while (node)
    {
        const char* node_id = node->Attribute("id");

		shared_ptr<SceneNode> scene_node(new SceneNode());
        scene_node->visual_node = visual_node;
		scene_node->node_name = node_id;
		scene_node->ImportFromXML(node);
		scene_nodes[node_id] = scene_node;

		// register as sub-node of parent
		if (parent)
			parent->child_nodes.push_back(scene_node->node_name);

        ImportSubNodes(node, false, scene_node.get());
        
        node = node->NextSiblingElement("node");
    }
}

void Program::ExportPlyMeshes() const
{
	if (options.skip_mesh)
		return;

    string path, plyname;
    Utils::ExtractFilePath(options.filename, path);
    
    for (auto it_mesh = meshes.begin(); it_mesh != meshes.end(); ++it_mesh)
    {
        shared_ptr<Mesh> mesh = it_mesh->second;
        ofstream ply;
        plyname = path + mesh->name + ".ply";
        ply.open(plyname, ios::out | ios::binary);
        
        if (ply.is_open())
            mesh->ExportToPly(ply, !options.ply_ascii);
        else
        {
            cerr << "Error: open failed <" << plyname.c_str() << ">: " << strerror(errno) << "\n";
        }
    }
}

void Program::ExportPbrtScene() const
{
    string path, filename, extname, pbrtname;
    Utils::ExtractFilePath(options.filename, path, filename, extname);
    
    ofstream pbrt;
    pbrtname = path + filename + ".pbrt";
    pbrt.open(pbrtname, ios::out | ios::binary);
    
    if (pbrt.is_open())
    {
        ofstream& stream = pbrt;
        stream << "Include \"RenderIncl.pbrt\"\n\n";
        stream << "WorldBegin\n";
        stream << "\tInclude \"WorldIncl.pbrt\"\n";
        
        // list all textures
        for (auto it_tex = textures.begin(); it_tex != textures.end(); ++it_tex)
        {
            shared_ptr<Texture> texture = it_tex->second;
            stream << "\tTexture \"" << texture->name.c_str() << "\" \"color\" \"imagemap\" \"string filename\" [\"" << texture->path_name.c_str() << "\"]\n";
            if (texture->alpha_name != "")
            {
                stream << "\tTexture \"" << texture->alpha_name.c_str() << "\" \"color\" \"imagemap\" \"string filename\" [\"" << texture->alpha_path_name.c_str() << "\"]\n";
            }
        }
        
        // create glass material required for alpha textures
        if (options.alpha_channel_to_glass)
        {
            stream << "\tMakeNamedMaterial \"mixglass\" \"string type\" \"glass\" \"rgb Kr\" [0.5 0.5 0.5] \"rgb Kt\" [0.5 0.5 0.5]\n\n";
        }
        
        // pbrt does not support instancing with different materials, so we need to create a specific object for each different combination of mesh + material
        set<string> unique_mm;
        string mm_name;
		for (auto it_node = scene_nodes.begin(); it_node != scene_nodes.end(); ++it_node)
        {
			shared_ptr<SceneNode> scene_node = it_node->second;
            if (DoesMeshExist(scene_node.get()))
            {
                mm_name = scene_node->mesh_name;
                if (!scene_node->material_name.empty())
                    mm_name += "_" + scene_node->material_name;
                
                if (unique_mm.find(mm_name) == unique_mm.end())
                {
                    stream << "\tObjectBegin \"" << mm_name.c_str() << "\"\n";
                    
                    auto it_mat = materials.find(scene_node->material_name);
                    if (it_mat != materials.end())
                    {
						shared_ptr<Material> const& mat = it_mat->second;
                        if (mat.get() && !mat->fx_name.empty())
                        {
                            bool create_mix_material = false;
                            const char* alpha_cstr = nullptr;
                            if (options.alpha_channel_to_glass)
                            {
                                // if alpha channel was extracted, we need to create an intermediate material to blend base material with glass
                                auto it_ref_tex = textures.find(mat->diffuse.tex_name);
                                if (it_ref_tex != textures.end())
                                {
                                    shared_ptr<Texture> tex_ref = it_ref_tex->second;
                                    if (tex_ref->alpha_name != "")
                                    {
                                        alpha_cstr = tex_ref->alpha_name.c_str();
                                        create_mix_material = true;
                                    }
                                }
                            }
                            if (create_mix_material)
                                stream << "\tMakeNamedMaterial \"mat-" << mm_name.c_str() << "\" \"string type\" \"" << options.default_material << "\" ";
                            else
                                stream << "\tMaterial \"" << options.default_material << "\" ";
                            
                            mat->diffuse.ExportPbrt(stream, "Kd", options.material_type);
                            mat->specular.ExportPbrt(stream, "Ks", options.material_type);
                            
                            if (options.material_type == EPbrtMaterial::Disney ||
                                options.material_type == EPbrtMaterial::Uber)
							{
								stream << "\"float roughness\" [0.5] ";// \"rgb scatterdistance\" [0.01 0.01 0.01] ";
							}
                            if (options.material_type == EPbrtMaterial::Uber && mat->transparency < 1.f)
                            {
                                stream << "\"rgb opacity\" [ " << mat->transparency << " " << mat->transparency << " " << mat->transparency << " ]";
                            }
                            stream << endl;
                            
                            if (create_mix_material)
                            {
                                stream << "\tMaterial \"mix\" \"string namedmaterial1\" [ \"mat-" << mm_name.c_str() << "\" ] \"string namedmaterial2\" [ \"mixglass\" ] \"texture amount\" [ \"" << alpha_cstr << "\" ]\n";
                            }
                        }
                    }
                    
                    stream << "\tShape \"plymesh\" \"string filename\" \"" << scene_node->mesh_name.c_str() << ".ply" << "\"\n";
                    stream << "\tObjectEnd\n";
                    
                    unique_mm.insert(mm_name);
                }
            }
        }
        
        stream << "\n";
        
        // export scene recursively, starting from visual nodes
		for (auto it_node = scene_nodes.begin(); it_node != scene_nodes.end(); ++it_node)
		{
			shared_ptr<SceneNode> const& scene_node = it_node->second;
            if (scene_node->visual_node)
            {
                ExportPbrtNode(stream, scene_node.get(), 1);
            }
        }
        
        stream << "WorldEnd" << endl;
    }
}

void Program::ExportPbrtNode(ofstream& stream, SceneNode* scene_node, int indent_level) const
{
    if (!scene_node)
        return;
    
    string indent("");
    for (int indent_idx = 0; indent_idx < indent_level; indent_idx++)
        indent += "\t";
    
    bool has_transform = (scene_node->matrix.size() == 16);
    if (has_transform)
    {
        stream << indent.c_str() << "TransformBegin\n";
        stream << indent.c_str() << "ConcatTransform [";
        stream << scene_node->matrix[0] << " " << scene_node->matrix[4] << " " << scene_node->matrix[8] << " " << scene_node->matrix[12] << " ";
        stream << scene_node->matrix[1] << " " << scene_node->matrix[5] << " " << scene_node->matrix[9] << " " << scene_node->matrix[13] << " ";
        stream << scene_node->matrix[2] << " " << scene_node->matrix[6] << " " << scene_node->matrix[10] << " " << scene_node->matrix[14] << " ";
        stream << scene_node->matrix[3] << " " << scene_node->matrix[7] << " " << scene_node->matrix[11] << " " << scene_node->matrix[15] << " ";
        stream << "]\n";
    }
    
    if (DoesMeshExist(scene_node))
    {
        //if (!mesh_instance->material_name.empty())
        //    stream << "\tNamedMaterial \"" << mesh_instance->material_name.c_str() << "\"\n";
        
        string mm_name = scene_node->mesh_name;
        if (!scene_node->material_name.empty())
            mm_name += "_" + scene_node->material_name;
        
        stream << indent.c_str() << "ObjectInstance \"" << mm_name.c_str() << "\"\n";
    }
    else if (has_transform)
    {
        stream << indent.c_str() << "# " << scene_node->node_name.c_str() << endl;
    }
    
    for (const string& child_name : scene_node->child_nodes)
    {
        auto it_node = scene_nodes.find(child_name);
        if (it_node != scene_nodes.end())
        {
            ExportPbrtNode(stream, it_node->second.get(), indent_level+1);
        }
    }
    
    if (has_transform)
        stream << indent.c_str() << "TransformEnd\n";
}

bool Program::DoesMeshExist(SceneNode* scene_node) const
{
	if (!scene_node || scene_node->mesh_name.empty())
		return false;

    const auto it_mesh = meshes.find(scene_node->mesh_name);
	return it_mesh != meshes.end();
}

//////////////////////////////////////////////////////////////////////////////////////////
void Utils::ConvertStringToArray(const char* str, vector<int>& out_vector)
{
	const char* p = str;
	char* end = nullptr;
	for (int i = strtol(p, &end, 10); p != end; i = strtol(p, &end, 10))
	{
		p = end;
		out_vector.push_back(i);
	}
}

void Utils::ConvertStringToArray(const char* str, vector<float>& out_vector)
{
	const char* p = str;
	char* end = nullptr;
	for (float t = strtof(p, &end); p != end; t = strtof(p, &end))
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
		if (0 == strcmp(str_id, node_id))
		{
			return node;
		}

		node = node->NextSiblingElement(node_name);
	}
	return nullptr;
}

XMLElement* Utils::FindNodeBySid(XMLNode* node_parent, const char* node_name, const char* node_id)
{
    XMLElement* node = node_parent->FirstChildElement(node_name);
    while (node)
    {
        const char* str_id = node->Attribute("sid");
        if (0 == strcmp(str_id, node_id))
        {
            return node;
        }
        
        node = node->NextSiblingElement(node_name);
    }
    return nullptr;
}

void Utils::ExtractFilePath(const string& fullname, string& out_path)
{
    size_t delimiter_idx = fullname.rfind('/');
    if (string::npos == delimiter_idx)
        delimiter_idx = fullname.rfind('\\');
    
    if (string::npos != delimiter_idx)
    {
        out_path = fullname.substr(0, delimiter_idx+1);
    }
    else
        out_path = "";
}

void Utils::ExtractFilePath(const string& fullname, string& out_path, string& out_filename, string& out_extension)
{
	size_t delimiter_idx = fullname.rfind('/');
    if (string::npos == delimiter_idx)
        delimiter_idx = fullname.rfind('\\');
    
    if (string::npos != delimiter_idx)
    {
        out_path = fullname.substr(0, delimiter_idx+1);
    }
    else
        out_path = "";
    
	size_t ext_idx = fullname.rfind('.');
    if (string::npos != ext_idx && ext_idx > delimiter_idx)
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
