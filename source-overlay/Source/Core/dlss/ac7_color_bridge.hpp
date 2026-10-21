#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstring>

namespace AC7Capture {
// Experimental, bounded HDR transport through DLSS's LDR color path.
// Fixed mapping across frames avoids introducing scene-dependent exposure jumps.
class ColorBridge {
    using Blob = Microsoft::WRL::ComPtr<ID3DBlob>;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> encode, decode;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> input, output;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> input_srv, output_srv;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> input_uav, output_uav;
    UINT width = 0, height = 0, uav_count = 8;
    static constexpr const char* shader = R"(
Texture2D<float4> Source : register(t0);
RWTexture2D<float4> Target : register(u0);
float3 ToSRGB(float3 v) { return float3(v.x<=0.0031308?12.92*v.x:1.055*pow(v.x,1.0/2.4)-0.055, v.y<=0.0031308?12.92*v.y:1.055*pow(v.y,1.0/2.4)-0.055, v.z<=0.0031308?12.92*v.z:1.055*pow(v.z,1.0/2.4)-0.055); }
float3 ToLinear(float3 v) { return float3(v.x<=0.04045?v.x/12.92:pow((v.x+0.055)/1.055,2.4), v.y<=0.04045?v.y/12.92:pow((v.y+0.055)/1.055,2.4), v.z<=0.04045?v.z/12.92:pow((v.z+0.055)/1.055,2.4)); }
[numthreads(8,8,1)] void Encode(uint3 p:SV_DispatchThreadID) {
 uint w,h;Target.GetDimensions(w,h);if(p.x>=w||p.y>=h)return;
 float4 c=Source.Load(int3(p.xy,0));float3 v=log2(1+clamp(c.rgb,0,65504))/log2(65505.0);
 Target[p.xy]=float4(saturate(ToSRGB(v)),c.a);
}
[numthreads(8,8,1)] void Decode(uint3 p:SV_DispatchThreadID) {
 uint w,h;Target.GetDimensions(w,h);if(p.x>=w||p.y>=h)return;
 float4 c=Source.Load(int3(p.xy,0));float3 v=ToLinear(saturate(c.rgb));
 Target[p.xy]=float4(clamp(exp2(v*log2(65505.0))-1,0,65504),c.a);
}
)";
    bool Shaders(ID3D11Device* d) {
        if (encode && decode) return true;
        using Compile = HRESULT(WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
        HMODULE module=LoadLibraryW(L"D3DCompiler_47.dll");if(!module)return false;
        auto compile=reinterpret_cast<Compile>(GetProcAddress(module,"D3DCompile"));
        bool ok=compile!=nullptr;
        for(int i=0;ok&&i<2;++i){Blob code,errors;ok=SUCCEEDED(compile(shader,strlen(shader),"AC7 bounded color",nullptr,nullptr,i?"Decode":"Encode","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors));
            if(ok)ok=SUCCEEDED(d->CreateComputeShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,i?decode.ReleaseAndGetAddressOf():encode.ReleaseAndGetAddressOf()));}
        FreeLibrary(module);return ok;
    }
    bool Texture(ID3D11Device* d, Microsoft::WRL::ComPtr<ID3D11Texture2D>& t,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& s,Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& u,UINT w,UINT h) {
        t.Reset();s.Reset();u.Reset();D3D11_TEXTURE2D_DESC desc={};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
        return SUCCEEDED(d->CreateTexture2D(&desc,nullptr,&t))&&SUCCEEDED(d->CreateShaderResourceView(t.Get(),nullptr,&s))&&SUCCEEDED(d->CreateUnorderedAccessView(t.Get(),nullptr,&u));
    }
    void Dispatch(ID3D11DeviceContext* c,ID3D11ComputeShader* cs,ID3D11ShaderResourceView* s,ID3D11UnorderedAccessView* u) {
        ID3D11UnorderedAccessView* empty_uavs[64] = {};
        ID3D11ShaderResourceView* empty_srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
        c->OMSetRenderTargetsAndUnorderedAccessViews(0,nullptr,nullptr,0,uav_count,empty_uavs,nullptr);
        c->CSSetUnorderedAccessViews(0,uav_count,empty_uavs,nullptr);
        c->CSSetShaderResources(0,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,empty_srvs);
        c->CSSetShader(cs,nullptr,0);c->CSSetShaderResources(0,1,&s);c->CSSetUnorderedAccessViews(0,1,&u,nullptr);c->Dispatch((width+7)/8,(height+7)/8,1);
        ID3D11ShaderResourceView* nulls=nullptr;ID3D11UnorderedAccessView* nullu=nullptr;c->CSSetShaderResources(0,1,&nulls);c->CSSetUnorderedAccessViews(0,1,&nullu,nullptr);
    }
public:
    bool Prepare(ID3D11Device* d,UINT w,UINT h) {
        uav_count = d->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_1 ? 64 : 8;
        if(!Shaders(d))return false;
        if(w!=width||h!=height||!input_uav||!output_uav){
            if(!Texture(d,input,input_srv,input_uav,w,h)||!Texture(d,output,output_srv,output_uav,w,h))return false;width=w;height=h;
        }return true;
    }
    bool Encode(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Resource* source) {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        if(FAILED(d->CreateShaderResourceView(source,nullptr,&srv)))return false;
        Dispatch(c,encode.Get(),srv.Get(),input_uav.Get());return true;
    }
    bool Decode(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Resource* target) {
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
        if(FAILED(d->CreateUnorderedAccessView(target,nullptr,&uav)))return false;
        Dispatch(c,decode.Get(),output_srv.Get(),uav.Get());return true;
    }
    ID3D11Resource* Input()const{return input.Get();}
    ID3D11Resource* Output()const{return output.Get();}
};
}

