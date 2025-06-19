#include "GameManager.h"

//helper func
template <typename T>
T clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorData* data = reinterpret_cast<MonitorData*>(dwData);

    // If we're looking for a specific display index
    if (data->targetIndex >= 0) {
        if (data->currentIndex == data->targetIndex) {
            data->monitorRect = *lprcMonitor;
            data->monitorIndex = data->currentIndex;
            data->found = true;
            return FALSE; // Stop enumeration
        }
        data->currentIndex++;
        return TRUE; // Continue enumeration
    }

    // If we're looking for the display containing a point
    if (PtInRect(lprcMonitor, data->point)) {
        data->monitorRect = *lprcMonitor;
        data->monitorIndex = data->currentIndex;
        data->found = true;
        return FALSE; // Stop enumeration
    }

    data->currentIndex++;
    return TRUE; // Continue enumeration
}

GameManager::GameManager() : lastWidth(0), lastHeight(0), hdcWindow(nullptr), hdcMemDC(nullptr), hbmScreen(nullptr), bitmapData(nullptr) {
    hwnd = FindWindow(NULL, L"Roblox");
    if (!hwnd) {
        std::cerr << "Roblox window not found\n";
    }

    running.store(true);
    pause.store(false);
    InitializeGDI();
    InitializeDXGI();
    trackingThread = std::thread(&GameManager::keyTrack, this);
}

GameManager::~GameManager() {
    running = false;

    if (trackingThread.joinable())
        trackingThread.join();

}

bool GameManager::InitializeGDI() {
    hdcWindow = GetDC(hwnd);
    if (!hdcWindow) return false;

    hdcMemDC = CreateCompatibleDC(hdcWindow);
    if (!hdcMemDC) {
        ReleaseDC(hwnd, hdcWindow);
        return false;
    }

    // Setup DIB for direct memory access
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;

    return true;
}

bool GameManager::InitializeDXGI() {
    HRESULT hr;

    // Create device with hardware acceleration
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_SINGLETHREADED, // Single-threaded for better performance
        nullptr, 0, D3D11_SDK_VERSION,
        &d3dDevice, &featureLevel, &d3dContext);

    if (FAILED(hr)) return false;

    // Get DXGI interfaces
    ComPtr<IDXGIDevice> dxgiDevice;
    d3dDevice.As(&dxgiDevice);
    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(&dxgiAdapter);
    ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    ComPtr<IDXGIOutput1> dxgiOutput1;
    dxgiOutput.As(&dxgiOutput1);

    // Create desktop duplication
    hr = dxgiOutput1->DuplicateOutput(d3dDevice.Get(), &deskDupl);
    if (FAILED(hr)) return false;

    deskDupl->GetDesc(&outputDesc);

    // Create staging texture for CPU access
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width = outputDesc.ModeDesc.Width;
    stagingDesc.Height = outputDesc.ModeDesc.Height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    hr = d3dDevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    return SUCCEEDED(hr);
}

bool GameManager::Capture(cv::Mat& dst) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Recreate bitmap only if size changed
    if (width != lastWidth || height != lastHeight) {
        if (hbmScreen) DeleteObject(hbmScreen);

        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // Top-down DIB

        hbmScreen = CreateDIBSection(hdcMemDC, &bmi, DIB_RGB_COLORS, &bitmapData, NULL, 0);
        if (!hbmScreen) return false;

        SelectObject(hdcMemDC, hbmScreen);
        lastWidth = width;
        lastHeight = height;

        // Pre-allocate Mat
        tempMat = cv::Mat(height, width, CV_8UC4, bitmapData);
    }

    if (!BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
        return false;
    }

    if (dst.rows != height || dst.cols != width || dst.type() != CV_8UC3) {
        dst = cv::Mat(height, width, CV_8UC3);
    }

    cv::cvtColor(tempMat, dst, cv::COLOR_BGRA2BGR);
    return true;
}

bool GameManager::CaptureDXGI(cv::Mat& dst) {
    HRESULT hr;
    ComPtr<IDXGIResource> deskRes;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;

    // Get new frame
    hr = deskDupl->AcquireNextFrame(0, &frameInfo, &deskRes);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return false; // No new frame
    }
    if (FAILED(hr)) {
        return false;
    }

    // Get texture
    ComPtr<ID3D11Texture2D> deskTexture;
    deskRes.As(&deskTexture);

    // Copy to staging texture
    d3dContext->CopyResource(stagingTexture.Get(), deskTexture.Get());

    // Map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = d3dContext->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        deskDupl->ReleaseFrame();
        return false;
    }

    // Get window position and size
    RECT windowRect, clientRect;
    GetWindowRect(hwnd, &windowRect);
    GetClientRect(hwnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // Calculate window position on screen
    POINT clientPoint = { 0, 0 };
    ClientToScreen(hwnd, &clientPoint);

    // Create Mat directly from mapped data (region of interest)
    int startX = std::max((LONG)0, clientPoint.x);
    int startY = std::max((LONG)0, clientPoint.y);

    // Ensure we have enough space in destination
    if (dst.rows != height || dst.cols != width || dst.type() != CV_8UC3) {
        dst = cv::Mat(height, width, CV_8UC3);
    }

    // Copy data efficiently
    uint8_t* src = (uint8_t*)mappedResource.pData;
    src += startY * mappedResource.RowPitch + startX * 4; // BGRA format

    for (int y = 0; y < height; ++y) {
        uint8_t* srcRow = src + y * mappedResource.RowPitch;
        uint8_t* dstRow = dst.ptr<uint8_t>(y);

        for (int x = 0; x < width; ++x) {
            // Convert BGRA to BGR
            dstRow[x * 3] = srcRow[x * 4];     // B
            dstRow[x * 3 + 1] = srcRow[x * 4 + 1]; // G  
            dstRow[x * 3 + 2] = srcRow[x * 4 + 2]; // R
        }
    }

    d3dContext->Unmap(stagingTexture.Get(), 0);
    deskDupl->ReleaseFrame();
    return true;
}

DisplayInfo GameManager::getDisplayInfo(int displayIndex) {
    DisplayInfo displayInfo = {};
    MonitorData data = {};
    data.targetIndex = displayIndex;
    data.currentIndex = 0;
    data.monitorIndex = 0;
    data.found = false;
    ZeroMemory(&data.monitorRect, sizeof(RECT));

    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

    if (data.found) {
        displayInfo.rect = data.monitorRect;
        displayInfo.index = displayIndex;
        displayInfo.isPrimary = (data.monitorRect.left == 0 && data.monitorRect.top == 0);
    }
    else {
        // Fallback to primary display if index not found
        displayInfo.rect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        displayInfo.index = 0;
        displayInfo.isPrimary = true;
    }

    return displayInfo;
}

double GameManager::randomRange(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

Point GameManager::bezierCurve(double t, Point p0, Point p1, Point p2, Point p3) {
    double u = 1.0 - t;
    double tt = t * t;
    double uu = u * u;
    double uuu = uu * u;
    double ttt = tt * t;

    Point point;
    point.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    point.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;

    return point;
}

void GameManager::humanMouseMove(int startX, int startY, int targetX, int targetY, int targetDisplayIndex, int startDisplayIndex) {
    // Get display information for the target display and convert relative to absolute
    if (targetDisplayIndex >= 0) {
        DisplayInfo targetDisplay = getDisplayInfo(targetDisplayIndex);
        // Convert relative coordinates to absolute screen coordinates
        targetX += targetDisplay.rect.left;
        targetY += targetDisplay.rect.top;

    }

    // Handle start display if specified (useful for future enhancements)
    if (startDisplayIndex >= 0) {
        DisplayInfo startDisplay = getDisplayInfo(startDisplayIndex);
    }

    // Get screen metrics (total desktop area)
    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

    // Starting and ending points
    Point start = { static_cast<double>(startX), static_cast<double>(startY) };
    Point end = { static_cast<double>(targetX), static_cast<double>(targetY) };

    // Calculate distance
    double distance = sqrt(pow(end.x - start.x, 2) + pow(end.y - start.y, 2));

    // Determine number of steps based on distance
    int steps = static_cast<int>(std::min(std::max(distance / 10.0, 10.0), 50.0));

    // Add some randomness to control points for natural curves
    double offsetFactor = randomRange(0.3, 0.5);
    double midPointOffset = distance * offsetFactor;

    // Define control points for the Bezier curve
    Point control1, control2;

    // Randomize control points to create natural arcs
    control1.x = start.x + (end.x - start.x) * randomRange(0.2, 0.4) + randomRange(-midPointOffset, midPointOffset);
    control1.y = start.y + (end.y - start.y) * randomRange(0.2, 0.4) + randomRange(-midPointOffset, midPointOffset);

    control2.x = start.x + (end.x - start.x) * randomRange(0.6, 0.8) + randomRange(-midPointOffset, midPointOffset);
    control2.y = start.y + (end.y - start.y) * randomRange(0.6, 0.8) + randomRange(-midPointOffset, midPointOffset);

    // Variable speed (slower at start and end, faster in the middle)
    for (int i = 0; i <= steps; i++) {
        // Use a non-linear parameter to simulate acceleration/deceleration
        double t = static_cast<double>(i) / steps;

        // Apply easing function for more natural acceleration/deceleration
        double easedT = t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;

        // Calculate point on Bezier curve
        Point point = bezierCurve(easedT, start, control1, control2, end);

        // Ensure point stays within virtual screen bounds
        point.x = std::max(static_cast<double>(screenLeft),
            std::min(point.x, static_cast<double>(screenLeft + screenWidth - 1)));
        point.y = std::max(static_cast<double>(screenTop),
            std::min(point.y, static_cast<double>(screenTop + screenHeight - 1)));

        // Convert to normalized coordinates (0-65535) relative to virtual screen
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
        input.mi.dx = static_cast<LONG>(((point.x - screenLeft) * 65535) / screenWidth);
        input.mi.dy = static_cast<LONG>(((point.y - screenTop) * 65535) / screenHeight);

        SendInput(1, &input, sizeof(INPUT));

        // Random delay between movements (variable speed)
        int delay;
        if (i < steps * 0.2 || i > steps * 0.8) {
            // Slower at beginning and end
            delay = static_cast<int>(randomRange(100, 110));
        }
        else {
            // Faster in the middle
            delay = static_cast<int>(randomRange(80, 100));
        }
        std::this_thread::sleep_for(std::chrono::nanoseconds(delay));
    }
}

void GameManager::mouseMove(int targetX, int targetY, int displayIndex) {
    POINT currentPos;
    GetCursorPos(&currentPos);

    int currentDisplayIndex = getCurrentDisplayIndex();

    humanMouseMove(currentPos.x, currentPos.y, targetX, targetY, displayIndex, currentDisplayIndex);
}

void GameManager::mouseScroll(bool isUp, int scrollAmount, int scrollCount) {
    int multiplier = isUp ? 1 : -1;

    for (int i = 0; i < scrollCount; ++i) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, scrollAmount * multiplier, 0);
        Sleep(10);
    }

    Sleep(100);
}

int GameManager::getCurrentDisplayIndex() {
    POINT p;
    GetCursorPos(&p);
    MonitorData data = {};
    data.point = p;
    data.targetIndex = -1; // We're looking for point, not index
    data.currentIndex = 0;
    data.found = false;
    data.monitorIndex = 0;
    ZeroMemory(&data.monitorRect, sizeof(RECT));

    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

    return data.found ? data.monitorIndex : 0;
}

Point GameManager::absoluteToRelative(int absoluteX, int absoluteY, int displayIndex) {
    DisplayInfo display = getDisplayInfo(displayIndex);
    Point relative;
    relative.x = absoluteX - display.rect.left;
    relative.y = absoluteY - display.rect.top;
    return relative;
}

Point GameManager::relativeToAbsolute(int relativeX, int relativeY, int displayIndex) {
    DisplayInfo display = getDisplayInfo(displayIndex);
    Point absolute;
    absolute.x = relativeX + display.rect.left;
    absolute.y = relativeY + display.rect.top;
    return absolute;
}

void GameManager::keyTrack() {
    while (isRunning()) {
        if (GetAsyncKeyState('S') & 0x8000) {
            POINT p;
            GetCursorPos(&p);
            MonitorData data = {};
            data.point = p;
            data.targetIndex = -1;
            data.currentIndex = 0;
            data.monitorIndex = 0;
            data.found = false;
            ZeroMemory(&data.monitorRect, sizeof(RECT));

            EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

            if (data.found) {
                int relativeX = p.x - data.monitorRect.left;
                int relativeY = p.y - data.monitorRect.top;

                std::cout << "(x, y, display): " << "(" << relativeX << ", " << relativeY << ", " << (data.monitorIndex) << ")" << '\n';
            }
            else {
                std::cout << "Mouse position not found on any display!" << "\n";
            }

            Sleep(200);
        }
    }
}

void GameManager::leftClick() {

    for (int i = 0; i < 1; i++) {
        INPUT input[2] = {};
        // Prepare separate down and up events
        INPUT inputDown = {};
        inputDown.type = INPUT_MOUSE;
        inputDown.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &inputDown, sizeof(INPUT));
        Sleep(100);  // Optional delay to simulate a real click
        INPUT inputUp = {};
        inputUp.type = INPUT_MOUSE;
        inputUp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &inputUp, sizeof(INPUT));
    }

    Sleep(100);
}

void GameManager::pressKey(WORD vkCode, int durationMs) {
    SHORT key;
    UINT mappedkey;
    INPUT input = { 0 };
    key = VkKeyScan(vkCode);
    mappedkey = MapVirtualKey(LOBYTE(key), 0);
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.wScan = mappedkey;
    SendInput(1, &input, sizeof(input));
    Sleep(durationMs);
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(input));
}

void GameManager::panCameraRight(int durationMs, int speed) {
    INPUT inputs[2] = {};

    // 1. Press and hold the right mouse button
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &inputs[0], sizeof(INPUT));

    // 2. Move mouse to the right repeatedly
    int steps = durationMs / 10; // how many steps
    for (int i = 0; i < steps; ++i) {
        INPUT move = {};
        move.type = INPUT_MOUSE;
        move.mi.dx = -speed;  // move right
        move.mi.dy = 0;
        move.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &move, sizeof(INPUT));
        Sleep(10); // small delay between moves
    }

    // 3. Release the right mouse button
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &inputs[1], sizeof(INPUT));
}