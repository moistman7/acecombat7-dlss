#pragma once
#include <d3d11_1.h>
#include <wrl/client.h>

// NGX receives all inputs explicitly. Isolate its pipeline changes from draws
// that follow it, including stages not covered by Luma's partial state stack.
namespace AC7Capture {
class ScopedNGXState {
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context;
    Microsoft::WRL::ComPtr<ID3DDeviceContextState> previous;
public:
    ScopedNGXState(ID3D11DeviceContext* source, ID3DDeviceContextState* isolated) {
        if (isolated && SUCCEEDED(source->QueryInterface(IID_PPV_ARGS(&context))))
            context->SwapDeviceContextState(isolated, &previous);
    }
    ~ScopedNGXState() {
        if (previous) context->SwapDeviceContextState(previous.Get(), nullptr);
    }
    ScopedNGXState(const ScopedNGXState&) = delete;
    ScopedNGXState& operator=(const ScopedNGXState&) = delete;
};
inline Microsoft::WRL::ComPtr<ID3DDeviceContextState> CreateNGXState(ID3D11Device* source) {
    Microsoft::WRL::ComPtr<ID3D11Device1> device;
    Microsoft::WRL::ComPtr<ID3DDeviceContextState> state;
    if (SUCCEEDED(source->QueryInterface(IID_PPV_ARGS(&device)))) {
        auto level = source->GetFeatureLevel();
        device->CreateDeviceContextState(0, &level, 1, D3D11_SDK_VERSION,
            __uuidof(ID3D11Device), nullptr, &state);
    }
    return state;
}
}
