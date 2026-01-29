#ifndef READ_OBJ
#define READ_OBJ

#include <fstream>
#include <string>
#include <vector>
#include "screenRender.hpp"

// Convert .obj file to an array of worldTriangle objects. Obj file must have
// UV coordinates removed.
void objToTriangles(std::string filename, std::vector<worldTriangle> &triangleArray);

#endif