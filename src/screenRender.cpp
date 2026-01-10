#include <windows.h>
#include <gdiplus.h>
#include <array>
#include <vector>
#include <algorithm>
#include "matrices.hpp"
#include "screenRender.hpp"

void clipToScreenSpace(point4D &clipVertex, size_t screenWidth, size_t screenHeight) {
    point4D screenPoint;
    clipVertex.x = screenWidth - (clipVertex.x + 1.0f) * screenWidth * 0.5f;
    clipVertex.y = screenHeight - (clipVertex.y + 1.0f) * screenHeight * 0.5f;
    clipVertex.z = clipVertex.z;
    clipVertex.w = clipVertex.w;
}

bool transformVertexToClip(point4D &vertex, std::array<float, 16> matrix, Camera &camera, float aspectRatio) {
    vertex = matrixVectorMultiply(matrix, vertex);

    if(vertex.x > vertex.w || vertex.x < -vertex.w ||
       vertex.y > vertex.w || vertex.y < -vertex.w ||
       vertex.z > camera.getFar() || vertex.z < -camera.getNear()) {

        return TRUE;
    }

    return FALSE;
}

void OnPaint(HDC hdc, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr, Camera &camera)
{
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Bitmap bitmap(width, height, PixelFormat32bppARGB);
    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0,0,width,height);
    Gdiplus::Status status = bitmap.LockBits(
        &rect,
        Gdiplus::ImageLockModeWrite,
        PixelFormat32bppARGB,
        &bitmapData
    );

    if (status == Gdiplus::Ok) {
        // Get the pointer to the first pixel in the locked memory
        BYTE* pixels = (BYTE*)bitmapData.Scan0;
        INT stride = bitmapData.Stride;
        Gdiplus::ARGB color;

        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                // Calculate the index in the raw byte array
                // Stride is the width of a single row in bytes, which might be more than (width * bytes_per_pixel) due to alignment
                long index = (y * stride) + (x * 4); // 4 bytes per pixel for 32bppARGB

                color = imageArr[x + width * y];

                pixels[index + 0] = (BYTE)((color >> 0) & 0xff);  // Blue
                pixels[index + 1] = (BYTE)((color >> 8) & 0xff);  // Green
                pixels[index + 2] = (BYTE)((color >> 16) & 0xff); // Red
                pixels[index + 3] = (BYTE)((color >> 24) & 0xff); // Alpha
            }
        }

        // Unlock the bitmap data
        bitmap.UnlockBits(&bitmapData);
    }
    graphics.DrawImage(&bitmap, 0, 0);
}

void renderImage(Camera &camera, std::vector<worldTriangle> &triangles, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr, std::vector<float> &depthBuffer) {
    for(size_t y = 0; y < height; y++) {
        for(size_t x = 0; x < width; x++) {
            imageArr[x + y * width] = 0xFF000000;
            depthBuffer[x + y * width] = 1;
        }
    }

    float aspectRatio = static_cast<float> (width) / static_cast<float> (height);

    std::array<float, 16> cameraToOriginM;
    cameraToOrigin(camera, cameraToOriginM);

    std::array<float, 16> cameraRotatePitchM;
    cameraRotatePitch(camera, cameraRotatePitchM);

    std::array<float, 16> cameraRotateYawM;
    cameraRotateYaw(camera, cameraRotateYawM);

    std::array<float, 16> cameraToClipM;
    cameraToClipSpace(camera, aspectRatio, cameraToClipM);

    std::array<float, 16> combinedM;
    combinedM = matrixMultiply(cameraRotateYawM, cameraToOriginM);
    combinedM = matrixMultiply(cameraRotatePitchM, combinedM);
    combinedM = matrixMultiply(cameraToClipM, combinedM);

    for(auto& worldTri : triangles) {
        screenTriangle screenTri(worldTri, camera, width, height, combinedM);
        if(screenTri.isCulled()) continue;
        Gdiplus::ARGB triangleColor = 0xFFFF0000;
        point4D normal = worldTri.getNormal();
        triangleColor = Gdiplus::Color::MakeARGB(
            0xFF,
            0xFF * normal.x,
            0xFF * normal.y,
            0xFF * normal.z
        );

        int triTop = screenTri.getTop();
        int triBottom = screenTri.getBottom();
        int triLeft = screenTri.getLeft();
        int triRight = screenTri.getRight();

        triTop = std::clamp(triTop, 0, static_cast<int> (height-1));
        triBottom = std::clamp(triBottom, 0, static_cast<int> (height-1));
        triLeft = std::clamp(triLeft, 0, static_cast<int> (width-1));
        triRight = std::clamp(triRight, 0, static_cast<int> (width-1));

        for(int y = triTop; y <= triBottom; y++) {
            for(int x = triLeft; x <= triRight; x++) {
                float depth;
                bool pointInTriangle = screenTri.checkPointInTriangle(x + 0.5f, y + 0.5f, depth);
                if(pointInTriangle == TRUE && depth < depthBuffer[x + width * y]) {
                    imageArr[x + width * y] = triangleColor;
                    depthBuffer[x + width * y] = depth;
                }
            }
        }
    }
}

screenTriangle::screenTriangle(worldTriangle &worldTri, Camera &camera, float width, float height, std::array<float, 16> &matrix) {
    point4D normal = worldTri.getNormal();
    point4D cameraVec = camera.getViewVec();

    if((normal.x * cameraVec.x + normal.y * cameraVec.y + normal.z * cameraVec.z) > 0.0f) { //Dot product for backface culling
        culled = TRUE;
        return;
    }

    float aspectRatio = static_cast<float> (width) / static_cast<float> (height);
    this->A = worldTri.getAPos();
    this->B = worldTri.getBPos();
    this->C = worldTri.getCPos();

    bool aClipped = transformVertexToClip(A, matrix, camera, aspectRatio);
    bool bClipped = transformVertexToClip(B, matrix, camera, aspectRatio);
    bool cClipped = transformVertexToClip(C, matrix, camera, aspectRatio);

    if(aClipped  && bClipped && cClipped) {
        culled = TRUE;
        return;
    }
    culled = FALSE;

    A.perspectiveDivide();
    B.perspectiveDivide();
    C.perspectiveDivide();

    clipToScreenSpace(A, width, height);
    clipToScreenSpace(B, width, height);
    clipToScreenSpace(C, width, height);

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

bool Camera::updateCameraPos(HWND hWnd, UINT Message, USHORT VKey) {
    if(Message != WM_KEYDOWN) return FALSE;
    switch(VKey) {
        case VK_SPACE:
            yPos += 0.1f;
            return TRUE;
        case VK_SHIFT:
            yPos += -0.1f;
            return TRUE;
        case 0x57: //W
            zPos += 0.1f * cos(yaw); //add units in camera yaw direction
            xPos += 0.1f * sin(yaw);
            return TRUE;
        case 0x53: //S
            zPos += 0.1f * -cos(yaw); //subtract units in camera yaw direction
            xPos += 0.1f * -sin(yaw);
            return TRUE;
        case 0x41: //A
            zPos += 0.1f * -sin(yaw);
            xPos += 0.1f * cos(yaw);
            return TRUE;
        case 0x44: //D
            zPos += 0.1f * sin(yaw);
            xPos += 0.1f * -cos(yaw);
            return TRUE;
        case VK_ESCAPE:
            {
            CURSORINFO cursorInfo;
            cursorInfo.cbSize = sizeof(cursorInfo);
            RECT rect;
            if(GetCursorInfo(&cursorInfo) && GetClientRect(hWnd, &rect)) {
                POINT pointTL, pointBR;
                pointTL.x = rect.left;
                pointTL.y = rect.top;
                pointBR.x = rect.right;
                pointBR.y = rect.bottom;
                    
                ClientToScreen(hWnd, &pointTL);
                ClientToScreen(hWnd, &pointBR);

                rect.left = pointTL.x;
                rect.top = pointTL.y;
                rect.right = pointBR.x;
                rect.bottom = pointBR.y;

                if(cursorInfo.flags == CURSOR_SHOWING) {
                    ShowCursor(FALSE);
                    ClipCursor(&rect);
                } else {
                    ShowCursor(TRUE);
                    SetCursorPos((rect.right + rect.left) / 2, (rect.bottom - rect.top) / 2);
                    ClipCursor(NULL);
                }
            }
            }
            return TRUE;
    }
    return FALSE;
}