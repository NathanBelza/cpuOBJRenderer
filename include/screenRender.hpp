#ifndef SCREEN_RENDER
#define SCREEN_RENDER

#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <algorithm>

struct point4D {
    float x, y, z, w;
    point4D() {x = 0; y = 0; z = 0; w = 1;}
    point4D(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    point4D(point4D A, point4D B) { //Vector
        x = B.x - A.x;
        y = B.y - A.y;
        z = B.z - A.z;
        w = 1;
    }

    void perspectiveDivide() {
        x /= w;
        y /= w;
        z /= w;
        w = 1;
    }

    void normalize() {
        float div = 1 / sqrt(x * x + y * y + z * z);
        x *= div;
        y *= div;
        z *= div;
    }
};

class Camera {
    private:
    float xPos, yPos, zPos;
    float pitch, yaw, roll; //implement roll later
    float pitchTemp, yawTemp;
    float fov;
    float nearPlaneDist, farPlaneDist;
    point4D cameraViewVec;
    public:
    Camera(float _xPos, float _yPos, float _zPos,
        float _pitch, float _yaw, float _roll,
        float _fov, float _nearPlaneDist, float _farPlaneDist) :
        xPos(_xPos), yPos(_yPos), zPos(_zPos),
        pitch(_pitch * (M_PI/180.0f)), yaw(_yaw * (M_PI/180.0f)), roll(_roll * (M_PI/180.0f)),
        fov(_fov * (M_PI/180.0f)), nearPlaneDist(_nearPlaneDist), farPlaneDist(_farPlaneDist) {

            cameraViewVec.x = cos(pitch) * sin(yaw);
            cameraViewVec.y = sin(pitch);
            cameraViewVec.z = cos(pitch) * cos(yaw);
            cameraViewVec.w = 1;
        }
    
    point4D getPos() {
        point4D cameraPos {
            xPos, yPos, zPos, 1
        };
        return cameraPos;
    }
    float getYawR() const {return yaw;}
    float getPitchR() const {return pitch;}
    float getFovR() const {return fov;}
    float getNear() const {return nearPlaneDist;}
    float getFar() const {return farPlaneDist;}
    point4D getViewVec() const {return cameraViewVec;}
    void setFov(float _fov) {fov = _fov * (M_PI/180.0f);}
    void setNear(float _nearPlaneDist) {nearPlaneDist = _nearPlaneDist;}
    void setFar(float _farPlaneDist) {farPlaneDist = _farPlaneDist;}

    void updateViewAngle(LONG deltaX, LONG deltaY) {
        yawTemp = yaw + -deltaX / 250.0f;
        pitchTemp = pitch + -deltaY / 250.0f;
        yawTemp = fmodf(yawTemp, 2 * M_PI);
        if(yawTemp < 0) yawTemp += 2 * M_PI;
        if(pitchTemp > M_PI_2) pitchTemp = M_PI_2;
        if(pitchTemp < -M_PI_2) pitchTemp = -M_PI_2;
        yaw = yawTemp;
        pitch = pitchTemp;

        cameraViewVec.x = cos(pitch) * sin(yaw);
        cameraViewVec.y = sin(pitch);
        cameraViewVec.z = cos(pitch) * cos(yaw);
        cameraViewVec.w = 1;
    }

    bool updateCameraPos(HWND hWnd, UINT Message, USHORT VKey);
};

class worldTriangle {
    private:
    point4D A, B, C; //Vertecies
    point4D normalVector;
    public:
    worldTriangle(point4D a, point4D b, point4D c) {
        A = a;
        A.w = 1;
        B = b;
        B.w = 1;
        C = c;
        C.w = 1;
        point4D AB(A, B);
        point4D AC(A, C);
        normalVector = _crossProduct(AB, AC);
        normalVector.normalize();
    }
    point4D getAPos() const {return A;}
    point4D getBPos() const {return B;}
    point4D getCPos() const {return C;}
    point4D getNormal() const {return normalVector;}
    private:
    point4D _crossProduct(point4D A, point4D B) {
        point4D C;
        C.x = A.y * B.z - A.z * B.y;
        C.y = A.z * B.x - A.x * B.z;
        C.z = A.x * B.y - A.y * B.x;
        C.w = 1;
        return C;
    }
};

class screenTriangle {
    private:
    point4D A, B, C; //Vertecies
    bool culled;
    int triTop, triBottom, triLeft, triRight;
    float v0x, v0y, v1x, v1y;
    float detInverse;

    public:
    screenTriangle(point4D a, point4D b, point4D c) {
        culled = false;
        A = a;
        B = b;
        C = c;
        triTop = static_cast<int> (floor(std::min({A.y, B.y, C.y})));
        triBottom = static_cast<int> (ceil(std::max({A.y, B.y, C.y})));
        triLeft = static_cast<int> (floor(std::min({A.x, B.x, C.x})));
        triRight = static_cast<int> (ceil(std::max({A.x, B.x, C.x})));

        v0x = B.x - A.x;
        v0y = B.y - A.y;
        v1x = C.x - A.x;
        v1y = C.y - A.y;

        detInverse = 1 / (v0x * v1y - v0y * v1x);
    }
    screenTriangle(worldTriangle &worldTri, Camera &camera, float width, float height, std::array<float, 16> &matrix);

    bool checkPointInTriangle(float x, float y, float &depth) { //Check if a point is in the bounds of the triangle
        float u, v, w;
        _findBarycentric(x, y, u, v, w);

        depth = u * A.z + v * B.z + w * C.z;

        if(u > 0 && v > 0 && w > 0) return true;
        else return false;
    }
    bool isCulled() const {return culled;}
    int getTop() const {return triTop;}
    int getBottom() const {return triBottom;}
    int getLeft() const {return triLeft;}
    int getRight() const {return triRight;}

    private:
    void _findBarycentric(float Px, float Py, float &u, float &v, float &w) {
        float v2x = Px - A.x;
        float v2y = Py - A.y;
        v = detInverse * (v1y * v2x - v1x * v2y);
        w = detInverse * (-v0y * v2x + v0x * v2y);
        u = 1 - v - w;
    }
};

void renderImage(Camera &camera, std::vector<worldTriangle> &triangles, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr, std::vector<float> &depthBuffer);
void OnPaint(HDC hdc, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr, Camera &camera);

#endif