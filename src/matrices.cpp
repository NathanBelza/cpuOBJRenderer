#include <array>
#include "screenRender.hpp"
#include "matrices.hpp"

size_t index(size_t col, size_t row) {return col + 4 * row;}

void matrixMultiply(const std::array<float, 16> &A, const std::array<float, 16> &B, std::array<float, 16> &C) {
    for(size_t row = 0; row < 4; row++) {
        for(size_t col = 0; col < 4; col++) {
            float sum = 0.0f;
            for(size_t p = 0; p < 4; p++) {
                sum += A[index(p,row)] * B[index(col,p)];
            }
            C[index(col,row)] = sum;
        }
    }
}

point4D matrixVectorMultiply(const std::array<float, 16> &A, point4D V) { //Confirmed works
    point4D _V;
    _V.x = A[0] * V.x + A[1] * V.y + A[2] * V.z + A[3] * V.w;
    _V.y = A[4] * V.x + A[5] * V.y + A[6] * V.z + A[7] * V.w;
    _V.z = A[8] * V.x + A[9] * V.y + A[10] * V.z + A[11] * V.w;
    _V.w = A[12] * V.x + A[13] * V.y + A[14] * V.z + A[15] * V.w;
    return _V;
}

void cameraToOrigin(Camera &camera, std::array<float, 16> &A) {
    point4D cameraPos = camera.getPos();

    A[0] = 1; //Row 1
    A[1] = 0;
    A[2] = 0;
    A[3] = -cameraPos.x;

    A[4] = 0; //Row 2
    A[5] = 1;
    A[6] = 0;
    A[7] = -cameraPos.y;

    A[8] = 0; //Row 3
    A[9] = 0;
    A[10] = 1;
    A[11] = -cameraPos.z;

    A[12] = 0; //Row 4
    A[13] = 0;
    A[14] = 0;
    A[15] = 1;
}

void cameraRotateYaw(Camera &camera, std::array<float, 16> &A) {
    float yaw = camera.getYawR();
    float sinYaw = sin(yaw);
    float cosYaw = cos(yaw);

    A[0] = cosYaw; //Row 1
    A[1] = 0;
    A[2] = -sinYaw;
    A[3] = 0;

    A[4] = 0; //Row 2
    A[5] = 1;
    A[6] = 0;
    A[7] = 0;

    A[8] = sinYaw; //Row 3
    A[9] = 0;
    A[10] = cosYaw;
    A[11] = 0;

    A[12] = 0; //Row 4
    A[13] = 0;
    A[14] = 0;
    A[15] = 1;
}

void cameraRotatePitch(Camera &camera, std::array<float, 16> &A) {
    float pitch = camera.getPitchR();
    float sinPitch = sin(pitch);
    float cosPitch = cos(pitch);

    A[0] = 1; //Row 1
    A[1] = 0;
    A[2] = 0;
    A[3] = 0;

    A[4] = 0; //Row 2
    A[5] = cosPitch;
    A[6] = -sinPitch;
    A[7] = 0;

    A[8] = 0; //Row 3
    A[9] = sinPitch;
    A[10] = cosPitch;
    A[11] = 0;

    A[12] = 0; //Row 4
    A[13] = 0;
    A[14] = 0;
    A[15] = 1;
}

void cameraToClipSpace(Camera &camera, float aspectRatio, std::array<float, 16> &A) {
    float nearPlaneDist = camera.getNear();
    float farPlaneDist = camera.getFar();
    float fov = camera.getFovR();
    float tFov = tan(fov * 0.5f); //Near / Right

    A[0] = 1 / tFov; //Row 1
    A[1] = 0;
    A[2] = 0;
    A[3] = 0;

    A[4] = 0; //Row 2
    A[5] = aspectRatio / (tFov);
    A[6] = 0;
    A[7] = 0;

    A[8] = 0; //Row 3
    A[9] = 0;
    A[10] = (farPlaneDist + nearPlaneDist) / (farPlaneDist - nearPlaneDist);
    A[11] = -2 * farPlaneDist * nearPlaneDist / (farPlaneDist - nearPlaneDist);

    A[12] = 0; //Row 4
    A[13] = 0;
    A[14] = 1;
    A[15] = 0;
}

