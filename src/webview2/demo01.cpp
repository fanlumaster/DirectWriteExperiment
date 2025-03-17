/*
 * Not working
*/

#include <WebView2.h>
#include <string>
#include <wil/com.h>
#include <windows.h>
#include <winuser.h>
#include <wrl.h>

using namespace Microsoft::WRL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"WebView2Window";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        0, CLASS_NAME, L"WebView2 Example", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void InitializeWebView(HWND hWnd)
{
    wil::com_ptr<ICoreWebView2Environment> webviewEnvironment;
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, //
        nullptr, //
        nullptr, //
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment *env) -> HRESULT {
                // if (FAILED(result))
                // {
                //     std::wstring errorMsg = L"First Failed to create WebView2 "
                //                             L"Environment. HRESULT: " +
                //                             std::to_wstring(result);
                //     MessageBox(hWnd, errorMsg.c_str(), L"Error", MB_OK);
                //     return result;
                // }
                env->CreateCoreWebView2Controller(
                    hWnd,
                    Callback<
                        ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hWnd](HRESULT result,
                               ICoreWebView2Controller *controller) -> HRESULT {
                            if (controller != nullptr)
                            {
                                controller->AddRef();
                            }
                            // if (FAILED(result))
                            // {
                            //     std::wstring errorMsg =
                            //         L"Failed to create WebView2 Controller. "
                            //         L"HRESULT: " +
                            //         std::to_wstring(result);
                            //     MessageBox(hWnd, errorMsg.c_str(), L"Error",
                            //                MB_OK);
                            //     return result;
                            // }
                            wil::com_ptr<ICoreWebView2Controller>
                                webviewController = controller;
                            wil::com_ptr<ICoreWebView2> webview;
                            controller->get_CoreWebView2(&webview);
                            RECT bounds;
                            GetClientRect(hWnd, &bounds);
                            webviewController->put_Bounds(bounds);
                            webview->Navigate(L"https://www.google.com");
                            return S_OK;
                        })
                        .Get());
                return S_OK;
            })
            .Get());

    if (FAILED(hr))
    {
        std::wstring errorMsg =
            L"Failed to create WebView2 Environment. HRESULT: " +
            std::to_wstring(hr);
        MessageBox(hWnd, errorMsg.c_str(), L"Error", MB_OK);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        InitializeWebView(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}