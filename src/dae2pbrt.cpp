/*
*/

#include "dae2pbrt.h"


// main program
int main(int argc, char *argv[])
{
	if (argc <= 1)
		return 0;

	Program program;
    XMLDocument doc;

	XMLError result = doc.LoadFile( argv[1] );
	if (result != XML_SUCCESS)
		return 0;

	XMLNode* node_root = doc.FirstChildElement("COLLADA");
	if (!node_root)
	{
		// no node to process
		return 0;
	}

	XMLNode* node_libgeom = node_root->FirstChildElement("library_geometries");
	program.ExtractMeshes(node_libgeom);

	XMLNode* node = node_libgeom->NextSibling();
        
    return 0;
}


////////////////////////////////////////////////////////////////////////
Mesh::Mesh()
{

}
Mesh::~Mesh()
{
	
}

////////////////////////////////////////////////////////////////////////
Program::Program()
{

}
Program::~Program()
{
	for (int mesh_idx = 0; mesh_idx < meshes.size(); mesh_idx++)
		delete meshes[mesh_idx];
	
	meshes.clear();
}

void Program::ExtractMeshes(XMLNode* node_lib)
{
	XMLElement* node_geom = node_lib->FirstChildElement("geometry");
	while (node_geom)
	{
		const char* id = node_geom->Attribute("id");
		const char* name = node_geom->Attribute("name");

		XMLElement* node_mesh = node_geom->FirstChildElement("geometry");
		if (node_mesh)
		{
			// extract source, vertices and polylist
		}

		node_geom = node_geom->NextSiblingElement("geometry");
	}
}