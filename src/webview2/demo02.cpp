/*
 * Work with WebView2
 */
#include "WebView2.h"
#include <dwmapi.h>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <windows.h>
#include <wrl.h>

#pragma comment(lib, "dwmapi.lib")

static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("WebView sample");

HINSTANCE hInst = 0;

using namespace Microsoft::WRL;

static ICoreWebView2Controller *webviewController = nullptr;
static ICoreWebView2 *webview = nullptr;

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
        };
        break;
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

    // The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd =
        CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                     CW_USEDEFAULT, 1200, 900, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        MessageBox(NULL, _T("Call to CreateWindow failed!"),
                   _T("Windows Desktop Guided Tour"), NULL);

        return 1;
    }

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(             //
        hWnd,                          //
        DWMWA_USE_IMMERSIVE_DARK_MODE, //
        &useDarkMode,                  //
        sizeof(useDarkMode)            //
    );

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment *env) -> HRESULT {
                // Create a CoreWebView2Controller and get the associated
                // CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(
                    hWnd,
                    Callback<
                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd](HRESULT result,
                               ICoreWebView2Controller *controller) -> HRESULT {
                            if (controller != nullptr)
                            {
                                controller->AddRef();
                                webviewController = controller;
                                webviewController->get_CoreWebView2(&webview);
                            }

                            // Add a few settings for the webview
                            // The demo step is redundant since the values are
                            // the default settings
                            ICoreWebView2Settings *settings;
                            webview->get_Settings(&settings);
                            settings->put_IsScriptEnabled(TRUE);
                            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                            settings->put_IsWebMessageEnabled(TRUE);
                            settings->Release();

                            // Resize WebView to fit the bounds of the parent
                            // window
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            webviewController->put_Bounds(bounds);

                            // Schedule an async task to navigate to Bing
                            // auto hr = webview->Navigate(
                            //     L"https://fanlumaster.github.io/");
                            auto hr = webview->NavigateToString(
                                L"<div>fjalshglasjlgfhaslkgjaslkdfhalshg</div>");

                            // <NavigationEvents>
                            // Step 4 - Navigation events
                            // register an
                            // ICoreWebView2NavigationStartingEventHandler to
                            // cancel any non-https navigation
                            EventRegistrationToken token;
                            webview->add_NavigationStarting(
                                Callback<
                                    ICoreWebView2NavigationStartingEventHandler>(
                                    [](ICoreWebView2 *webview,
                                       ICoreWebView2NavigationStartingEventArgs
                                           *args) -> HRESULT {
                                        wchar_t *uri;
                                        args->get_Uri(&uri);
                                        std::wstring source(uri);
                                        // CoTaskMemFree(uri);
                                        if (source.substr(0, 5) != L"https")
                                        {
                                            args->put_Cancel(true);
                                        }
                                        return S_OK;
                                    })
                                    .Get(),
                                &token);
                            // </NavigationEvents>

                            // <Scripting>
                            // Step 5 - Scripting
                            // Schedule an async task to add initialization
                            // script that freezes the Object object
                            webview->AddScriptToExecuteOnDocumentCreated(
                                L"Object.freeze(Object);", nullptr);
                            // Schedule an async task to get the document URL
                            webview->ExecuteScript(
                                L"window.document.URL;",
                                Callback<
                                    ICoreWebView2ExecuteScriptCompletedHandler>(
                                    [](HRESULT errorCode,
                                       LPCWSTR resultObjectAsJson) -> HRESULT {
                                        LPCWSTR URL = resultObjectAsJson;
                                        // doSomethingWithURL(URL);
                                        return S_OK;
                                    })
                                    .Get());
                            // </Scripting>

                            // <CommunicationHostWeb>
                            // Step 6 - Communication between host and web
                            // content Set an event handler for the host to
                            // return received message back to the web content
                            webview->add_WebMessageReceived(
                                Callback<
                                    ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2 *webview,
                                       ICoreWebView2WebMessageReceivedEventArgs
                                           *args) -> HRESULT {
                                        wchar_t *message;
                                        args->TryGetWebMessageAsString(
                                            &message);
                                        // processMessage(&message);
                                        webview->PostWebMessageAsString(
                                            message);
                                        // CoTaskMemFree(message);
                                        return S_OK;
                                    })
                                    .Get(),
                                &token);

                            // Schedule an async task to add initialization
                            // script that 1) Add an listener to print message
                            // from the host 2) Post document URL to the host
                            webview->AddScriptToExecuteOnDocumentCreated(
                                L"window.chrome.webview.addEventListener("
                                L"\'message\', event => "
                                L"alert(event.data));"
                                L"window.chrome.webview.postMessage(window."
                                L"document.URL);",
                                nullptr);
                            // </CommunicationHostWeb>

                            return S_OK;
                        })
                        .Get());
                return S_OK;
            })
            .Get());
    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}