#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <algorithm>
#include <random>

using Microsoft::WRL::ComPtr;

struct Point {
    double x, y;
};

struct MonitorData {
    POINT point;
    int monitorIndex;       // The index of the monitor (for results)
    int targetIndex;        // The display index we're looking for (-1 if looking by point)
    int currentIndex;       // Current enumeration index
    RECT monitorRect;       // The rect of the found monitor
    bool found;             // Whether we found the target monitor
};

struct DisplayInfo {
    RECT rect;
    int index;
    bool isPrimary;
};

class GameManager {
public:
    GameManager();
    ~GameManager();

    bool Capture(cv::Mat& dst);
    bool CaptureDXGI(cv::Mat& dst);
    bool isPaused() const {
        return pause.load();
    }

    bool isRunning() const {
        return running.load();
    }

    void mouseMove(int targetX, int targetY, int displayIndex = -1);
    void mouseScroll(bool isUp, int scrollAmount = -120, int scrollCount = 50);
    void leftClick();
    void pressKey(WORD vkCode, int durationMs = 100);
    void panCameraRight(int durationMs, int speed = 10);

private:
    bool InitializeGDI();
    bool InitializeDXGI();

    void keyTrack();
    std::atomic<bool> running;
    std::atomic<bool> pause;
    std::thread trackingThread;

    HWND hwnd;

    HDC hdcWindow;
    HDC hdcMemDC;
    HBITMAP hbmScreen;
    void* bitmapData;
    BITMAPINFO bmi;
    int lastWidth, lastHeight;

    cv::Mat tempMat;
    cv::Mat bgra2bgrMat;

    //DXGI ZONE
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    ComPtr<IDXGIOutputDuplication> deskDupl;
    ComPtr<ID3D11Texture2D> stagingTexture;
    DXGI_OUTDUPL_DESC outputDesc;

    double randomRange(double min, double max);
    static Point bezierCurve(double t, Point p0, Point p1, Point p2, Point p3);
    void humanMouseMove(int startX, int startY, int targetX, int targetY, int targetDisplayIndex = -1, int startDisplayIndex = -1);

    std::mt19937 rng{ std::random_device{}() };  // random generator

    //DISPLAY ZONE
    DisplayInfo getDisplayInfo(int displayIndex);
    int getCurrentDisplayIndex();
    Point absoluteToRelative(int absoluteX, int absoluteY, int displayIndex);
    Point relativeToAbsolute(int relativeX, int relativeY, int displayIndex);

};

#endif // !GAME_MANAGER_H

