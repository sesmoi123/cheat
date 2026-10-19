#include "Render/Hook.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Cheat.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "Menu/Menu.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace hook {

namespace {

using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
using WndProcFn = LRESULT(__stdcall*)(HWND, UINT, WPARAM, LPARAM);

PresentFn originalPresent = nullptr;
ResizeBuffersFn originalResizeBuffers = nullptr;
WndProcFn originalWndProc = nullptr;

ID3D11Device* d3dDevice = nullptr;
ID3D11DeviceContext* d3dContext = nullptr;
ID3D11RenderTargetView* renderTarget = nullptr;
HWND window = nullptr;

bool initialized = false;
bool menuVisible = true;
bool installed = false;

LRESULT __stdcall hookedWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_KEYDOWN && wparam == static_cast<WPARAM>(cfg().menu_key)) {
        toggle_menu();
        return 0;
    }

    if (menuVisible) {
        ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);

        switch (message) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            return 0;
        default:
            break;
        }
    }

    return CallWindowProcW(originalWndProc, hwnd, message, wparam, lparam);
}

void create_render_target(IDXGISwapChain* swapChain) {
    if (renderTarget) {
        renderTarget->Release();
        renderTarget = nullptr;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer)))) {
        d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &renderTarget);
        backBuffer->Release();
    }
}

bool initialize_overlay(IDXGISwapChain* swapChain) {
    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&d3dDevice)))) {
        return false;
    }

    d3dDevice->GetImmediateContext(&d3dContext);

    DXGI_SWAP_CHAIN_DESC description{};
    swapChain->GetDesc(&description);
    window = description.OutputWindow;

    originalWndProc = reinterpret_cast<WndProcFn>(
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hookedWndProc)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    menu::load_fonts();
    menu::apply_style();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(d3dDevice, d3dContext);

    create_render_target(swapChain);

    initialized = true;
    logger::info("overlay initialise");
    return true;
}

HRESULT __stdcall hookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
    if (!initialized) {
        if (!initialize_overlay(swapChain)) {
            return originalPresent(swapChain, syncInterval, flags);
        }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    cheat::tick();

    ImGui::Render();

    if (d3dContext && renderTarget) {
        d3dContext->OMSetRenderTargets(1, &renderTarget, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return originalPresent(swapChain, syncInterval, flags);
}

HRESULT __stdcall hookedResizeBuffers(IDXGISwapChain* swapChain, UINT bufferCount, UINT width, UINT height,
                                      DXGI_FORMAT format, UINT swapChainFlags) {
    if (renderTarget) {
        renderTarget->Release();
        renderTarget = nullptr;
    }

    const HRESULT result = originalResizeBuffers(swapChain, bufferCount, width, height, format, swapChainFlags);

    if (initialized && d3dContext) {
        create_render_target(swapChain);
    }

    return result;
}

} // namespace

bool install() {
    if (installed) {
        return true;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"RetracOGFNDummy";
    RegisterClassExW(&windowClass);

    HWND dummyWindow = CreateWindowW(windowClass.lpszClassName, L"Retrac OGFN", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                     nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!dummyWindow) {
        logger::error("create dummy window failed");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = 1;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = dummyWindow;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;

    const HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &description,
        &swapChain, &device, &level, &context);

    if (FAILED(result) || !swapChain) {
        logger::error("D3D11CreateDeviceAndSwapChain failed (0x%08X)", static_cast<unsigned>(result));
        DestroyWindow(dummyWindow);
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(swapChain);
    originalPresent = reinterpret_cast<PresentFn>(vtable[8]);
    originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(vtable[13]);

    DWORD protection = 0;
    VirtualProtect(vtable, 16 * sizeof(void*), PAGE_READWRITE, &protection);
    vtable[8] = reinterpret_cast<void*>(&hookedPresent);
    vtable[13] = reinterpret_cast<void*>(&hookedResizeBuffers);
    VirtualProtect(vtable, 16 * sizeof(void*), protection, &protection);

    swapChain->Release();
    context->Release();
    device->Release();
    DestroyWindow(dummyWindow);

    installed = true;
    logger::info("Present/ResizeBuffers hooks installes");
    return true;
}

void uninstall() {
    if (!installed) {
        return;
    }

    if (window && originalWndProc) {
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWndProc));
        originalWndProc = nullptr;
    }

    if (initialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (renderTarget) {
            renderTarget->Release();
            renderTarget = nullptr;
        }
        if (d3dContext) {
            d3dContext->Release();
            d3dContext = nullptr;
        }
        if (d3dDevice) {
            d3dDevice->Release();
            d3dDevice = nullptr;
        }
        initialized = false;
    }

    installed = false;
}

bool menu_visible() {
    return menuVisible;
}

void set_menu_visible(bool value) {
    menuVisible = value;
    if (ImGui::GetCurrentContext()) {
        ImGui::GetIO().MouseDrawCursor = value;
    }
}

void toggle_menu() {
    set_menu_visible(!menuVisible);
}

HWND game_window() {
    return window;
}

ID3D11Device* device() {
    return d3dDevice;
}

ID3D11DeviceContext* context() {
    return d3dContext;
}

} // namespace hook
