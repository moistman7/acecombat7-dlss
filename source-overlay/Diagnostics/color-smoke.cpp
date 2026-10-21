#define NOMINMAX
#include <Windows.h>
#include "../Source/Core/dlss/ac7_color_bridge.hpp"
#include <vector>
#include <cmath>
#include <iostream>
using Microsoft::WRL::ComPtr;
int main(){ComPtr<ID3D11Device>d;ComPtr<ID3D11DeviceContext>c;if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)))return 1;
 const UINT w=19,h=13;float values[]={0,.00001f,.001f,.01f,.1f,1,10,100,500,10000,65504};std::vector<float> pixels(w*h*4);for(UINT i=0;i<w*h;++i){for(int k=0;k<3;++k)pixels[i*4+k]=values[(i+k)%11];pixels[i*4+3]=1;}
 D3D11_TEXTURE2D_DESC td={};td.Width=w;td.Height=h;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;D3D11_SUBRESOURCE_DATA initial={pixels.data(),w*16,0};ComPtr<ID3D11Texture2D>source,target;if(FAILED(d->CreateTexture2D(&td,&initial,&source))||FAILED(d->CreateTexture2D(&td,nullptr,&target)))return 2;
 AC7Capture::ColorBridge bridge;if(!bridge.Prepare(d.Get(),w,h)||!bridge.Encode(d.Get(),c.Get(),source.Get()))return 3;
 c->CopyResource(bridge.Output(),bridge.Input());if(!bridge.Decode(d.Get(),c.Get(),target.Get()))return 4;
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D>staging;if(FAILED(d->CreateTexture2D(&td,nullptr,&staging)))return 5;c->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(c->Map(staging.Get(),0,D3D11_MAP_READ,0,&map)))return 6;
 float worst=0;for(UINT y=0;y<h;++y){auto row=reinterpret_cast<float*>(static_cast<char*>(map.pData)+y*map.RowPitch);for(UINT x=0;x<w;++x)for(UINT k=0;k<4;++k){float expected=pixels[(y*w+x)*4+k],actual=row[x*4+k];if(!std::isfinite(actual)||actual<0||actual>65504)return 7;float error=std::abs(actual-expected)/std::max(expected,.00001f);worst=std::max(error,worst);if(std::abs(actual-expected)>std::max(.00001f,expected*.02f))return 8;}}
 c->Unmap(staging.Get(),0);if(!bridge.Prepare(d.Get(),7,5)||!bridge.Prepare(d.Get(),w,h))return 9;
 std::cout<<"PASS: GPU encode/decode roundtrip, odd dimensions, alpha, 0..65504 finite bounds and resource resize; worst relative error "<<worst<<"\n";
}

