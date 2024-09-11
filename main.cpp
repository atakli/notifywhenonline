#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <fstream>
// #include <algorithm>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"User32.lib")
#pragma comment (lib,"Gdi32.lib")
#pragma comment (lib,"Kernel32.lib")

int monitorIndex = 0;  // Start with 0 for the first monitor
HMONITOR hMonitor = NULL;

const std::wstring person = L"Person";                              // whatsapta gorunen kisi ismi
const unsigned int interval = 60;                                   // saniye cinsinden kontrol edilme sikligi
const std::string imagePath = "whatsapp_screenshot.png";

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitorEnum, HDC hdcMonitorEnum, LPRECT lprcMonitorEnum, LPARAM dwData)
{
    // if (monitorIndex == 1) {  // Change this index for different monitors
    //     hMonitor = hMonitorEnum;
    //     return FALSE;  // Stop enumeration
    // }
    // monitorIndex++;
    // return TRUE;  // Continue enumeration
    if (monitorIndex == 1) {  // Change this index for different monitors
        hMonitor = hMonitorEnum;
        std::cout << "monitorIndex 1: " << monitorIndex << std::endl;
        return FALSE;  // Stop enumeration
    }
    else
    {
        std::cout << "monitorIndex 0: " << monitorIndex << std::endl;
    }
    monitorIndex++;
    return TRUE;  // Continue enumeration
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    std::vector<HWND>& handles = *reinterpret_cast<std::vector<HWND>*>(lParam);
    wchar_t title[256];
    GetWindowText(hwnd, title, sizeof(title) / sizeof(wchar_t));
    if (std::wstring(title).find(L"WhatsApp") != std::wstring::npos) {
        handles.push_back(hwnd);
    }
    return TRUE;
}
std::string runTesseract()
{
    // Construct the command to run Tesseract
    std::string command = "tesseract " + imagePath + " " + "output";

    // Run the command
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Error running Tesseract command." << std::endl;
        return "";
    }

    std::ifstream outputFile("output.txt");
    if (!outputFile.is_open())
    {
        // std::cout << "outputFile: " << outputFile. << std::endl;
        std::cerr << "Could not open output file." << std::endl;
        return "";
    }

    std::stringstream textStream;
    textStream << outputFile.rdbuf();
    outputFile.close();

    return textStream.str();
}
struct Time
{
    int hour;
    int min;
};
bool is_difference_small(int min1, int min2)
{
    return std::abs(min1 - min2) < 3;
}
Time get_time()
{
    auto now = std::chrono::system_clock::now();
    const std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&time_t_now);
    // return std::format("{:02d}:{:02d}", local_time->tm_hour, local_time->tm_min);
    return {local_time->tm_hour, local_time->tm_min};
}
bool is_time_close(const std::string& time)
{
    // if (time == "online")
    const Time now = get_time();
    const auto last_seen_hour = std::string_view{time.begin(), time.begin() + 2};
    if (last_seen_hour != std::to_string(now.hour) && (now.hour - std::atoi(last_seen_hour.data()) != 1))
        return false;

    return is_difference_small(std::atoi(time.substr(3, 5).c_str()) , now.min);
}
void notify_online(const std::wstring& info)
{
    MessageBox(NULL, info.c_str(), person.c_str(), MB_OK | MB_TOPMOST | MB_SETFOREGROUND);
    HWND hwnd = GetActiveWindow();
    if (hwnd != NULL)
    {
        ShowWindow(hwnd, SW_SHOW);
        SetForegroundWindow(hwnd);
    }
}
DWORD WINAPI ShowMessageBox(LPVOID lpParam)
{
    HWND hwnd = GetConsoleWindow(); // Get the handle of the console window
    int msgBoxID = MessageBoxA(NULL, "This is a message box", "Title", MB_OK | MB_TOPMOST | MB_SETFOREGROUND);

    // Force the message box to be in the foreground
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);

    return 0;
}
int main()
{
    // Get the screen device context for the second monitor
    HDC hdcScreen = GetDC(NULL);
    // HDC hdcScreen = GetDC(hwnd);
    std::cout << "hdcScreen: " << hdcScreen << std::endl;
    HDC hdcMonitor = CreateCompatibleDC(hdcScreen);

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    if (!hMonitor)
    {
        std::cerr << "hMonitor is null however" << std::endl;
        ReleaseDC(NULL, hdcScreen);
        exit(EXIT_FAILURE);
    }
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &mi);

    RECT rc = mi.rcMonitor;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ oldBitmap = SelectObject(hdcMonitor, hBitmap);

    auto TakeScreenshot = [&]()
    {
        // Use the monitor's DC
        BitBlt(hdcMonitor, 0, 0, width, height, hdcScreen, rc.left, rc.top, SRCCOPY);

        Gdiplus::Bitmap bitmap(hBitmap, NULL);
        CLSID clsid;
        GetEncoderClsid(L"image/png", &clsid);
        Gdiplus::Status status = bitmap.Save(L"whatsapp_screenshot.png", &clsid, NULL);
    };

    while (1)
    {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        std::vector<HWND> handles;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&handles));

        if (handles.empty()) {
            std::cerr << "No WhatsApp window found." << std::endl;
            Gdiplus::GdiplusShutdown(gdiplusToken);
            return 1;
        }
        // std::cout << "handles.size(): " << handles.size() << std::endl;
        HWND hwnd = handles.size() > 1 ? handles[1] : handles[0];
        // HWND hwnd = handles[0];
        // HWND hwnd = handles[1];

        ShowWindow(hwnd, SW_MAXIMIZE);
        SetForegroundWindow(hwnd);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        TakeScreenshot();

        ShowWindow(hwnd, SW_MINIMIZE);

        // Shutdown GDI+
        Gdiplus::GdiplusShutdown(gdiplusToken);

        const std::string extractedText = runTesseract();

        if (extractedText.find("online") != std::string::npos || extractedText.find("typing") != std::string::npos)
        {
            std::cout << "online" << std::endl;
            notify_online(L"online!");
            exit(EXIT_SUCCESS);
        }

        auto pos = extractedText.find("last seen today at ");

        const std::string last_seen_time = [&]
        {
            if (pos != std::string::npos)
            {
                return std::string{extractedText.begin() + pos + 19, extractedText.begin() + pos + 19 + 5};
            }
            else
            {
                std::cerr << "whatsapp ekran goruntusunde bir sorun var. son gorulme gorunmuyor" << std::endl;
                exit(EXIT_FAILURE);
            }
        }();

        std::cout << "last_seen_time: " << last_seen_time << std::endl;
        // last_seen_time = "17:02";
        if (is_time_close(last_seen_time))
        {
            // std::jthread t1{notify_online, L"last seen recently!"};
            notify_online(L"last seen recently!");
            exit(EXIT_SUCCESS);
        }
        std::this_thread::sleep_for(std::chrono::seconds(interval));
    }
    SelectObject(hdcMonitor, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMonitor);
    ReleaseDC(NULL, hdcScreen);
    return 0;
}
