#include "WebView2.h"
#include <chrono>
#include <dwmapi.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <synchapi.h>
#include <tchar.h>
#include <windows.h>
#include <winnt.h>
#include <winuser.h>
#include <wrl.h>
#include <wrl/client.h>

#pragma comment(lib, "dwmapi.lib")

#define HOTKEY_ID 1

static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("WebView sample");

static std::wstring HTMLString = LR"(
<!DOCTYPE html>
<html lang="zh-CN">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>垂直候选框</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      height: 100vh;
      margin: 0;
      overflow: hidden;
      border-radius: 6px;
      background: transparent;
    }

    .container {
      /* margin-top: 50px;
      margin-left: 50px; */
      margin-top: 0px;
      margin-left: 0px;
      background-color: #202020;
      padding: 2px;
      border-radius: 6px;
      box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.5);
      width: 100px;
      user-select: none;
      border: 2px solid #9b9b9b2e;
    }

    .row {
      justify-content: space-between;
      padding: 2px;
      margin-top: 2px;
    }

    .cand:hover {
      border-radius: 6px;
      background-color: #414141;
    }

    .row-wrapper {
      position: relative;
    }

    /* .cand:hover::before {
      content: "";
      position: absolute;
      left: 0;
      top: 50%;
      transform: translateY(-50%);
      height: 16px;
      width: 3px;
      background: linear-gradient(to bottom, #ff7eb3, #ff758c, #ff5a5f);
      border-radius: 8px;
    } */

    .first {
      background-color: #3e3e3eb9;
      border-radius: 6px;
    }

    .first::before {
      content: "";
      position: absolute;
      left: 0;
      top: 50%;
      transform: translateY(-50%);
      height: 16px;
      width: 3px;
      background: linear-gradient(to bottom, #ff7eb3, #ff758c, #ff5a5f);
      border-radius: 8px;
    }

    .text {
      padding-left: 8px;
      color: #e9e8e8;
    }
  </style>

  </script>
</head>

<body>
  <div class="container">
    <div class="row pinyin">
      <div class="text">ni'uo</div>
    </div>
    <div class="row-wrapper">
      <div class="row cand first">
        <div class="text">1. 你说</div>
      </div>
    </div>
    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">2. 笔画</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">3. 量子</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">4. 牛魔</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">5. 仙人</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">6. 可恨</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">7. 木槿</div>
      </div>
    </div>

    <div class="row-wrapper">
      <div class="row cand">
        <div class="text">8. 无量</div>
      </div>
    </div>
  </div>
</body>

</html>
)";

HINSTANCE hInst = 0;

using namespace Microsoft::WRL;

static ComPtr<ICoreWebView2Controller> webviewController;
static ComPtr<ICoreWebView2> webview;
ComPtr<ICoreWebView2_3> webview3;
ComPtr<ICoreWebView2Controller2> webviewController2;

void LogMessageW(const wchar_t *message)
{
    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    wchar_t timeBuffer[80];
    wcsftime(timeBuffer, sizeof(timeBuffer) / sizeof(wchar_t),
             L"%Y-%m-%d %H:%M:%S", localTime);

    std::wstring logfilePath = L"C:"
                               L"\\Users\\SonnyCalcr\\EDisk\\CppCodes\\Win32Cod"
                               L"es\\DirectWriteExperiment\\log.txt";
    std::wofstream logFile(logfilePath, std::ios_base::app);
    if (logFile.is_open())
    {
        logFile.imbue(std::locale("Chinese_China.65001"));
        logFile << L"[" << timeBuffer << L"] " << message;
        logFile << std::endl;
        logFile.close();
    }
}

void UpdateHtmlContentWithJavaScript(ICoreWebView2 *webview,
                                     const std::wstring &newContent)
{
    if (webview != nullptr)
    {
        std::wstring script =
            L"document.body.innerHTML = `" + newContent + L"`;";
        webview->ExecuteScript(script.c_str(), nullptr);
    }
}

void MeasureDomUpdateTime(ComPtr<ICoreWebView2> webview)
{
    std::wstring script =
        LR"(document.body.innerHTML = '<div>1. 原来</div> <div>2. 如此</div> <div>3. 竟然</div> <div>4. 这样</div> <div>5. 可恶</div> <div>6. 棋盘</div> <div>7. 磨合</div> <div>8. 樱花</div> </body>';)";

    auto start = std::chrono::high_resolution_clock::now();

    webview->ExecuteScript(script.c_str(), nullptr);

    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::wstring message =
        L"DOM update time: " + std::to_wstring(duration.count()) + L" μs";
    LogMessageW(message.c_str());
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                                LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (webviewController != nullptr)
        {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            webviewController->put_Bounds(bounds);

            // Debug: Print window size
            char buffer[100];
            sprintf(buffer, "Width: %ld, Height: %ld",
                    bounds.right - bounds.left, bounds.bottom - bounds.top);
            OutputDebugStringA(buffer);

            MeasureDomUpdateTime(webview);
        };

        break;
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void InitWebview(HWND hWnd)
{
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment *env) -> HRESULT {
                if (result != S_OK)
                {
                    MessageBox(hWnd, L"Failed to create WebView2 environment.",
                               L"Error", MB_OK);
                    return result;
                }

                // Create a CoreWebView2Controller and get the associated
                // CoreWebView2
                env->CreateCoreWebView2Controller(
                    hWnd,
                    Callback<
                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd](HRESULT result,
                               ICoreWebView2Controller *controller) -> HRESULT {
                            if (controller == nullptr || FAILED(result))
                            {
                                MessageBox(
                                    hWnd,
                                    L"Failed to create WebView2 controller.",
                                    L"Error", MB_OK);
                                return E_FAIL;
                            }

                            webviewController = controller;
                            webviewController->get_CoreWebView2(
                                webview.GetAddressOf());

                            if (webview == nullptr)
                            {
                                MessageBox(hWnd,
                                           L"Failed to get WebView2 instance.",
                                           L"Error", MB_OK);
                                return E_FAIL;
                            }

                            // Add a few settings for the webview
                            ICoreWebView2Settings *settings;
                            webview->get_Settings(&settings);
                            settings->put_IsScriptEnabled(TRUE);
                            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                            settings->put_IsWebMessageEnabled(TRUE);
                            settings->put_AreHostObjectsAllowed(TRUE);

                            if (webview->QueryInterface(
                                    IID_PPV_ARGS(&webview3)) == S_OK)
                            {
                                webview3->SetVirtualHostNameToFolderMapping(
                                    L"appassets", // 虚拟主机名
                                    L"C:"
                                    L"\\Users\\SonnyCalcr\\AppData\\Roaming\\Po"
                                    L"tPla"
                                    L"yerMini64\\Capture", // 本地文件夹路径
                                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS // 访问权限
                                );
                            }

                            if (controller->QueryInterface(
                                    IID_PPV_ARGS(&webviewController2)) == S_OK)
                            {
                                COREWEBVIEW2_COLOR backgroundColor = {0, 0, 0,
                                                                      0};
                                webviewController2->put_DefaultBackgroundColor(
                                    backgroundColor);
                            }

                            // Resize WebView to fit the bounds of the parent
                            // window
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            webviewController->put_Bounds(bounds);
                            // Navigate to a simple HTML string
                            auto hr =
                                webview->NavigateToString(HTMLString.c_str());
                            if (FAILED(hr))
                            {
                                MessageBox(hWnd,
                                           L"Failed to navigate to string.",
                                           L"Error", MB_OK);
                            }

                            // webview->OpenDevToolsWindow();

                            return S_OK;
                        })
                        .Get());
                return S_OK;
            })
            .Get());
}

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Register Ctrl + Alt + F12 hotkey
    if (!RegisterHotKey(nullptr, HOTKEY_ID, MOD_CONTROL | MOD_ALT, VK_F12))
    {
        MessageBox(nullptr, L"Cannot Register Hotkey！", L"Error",
                   MB_ICONERROR);
        return 1;
    }

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    // wcex.style =  CS_IME;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"),
                   _T("Windows Desktop Guided Tour"), NULL);

        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW, //
                               szWindowClass,                    //
                               L"TransparentWindow",             //
                               WS_POPUP,                         //
                               100,                              //
                               100,                              //
                               (108 + 15) * 1.5,                 //
                               (246 + 15) * 1.5,                 //
                               nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        MessageBox(NULL, _T("Call to CreateWindow failed!"),
                   _T("Windows Desktop Guided Tour"), NULL);

        return 1;
    }

    // DWM_BLURBEHIND bb = {0};
    // bb.dwFlags = DWM_BB_ENABLE;
    // bb.fEnable = true;
    // bb.hRgnBlur = NULL;
    // DwmEnableBlurBehindWindow(hWnd, &bb);

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    MoveWindow(hWnd, 1000, -1000, (108 + 15) * 1.5, (246 + 15) * 1.5, TRUE);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    InitWebview(hWnd);

    int state = SW_SHOW;

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID)
        {
            ShowWindow(hWnd, state);
            state = state == SW_HIDE ? SW_SHOW : SW_HIDE;
            MoveWindow(hWnd, 100, 100, (108 + 15) * 1.5, (246 + 15) * 1.5,
                       TRUE);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}