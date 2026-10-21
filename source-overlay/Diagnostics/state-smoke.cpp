#include "../Source/Core/dlss/ac7_ngx_state.hpp"
#include <iostream>
using Microsoft::WRL::ComPtr;
int main() {
 ComPtr<ID3D11Device> d; ComPtr<ID3D11DeviceContext> c;
 if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)))return 1;
 auto isolated=AC7Capture::CreateNGXState(d.Get());if(!isolated)return 2;
 D3D11_BUFFER_DESC desc={};desc.ByteWidth=256;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_INDEX_BUFFER;
 ComPtr<ID3D11Buffer> original,replacement;
 if(FAILED(d->CreateBuffer(&desc,nullptr,&original))||FAILED(d->CreateBuffer(&desc,nullptr,&replacement)))return 3;
 UINT stride=16,offset=32;ID3D11Buffer* b=original.Get();c->IASetVertexBuffers(0,1,&b,&stride,&offset);c->IASetIndexBuffer(b,DXGI_FORMAT_R16_UINT,4);
 D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=4;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 ComPtr<ID3D11Texture2D> t;ComPtr<ID3D11ShaderResourceView> v;if(FAILED(d->CreateTexture2D(&td,nullptr,&t))||FAILED(d->CreateShaderResourceView(t.Get(),nullptr,&v)))return 4;
 ID3D11ShaderResourceView* view=v.Get();c->VSSetShaderResources(7,1,&view);c->GSSetShaderResources(5,1,&view);c->PSSetShaderResources(0,1,&view);
 for(int i=0;i<2;++i){
  {AC7Capture::ScopedNGXState guard(c.Get(),isolated.Get());c->ClearState();b=replacement.Get();c->IASetVertexBuffers(0,1,&b,&stride,&offset);}
  ComPtr<ID3D11Buffer> actual;UINT s=0,o=0;c->IAGetVertexBuffers(0,1,&actual,&s,&o);if(actual.Get()!=original.Get()||s!=16||o!=32)return 5;
  actual.Reset();DXGI_FORMAT f;c->IAGetIndexBuffer(&actual,&f,&o);if(actual.Get()!=original.Get()||f!=DXGI_FORMAT_R16_UINT||o!=4)return 6;
  ComPtr<ID3D11ShaderResourceView> got;c->VSGetShaderResources(7,1,&got);if(got.Get()!=v.Get())return 7;
  got.Reset();c->GSGetShaderResources(5,1,&got);if(got.Get()!=v.Get())return 8;
  got.Reset();c->PSGetShaderResources(0,1,&got);if(got.Get()!=v.Get())return 9;
 }
 std::cout<<"PASS: vertex/index buffers and VS/GS/PS resources survive two isolated calls with ClearState and replacement bindings.\n";
}
