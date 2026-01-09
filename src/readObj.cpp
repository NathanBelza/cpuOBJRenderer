#include <fstream>
#include <string>
#include <sstream>
#include "screenRender.hpp"
#include "readObj.hpp"

#include <iostream>

void objToTriangles(std::string filename, std::vector<worldTriangle> &triangleArray) {
    std::ifstream model(filename.append(".obj"));
    if (!model) return;

    std::vector<point4D> vertex;
    std::string line;
    while(std::getline(model, line)) {
        if(line.starts_with("v ")) {
            point4D vTemp;
            char prefix;
            std::istringstream ss(line);

            ss >> prefix >> vTemp.x >> vTemp.y >> vTemp.z;
            vTemp.w = 1;
            vertex.push_back(vTemp);
        } else if(line.starts_with("f ")) {
            int vertA, vertB, vertC;
            char prefix;
            std::istringstream ss(line);

            ss >> prefix >> vertA >> vertB >> vertC;

            worldTriangle tempTri(vertex[vertA-1], vertex[vertB-1], vertex[vertC-1]);
            triangleArray.push_back(tempTri);
        }
    }
}