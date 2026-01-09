#ifndef READ_OBJ
#define READ_OBJ

#include <fstream>
#include <string>
#include <vector>
#include "screenRender.hpp"

void objToTriangles(std::string filename, std::vector<worldTriangle> &triangleArray);

#endif