#include "Render/Render.hpp"

#include <windowsx.h>

#include "Config.hpp"
#include "Features/Aimbot.hpp"
#include "Features/BulletTP.hpp"
#include "Features/ESP.hpp"
#include "Input/Pad.hpp"
#include "Logger.hpp"
#include "Menu/Menu.hpp"
#include "SDK/SDK.hpp"

#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace render {

namespace {

using PresentFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

PresentFn g_present = nullptr;
ResizeBuffersFn g_resize = nullptr;
void** g_presentSlot = nullptr;
void** g_resizeSlot = nullptr;

WNDPROC g_wndproc = nullptr;
HWND g_window = nullptr;

ID3D11Device* g_device = nullptr;
ID3D11DeviceContext* g_context = nullptr;
ID3D11RenderTargetView* g_target = nullptr;

bool g_imguiReady = false;
bool g_installed = false;
bool g_previousMenuKeyDown = false;

void release_target() {
    if (g_target) {
        g_target->Release();
        g_target = nullptr;
    }
}

void create_target(IDXGISwapChain* swapchain) {
    if (g_target || !g_device) {
        return;
    }
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                       reinterpret_cast<void**>(&backBuffer))) &&
        backBuffer) {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_target);
        backBuffer->Release();
    }
}

LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    // Anti-repetition : WM_KEYDOWN se declenche en rafale tant que la touche est tenue.
    if (message == WM_KEYDOWN || message == WM_KEYUP) {
        const bool down = (message == WM_KEYDOWN);
        if (static_cast<int>(wparam) == config::g.menu_key) {
            if (down && !g_previousMenuKeyDown) {
                toggle_menu();
            }
            g_previousMenuKeyDown = down;
        }
    }

    if (menu_open()) {
        ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);

        // On n'avale que la souris : le clavier continue d'aller au jeu.
        switch (message) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                return TRUE;
            case WM_MOUSEMOVE:
                if (ImGui::GetIO().WantCaptureMouse) {
                    return TRUE;
                }
                break;
            default:
                break;
        }
    }

    return CallWindowProcW(g_wndproc, window, message, wparam, lparam);
}

bool init_imgui(IDXGISwapChain* swapchain) {
    DXGI_SWAP_CHAIN_DESC description{};
    if (FAILED(swapchain->GetDesc(&description))) {
        return false;
    }

    g_window = description.OutputWindow;

    if (FAILED(swapchain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device))) ||
        !g_device) {
        return false;
    }
    g_device->GetImmediateContext(&g_context);

    create_target(swapchain);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    menu::apply_style();
    menu::load_fonts();

    ImGui_ImplWin32_Init(g_window);
    ImGui_ImplDX11_Init(g_device, g_context);

    g_wndproc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndproc)));

    g_imguiReady = true;
    logger::info("ImGui initialise");
    return true;
}

HRESULT WINAPI present_hook(IDXGISwapChain* swapchain, UINT sync, UINT flags) {
    if (!g_imguiReady) {
        init_imgui(swapchain);
    }

    if (g_imguiReady && g_context && g_target) {
        // Manette : XInput ne genere aucun WM_KEYDOWN, tout passe par la frame.
        pad::update();
        if (config::g.pad_enabled && pad::pressed_combo(config::g.menu_pad_combo)) {
            toggle_menu();
        }

        sdk::update();
        features::esp::tick();
        features::aimbot::tick();
        features::bullet_tp::tick();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        pad::feed_imgui();

        ImGui::GetIO().MouseDrawCursor = config::g.menu_open;

        ImGui::NewFrame();

        menu::draw();

        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_target, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return g_present(swapchain, sync, flags);
}

HRESULT WINAPI resize_hook(IDXGISwapChain* swapchain, UINT count, UINT width, UINT height,
                           DXGI_FORMAT format, UINT flags) {
    if (g_imguiReady) {
        release_target();
    }

    const HRESULT result = g_resize(swapchain, count, width, height, format, flags);

    if (g_imguiReady) {
        create_target(swapchain);
    }
    return result;
}

bool create_dummy_swapchain() {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"RetracDummy";
    RegisterClassExW(&windowClass);

    HWND window = CreateWindowW(windowClass.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100,
                                nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!window) {
        return false;
    }

    DXGI_SWAP_CHAIN_DESC description{};
    description.BufferCount = 1;
    description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.OutputWindow = window;
    description.SampleDesc.Count = 1;
    description.Windowed = TRUE;
    description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

    IDXGISwapChain* swapchain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL obtained{};

    const HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &description,
        &swapchain, &device, &obtained, &context);

    if (FAILED(result) || !swapchain) {
        DestroyWindow(window);
        UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(swapchain);
    g_presentSlot = &vtable[8];
    g_resizeSlot = &vtable[13];

    swapchain->Release();
    device->Release();
    context->Release();
    DestroyWindow(window);
    UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
    return true;
}

bool patch(void** slot, void* hook, void** original) {
    DWORD protection = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection)) {
        return false;
    }
    *original = *slot;
    *slot = hook;
    VirtualProtect(slot, sizeof(void*), protection, &protection);
    return true;
}

} // namespace

bool install() {
    if (g_installed) {
        return true;
    }
    if (!create_dummy_swapchain()) {
        return false;
    }

    if (!patch(g_presentSlot, reinterpret_cast<void*>(present_hook),
               reinterpret_cast<void**>(&g_present))) {
        return false;
    }
    patch(g_resizeSlot, reinterpret_cast<void*>(resize_hook), reinterpret_cast<void**>(&g_resize));

    g_installed = true;
    return true;
}

void uninstall() {
    if (!g_installed) {
        return;
    }

    pad::shutdown();

    if (g_presentSlot && g_present) {
        DWORD protection = 0;
        if (VirtualProtect(g_presentSlot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection)) {
            *g_presentSlot = reinterpret_cast<void*>(g_present);
            VirtualProtect(g_presentSlot, sizeof(void*), protection, &protection);
        }
    }

    if (g_wndproc && g_window) {
        SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_wndproc));
    }

    if (g_imguiReady) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiReady = false;
    }

    release_target();
    if (g_context) {
        g_context->Release();
        g_context = nullptr;
    }
    if (g_device) {
        g_device->Release();
        g_device = nullptr;
    }

    g_installed = false;
}

bool installed() {
    return g_installed;
}

bool menu_open() {
    return config::g.menu_open;
}

void toggle_menu() {
    config::g.menu_open = !config::g.menu_open;
    config::save();
}

} // namespace render
