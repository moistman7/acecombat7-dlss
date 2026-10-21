#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include "../Source/Core/includes/super_resolution.h"
#include "../Source/Core/dlss/ac7_capture.hpp"
using Microsoft::WRL::ComPtr;
int main() {
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
    if (FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context))) return 1;
    std::vector<uint32_t> pattern(16*8);
    for (unsigned i=0;i<pattern.size();++i) pattern[i]=0xFF000000u+i*12345u;
    D3D11_TEXTURE2D_DESC desc={}; desc.Width=16;desc.Height=8;desc.MipLevels=1;desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial={pattern.data(),16*4,0}; ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&desc,&initial,&texture))) return 2;
    SR::SettingsData settings;settings.render_preset=13;settings.render_width=16;settings.render_height=8;settings.output_width=16;settings.output_height=8;settings.auto_exposure=true;
    SR::SuperResolutionImpl::DrawData draw;draw.source_color=texture.Get();draw.output_color=texture.Get();draw.depth_buffer=texture.Get();draw.motion_vectors=texture.Get();draw.render_width=16;draw.render_height=8;draw.jitter_x=.25f;draw.reset=true;
    auto root=AC7Capture::Root();std::filesystem::create_directories(root);std::ofstream(root/"capture.request") << "smoke";
    auto folder=AC7Capture::Begin(context.Get(),settings,draw);if(folder.empty())return 3;
    AC7Capture::End(context.Get(),draw,folder,1);
    std::ifstream meta(folder/"input-color.json");std::string text((std::istreambuf_iterator<char>(meta)),{});
    auto start=text.find("\"row_pitch\":");if(start==std::string::npos)return 4;
    unsigned pitch=std::stoul(text.substr(start+12));
    std::ifstream raw(folder/"input-color.bin",std::ios::binary);std::vector<uint8_t> pixels((std::istreambuf_iterator<char>(raw)),{});
    if(pixels.size()!=size_t(pitch)*8)return 5;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<16;x++) {uint32_t actual;memcpy(&actual,pixels.data()+y*pitch+x*4,4);if(actual!=pattern[y*16+x])return 6;}
    std::ifstream exposure(folder/"exposure.json");std::string absent((std::istreambuf_iterator<char>(exposure)),{});if(absent!="{\"present\":false}")return 7;
    if(!std::filesystem::exists(folder/"complete.txt"))return 8;
    auto second=AC7Capture::Begin(context.Get(),settings,draw);if(second.empty())return 9;
    AC7Capture::End(context.Get(),draw,second,1);
    if(!AC7Capture::Begin(context.Get(),settings,draw).empty())return 10;
    std::cout << "PASS: GPU readback matches every texel; null exposure recorded; two evaluations captured; idle capture stops.\n";
    std::cout << folder << '\n';return 0;
}
