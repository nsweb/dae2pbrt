/*
*/

#include "tinyxml2/tinyxml2.h"

// main program
int main(int argc, char *argv[])
{
    if (argc >= 1 )
    {
        tinyxml2::XMLDocument doc;
        doc.LoadFile( argv[1] );
    }
    
    return 0;
}
