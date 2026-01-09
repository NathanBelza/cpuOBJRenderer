#ifndef MATRICES
#define MATRICES

#include <array>
#include "screenRender.hpp"

void matrixMultiply(const std::array<float, 16> &A, const std::array<float, 16> &B, std::array<float, 16> &C);

point4D matrixVectorMultiply(const std::array<float, 16> &A, point4D V);

void cameraToOrigin(Camera &camera, std::array<float, 16> &A);
void cameraRotateYaw(Camera &camera, std::array<float, 16> &A);
void cameraRotatePitch(Camera &camera, std::array<float, 16> &A);
void cameraToClipSpace(Camera &camera, float aspectRatio, std::array<float, 16> &A);

#endif