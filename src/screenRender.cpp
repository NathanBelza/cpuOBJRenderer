#include <windows.h>
#include <gdiplus.h>
#include <array>
#include <vector>
#include "matrices.hpp"
#include "screenRender.hpp"

#include <iostream>

point4D clipToScreenSpace(point4D clipVertex, size_t screenWidth, size_t screenHeight) {
    point4D screenPoint;
    screenPoint.x = screenWidth - (clipVertex.x + 1.0f) * screenWidth * 0.5f;
    screenPoint.y = screenHeight - (clipVertex.y + 1.0f) * screenHeight * 0.5f;
    screenPoint.z = clipVertex.z;
    screenPoint.w = clipVertex.w;
    return screenPoint;
}

bool transformVertexToClip(point4D &vertex, Camera &camera, float aspectRatio) {
    std::array<float, 16> cameraToOriginM;
    cameraToOrigin(camera, cameraToOriginM);

    std::array<float, 16> cameraRotatePitchM;
    cameraRotatePitch(camera, cameraRotatePitchM);

    std::array<float, 16> cameraRotateYawM;
    cameraRotateYaw(camera, cameraRotateYawM);

    std::array<float, 16> cameraToClipM;
    cameraToClipSpace(camera, aspectRatio, cameraToClipM);

    vertex = matrixVectorMultiply(cameraToOriginM, vertex);

    vertex = matrixVectorMultiply(cameraRotateYawM, vertex);

    vertex = matrixVectorMultiply(cameraRotatePitchM, vertex);

    vertex = matrixVectorMultiply(cameraToClipM, vertex);

    if(vertex.x > vertex.w || vertex.x < -vertex.w ||
       vertex.y > vertex.w || vertex.y < -vertex.w ||
       vertex.z > camera.getFar() || vertex.z < -camera.getNear()) {

        return TRUE;
    }

    return FALSE;
}

void OnPaint(HDC hdc, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr)
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

void renderImage(Camera camera, std::vector<worldTriangle> triangles, size_t width, size_t height, std::vector<Gdiplus::ARGB> &imageArr, std::vector<float> &depthBuffer) {
    for(size_t y = 0; y < height; y++) {
        for(size_t x = 0; x < width; x++) {
            imageArr[x + y * width] = 0xFFFFFFFF;
            depthBuffer[x + y * width] = 1;
        }
    }

    for(auto& worldTri : triangles) {
        screenTriangle screenTri(worldTri, camera, width, height);
        if(screenTri.isCulled()) continue;
        Gdiplus::ARGB triangleColor = 0xFFFF0000;
        point4D normal = worldTri.getNormal();
        triangleColor = Gdiplus::Color::MakeARGB(
            0xFF,
            normal.x * 255,
            normal.y * 255,
            normal.z
        );
        
        for(size_t y = 0; y < height; y++) {
            for(size_t x = 0; x < width; x++) {
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

screenTriangle::screenTriangle(worldTriangle worldTri, Camera camera, float width, float height) {
    point4D normal = worldTri.getNormal();
    point4D cameraVec = camera.getViewVec();

    //std::cout << cameraVec.x << ',' << cameraVec.y << ',' << cameraVec.z << '\n' << '\n';
    //std::cout << normal.x * cameraVec.x + normal.y * cameraVec.y + normal.z * cameraVec.z << '\n' << '\n';

    if((normal.x * cameraVec.x + normal.y * cameraVec.y + normal.z * cameraVec.z) > 0.0f) { //Dot product for backface culling
        culled = true;
        return;
    }

    float aspectRatio = static_cast<float> (width / height);
    point4D wA = worldTri.getAPos();
    point4D wB = worldTri.getBPos();
    point4D wC = worldTri.getCPos();

    bool aClipped = transformVertexToClip(wA, camera, aspectRatio);
    bool bClipped = transformVertexToClip(wB, camera, aspectRatio);
    bool cClipped = transformVertexToClip(wC, camera, aspectRatio);

    if(aClipped  && bClipped && cClipped) {
        culled = true;
        return;
    }
    culled = false;

    wA.perspectiveDivide();
    wB.perspectiveDivide();
    wC.perspectiveDivide();

    A = clipToScreenSpace(wA, width, height);
    B = clipToScreenSpace(wB, width, height);
    C = clipToScreenSpace(wC, width, height);

    AVector.x = B.x - A.x; //Create vectors around edges of triangle
    AVector.y = B.y - A.y;
    BVector.x = C.x - B.x;
    BVector.y = C.y - B.y;
    CVector.x = A.x - C.x;
    CVector.y = A.y - C.y;
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