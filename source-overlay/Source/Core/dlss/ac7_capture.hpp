#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <cstdint>

// Capture at the exact NGX evaluation boundary. No texture substitutions or
// changes to DLSS parameters. Only active on a foreground F8 rising edge, or
// when AC7-DLSS-Captures/capture.request exists. Two evaluations per request.
namespace AC7Capture {
namespace fs = std::filesystem;
inline std::mutex capture_mutex;
inline bool key_down = false;
inline unsigned remaining = 0, request_count = 0, evaluation = 0;
inline ULONGLONG last_poll = 0;
inline fs::path session;
inline fs::path Root() {
    wchar_t exe[32768] = {};
    GetModuleFileNameW(nullptr, exe, 32768);
    return fs::path(exe).parent_path() / L"AC7-DLSS-Captures";
}
inline bool Foreground() {
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}
inline void Log(const char* message) { OutputDebugStringA(message); }

inline void Texture(ID3D11DeviceContext* context, ID3D11Resource* resource,
                    const fs::path& folder, const char* name) {
    std::ofstream meta(folder / (std::string(name) + ".json"));
    if (!resource) { meta << "{\"present\":false}"; return; }
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = resource->QueryInterface(IID_PPV_ARGS(&texture));
    if (FAILED(hr)) { meta << "{\"present\":true,\"error\":\"not_texture2d\"}"; return; }
    D3D11_TEXTURE2D_DESC desc; texture->GetDesc(&desc);
    meta << "{\"present\":true,\"width\":" << desc.Width << ",\"height\":" << desc.Height
         << ",\"dxgi_format\":" << unsigned(desc.Format) << ",\"mip_levels\":" << desc.MipLevels
         << ",\"array_size\":" << desc.ArraySize << ",\"sample_count\":" << desc.SampleDesc.Count;
    if (context->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE || desc.SampleDesc.Count != 1) {
        meta << ",\"error\":\"unsupported_context_or_msaa\"}"; return;
    }
    // Preserve the resource's actual format, including typeless depth. Copying
    // into an incompatible format silently yields empty data in some wrappers.
    D3D11_TEXTURE2D_DESC staging_desc = desc;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0; staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.MiscFlags = 0;
    Microsoft::WRL::ComPtr<ID3D11Device> device; context->GetDevice(&device);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
    hr = device->CreateTexture2D(&staging_desc, nullptr, &staging);
    if (FAILED(hr)) { meta << ",\"error\":\"create_staging\",\"hresult\":" << unsigned(hr) << "}"; return; }
    context->CopyResource(staging.Get(), texture.Get());
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    hr = context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) { meta << ",\"error\":\"map_read\",\"hresult\":" << unsigned(hr) << "}"; return; }
    const size_t size = size_t(mapped.RowPitch) * desc.Height;
    const auto* bytes = static_cast<const unsigned char*>(mapped.pData);
    uint64_t nonzero = 0, checksum = 14695981039346656037ull;
    for (size_t i = 0; i < size; ++i) { nonzero += bytes[i] != 0; checksum = (checksum ^ bytes[i]) * 1099511628211ull; }
    std::ofstream pixels(folder / (std::string(name) + ".bin"), std::ios::binary);
    pixels.write(reinterpret_cast<const char*>(bytes), size);
    const bool written = pixels.good();
    context->Unmap(staging.Get(), 0);
    meta << ",\"row_pitch\":" << mapped.RowPitch << ",\"byte_count\":" << size
         << ",\"nonzero_bytes\":" << nonzero << ",\"fnv1a64\":\"" << std::hex << checksum
         << "\",\"write_ok\":" << (written ? "true" : "false") << "}";
}

inline fs::path Begin(ID3D11DeviceContext* context, const SR::SettingsData& settings,
                      const SR::SuperResolutionImpl::DrawData& draw) noexcept {
    try {
        std::lock_guard lock(capture_mutex);
        bool pressed = Foreground() && (GetAsyncKeyState(VK_F8) & 0x8000);
        bool edge = pressed && !key_down; key_down = pressed;
        const fs::path root = Root();
        std::error_code ec;
        bool requested = false;
        const auto tick = GetTickCount64();
        if (tick - last_poll >= 250) {
            last_poll = tick;
            requested = fs::exists(root / "capture.request", ec);
        }
        if ((edge || requested) && remaining == 0 && request_count < 6) {
            fs::create_directories(root);
            // Consume only this diagnostic tool's own request marker.
            if (requested) fs::remove(root / "capture.request", ec);
            SYSTEMTIME t; GetLocalTime(&t);
            std::ostringstream id;
            id << std::setfill('0') << std::setw(4) << t.wYear << std::setw(2) << t.wMonth
               << std::setw(2) << t.wDay << '-' << std::setw(2) << t.wHour << std::setw(2) << t.wMinute
               << std::setw(2) << t.wSecond << '-' << std::setw(3) << t.wMilliseconds
               << "-pid" << GetCurrentProcessId() << "-preset" << settings.render_preset << "-" << ++request_count;
            session = root / id.str(); remaining = 2; evaluation = 0;
        }
        if (remaining == 0) return {};
        --remaining;
        fs::path folder = session / ("eval-" + std::to_string(evaluation++));
        fs::create_directories(folder);
        std::ofstream meta(folder / "evaluation.json");
        meta << std::setprecision(9) << "{\"preset_hint\":" << settings.render_preset
             << ",\"render_width\":" << draw.render_width << ",\"render_height\":" << draw.render_height
             << ",\"output_width\":" << settings.output_width << ",\"output_height\":" << settings.output_height
             << ",\"hdr_input\":" << settings.hdr << ",\"auto_exposure\":" << settings.auto_exposure
             << ",\"inverted_depth\":" << settings.inverted_depth << ",\"mvs_jittered\":" << settings.mvs_jittered
             << ",\"mv_scale_x\":" << settings.mvs_x_scale << ",\"mv_scale_y\":" << settings.mvs_y_scale
             << ",\"jitter_x\":" << draw.jitter_x << ",\"jitter_y\":" << draw.jitter_y
             << ",\"reset\":" << draw.reset << ",\"pre_exposure_supplied\":" << draw.pre_exposure
             << ",\"exposure_texture_present\":" << (draw.exposure ? 1 : 0)
             << ",\"context_type\":" << unsigned(context->GetType()) << "}";
        Texture(context, draw.source_color, folder, "input-color");
        Texture(context, draw.depth_buffer, folder, "depth");
        Texture(context, draw.motion_vectors, folder, "motion-vectors");
        Texture(context, draw.exposure, folder, "exposure");
        return folder;
    } catch (...) { Log("AC7 DLSS capture failed before evaluation\n"); return {}; }
}
inline void End(ID3D11DeviceContext* context, const SR::SuperResolutionImpl::DrawData& draw,
                const fs::path& folder, uint32_t result) noexcept {
    if (folder.empty()) return;
    try {
        Texture(context, draw.output_color, folder, "output-color");
        std::ofstream(folder / "result.json") << "{\"ngx_result\":" << result << "}";
        std::ofstream(folder / "complete.txt") << "Capture complete. See per-texture metadata for readback validity.\n";
    } catch (...) { Log("AC7 DLSS capture failed after evaluation\n"); }
}
}
