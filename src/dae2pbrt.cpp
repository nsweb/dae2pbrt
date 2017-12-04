/*
*/

#include "dae2pbrt.h"

void Usage(const char* error = nullptr)
{
    
}

// main program
int main(int argc, char *argv[])
{
	if (argc <= 1)
		return 0;
    
    Options options;
    
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
            options.filename = argv[i];
    }

	Program program;
    XMLDocument doc;

	XMLError result = doc.LoadFile( options.filename.c_str() );
	if (result != XML_SUCCESS)
		return 0;

	XMLNode* node_root = doc.FirstChildElement("COLLADA");
	if (!node_root)
	{
		// no node to process
		return 0;
	}

	XMLNode* node_lib_geom = node_root->FirstChildElement("library_geometries");
	program.ExtractMeshes(node_lib_geom);

	XMLNode* node = node_lib_geom->NextSibling();
        
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

		XMLElement* node_mesh = node_geom->FirstChildElement("mesh");
		if (node_mesh)
		{
            Mesh* mesh = new Mesh();
            mesh->name = id;
            
			// extract source, vertices and polylist
            XMLElement* node_tri = node_mesh->FirstChildElement("triangles");
            if (node_tri)
            {
                int tri_count = node_tri->IntAttribute("count", 0);
                XMLElement* node_tri_idx = node_tri->FirstChildElement("p");
                if (node_tri_idx)
                {
                    const char* idx_array = node_tri_idx->GetText();
                }
            }
            
            meshes.push_back(mesh);
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
