#include <Windows.h>
#include <d3d11.h>
#include "../Source/Core/includes/super_resolution.h"
#include "../Source/Core/dlss/ac7_scaling_runtime.hpp"
#include <iostream>
#include <vector>
using Microsoft::WRL::ComPtr;
int main(){
 ComPtr<ID3D11Device>d;ComPtr<ID3D11DeviceContext>c;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)))return 1;
 AC7Scaling::ColorCopy copy;
 for(UINT w:{13u,27u}){
  UINT h=9;std::vector<unsigned short> pixels(w*h*4);for(size_t i=0;i<pixels.size();++i)pixels[i]=static_cast<unsigned short>(0x3000+i%1000);
  D3D11_TEXTURE2D_DESC td={};td.Width=w;td.Height=h;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA init={pixels.data(),w*8,0};ComPtr<ID3D11Texture2D>s,t,read;ComPtr<ID3D11ShaderResourceView>v;
  if(FAILED(d->CreateTexture2D(&td,&init,&s))||FAILED(d->CreateShaderResourceView(s.Get(),nullptr,&v))||FAILED(d->CreateTexture2D(&td,nullptr,&t)))return 2;
  ID3D11ShaderResourceView* bound=v.Get();c->PSSetShaderResources(4,1,&bound);
  if(!copy.Copy(d.Get(),c.Get(),v.Get(),t.Get()))return 3;
  ComPtr<ID3D11ShaderResourceView>got;c->PSGetShaderResources(4,1,&got);if(got.Get()!=v.Get())return 4;
  td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(d->CreateTexture2D(&td,nullptr,&read)))return 5;
  c->CopyResource(read.Get(),t.Get());D3D11_MAPPED_SUBRESOURCE m;if(FAILED(c->Map(read.Get(),0,D3D11_MAP_READ,0,&m)))return 6;
  for(UINT y=0;y<h;++y)if(memcmp((BYTE*)m.pData+y*m.RowPitch,pixels.data()+y*w*4,w*8))return 7;c->Unmap(read.Get(),0);
 }
 {
  D3D11_TEXTURE2D_DESC td={};td.Width=13;td.Height=9;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R11G11B10_FLOAT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  std::vector<UINT> pixels(13*9,0x781e03c0);D3D11_SUBRESOURCE_DATA init={pixels.data(),13*4,0};ComPtr<ID3D11Texture2D>s,t,read;ComPtr<ID3D11ShaderResourceView>v;
  if(FAILED(d->CreateTexture2D(&td,&init,&s))||FAILED(d->CreateShaderResourceView(s.Get(),nullptr,&v)))return 8;
  td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;if(FAILED(d->CreateTexture2D(&td,nullptr,&t))||!copy.Copy(d.Get(),c.Get(),v.Get(),t.Get()))return 9;
  td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(d->CreateTexture2D(&td,nullptr,&read)))return 10;
  c->CopyResource(read.Get(),t.Get());D3D11_MAPPED_SUBRESOURCE m;if(FAILED(c->Map(read.Get(),0,D3D11_MAP_READ,0,&m)))return 11;
  for(UINT y=0;y<9;++y){auto row=(unsigned short*)((BYTE*)m.pData+y*m.RowPitch);for(UINT x=0;x<13*4;++x)if(row[x]!=0x3c00)return 12;}c->Unmap(read.Get(),0);
 }
 std::cout<<"PASS: TAA bypass copies every HDR half-float texel, handles odd sizes/resizing, and preserves PS bindings.\n";
}

