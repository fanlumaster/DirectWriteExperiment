#include "WebView2.h"
#include <chrono>
#include <dwmapi.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <windows.h>
#include <winnt.h>
#include <winuser.h>
#include <wrl.h>
#include <wrl/client.h>

#pragma comment(lib, "dwmapi.lib")

static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("WebView sample");

static std::wstring HTMLString = LR"(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Document</title>
  <style>
    body {
      background: transparent;
      /*background-image: url('https://appassets/background.jpg'); */
      /*background-size: cover; */
    }
    div {
      border: 0px solid #ccc;
      background-color: #f9f9f9;
      transition: background-color 0.3s ease; 
      background-position: center;
      transition: background-color 0.3s ease;
    }

    div:hover {
      border: 1px solid #ccc;
      background-color: #ffeb3b;
      border-color: #ffc107;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <div>1. 量子</div>
  <div>2. 毛笔</div>
  <div>3. 什么</div>
  <div>4. 可恶</div>
  <div>5. 竟然</div>
  <div>6. 你说</div>
  <div>7. 好吧</div>
  <div>8. 难道</div>
</body>
</html>
)";

HINSTANCE hInst = 0;

using namespace Microsoft::WRL;

static ICoreWebView2Controller *webviewController = nullptr;
static ICoreWebView2 *webview = nullptr;
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

void MeasureDomUpdateTime(ICoreWebView2 *webview)
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
    // case WM_MOVE:
    //     MeasureDomUpdateTime(webview);
    //     break;
    case WM_DESTROY:
        // Release WebView2 objects
        if (webviewController != nullptr)
        {
            webviewController->Release();
            webviewController = nullptr;
        }
        if (webview != nullptr)
        {
            webview->Release();
            webview = nullptr;
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
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

                            controller->AddRef();
                            webviewController = controller;
                            webviewController->get_CoreWebView2(
                                reinterpret_cast<ICoreWebView2 **>(&webview));

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

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
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

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED,        //
                               szWindowClass,        //
                               L"TransparentWindow", //
                               WS_POPUP,             //
                               100,                  //
                               100,                  //
                               600,                  //
                               337,                  //
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
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    InitWebview(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}