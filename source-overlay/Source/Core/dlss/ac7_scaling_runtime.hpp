#pragma once
#include "ac7_capture.hpp"
#include "ac7_ngx_state.hpp"
#include <d3dcompiler.h>
#include <cstring>
#include <cmath>
namespace AC7Scaling {
inline int mode = 1;
inline unsigned revision = 0;
inline const char* ModeNames = "Native DLAA\0Quality\0Balanced\0Performance\0";
inline float Percentage() {static const float percentages[] = {100.f, 66.666667f, 58.f, 50.f};return percentages[mode>=0&&mode<=3?mode:0];}
inline bool Enabled() {
 static const bool enabled=[](){wchar_t exe[32768]={};GetModuleFileNameW(nullptr,exe,32768);return _wcsicmp(std::filesystem::path(exe).filename().c_str(),L"Ace7Game.exe")==0&&std::filesystem::exists(std::filesystem::path(exe).parent_path()/L"ac7-scaling-experimental.enabled");}();return enabled;
}
inline void Record(const std::string& message) noexcept {
 try {static unsigned count=0;static std::string last;if(last==message)return;last=message;if(count++>=256)return;std::filesystem::create_directories(AC7Capture::Root());std::ofstream(AC7Capture::Root()/("scaling-runtime-pid"+std::to_string(GetCurrentProcessId())+".txt"),std::ios::app)<<message<<'\n';}catch(...){}
}
// Invoke the game's own console-variable setter. No executable bytes are
// patched. Refuse an unknown image/layout instead of guessing an address.
inline bool ApplyMode() {
 if(!Enabled())return false;
 auto base=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
 const BYTE getter[]={0x48,0x8b,0x05,0x21,0x17,0x5f,0x03,0xc3};
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.SizeOfImage!=71409664||memcmp(base+0x4650e0,getter,sizeof(getter)))return false;
 auto variable=*reinterpret_cast<void**>(base+0x3a944e0);
 auto data=*reinterpret_cast<float**>(base+0x3a944e8);
 if(!variable||!data)return false;
 const float wanted=Percentage();
 if(std::abs(data[0]-wanted)<.001f&&std::abs(data[1]-wanted)<.001f)return true;
 auto setter=(*reinterpret_cast<void***>(variable))[12];MEMORY_BASIC_INFORMATION info={};
 if(!VirtualQuery(setter,&info,sizeof(info))||info.AllocationBase!=base)return false;
 static const wchar_t* values[]={L"100",L"66.666667",L"58",L"50"};
 reinterpret_cast<void(__fastcall*)(void*,const wchar_t*,uint32_t)>(setter)(variable,values[mode>=0&&mode<=3?mode:0],0x09000000u);
 Record("scene percentage requested "+std::to_string(wanted));return true;
}
inline bool PollModeRequest() {
 static ULONGLONG last=0;auto now=GetTickCount64();if(now-last<250)return false;last=now;
 try{auto path=AC7Capture::Root()/L"scaling-mode.request";std::ifstream file(path);int requested=-1;if(!(file>>requested)||requested<0||requested>3)return false;file.close();std::filesystem::remove(path);if(requested==mode)return false;mode=requested;++revision;return true;}catch(...){return false;}
}

class ColorCopy {
 Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader;
 Microsoft::WRL::ComPtr<ID3D11Texture2D> scratch;
 Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
 Microsoft::WRL::ComPtr<ID3DDeviceContextState> state;
 UINT width=0,height=0;
public:
 bool Copy(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11ShaderResourceView* source,ID3D11Texture2D* target) {
  D3D11_TEXTURE2D_DESC desc;target->GetDesc(&desc);if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||desc.SampleDesc.Count!=1||desc.ArraySize!=1||desc.MipLevels!=1)return false;
  if(!shader){
   using Compile=HRESULT(WINAPI*)(LPCVOID,SIZE_T,LPCSTR,const D3D_SHADER_MACRO*,ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,ID3DBlob**,ID3DBlob**);
   HMODULE module=LoadLibraryW(L"D3DCompiler_47.dll");if(!module)return false;auto compile=reinterpret_cast<Compile>(GetProcAddress(module,"D3DCompile"));
   const char* code="Texture2D<float4> S:register(t0);RWTexture2D<float4> T:register(u0);[numthreads(8,8,1)]void main(uint3 p:SV_DispatchThreadID){uint w,h;T.GetDimensions(w,h);if(p.x<w&&p.y<h)T[p.xy]=S.Load(int3(p.xy,0));}";
   Microsoft::WRL::ComPtr<ID3DBlob> binary,errors;bool ok=compile&&SUCCEEDED(compile(code,strlen(code),"AC7 TAA bypass",nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&binary,&errors));
   if(ok)ok=SUCCEEDED(device->CreateComputeShader(binary->GetBufferPointer(),binary->GetBufferSize(),nullptr,&shader));binary.Reset();errors.Reset();FreeLibrary(module);if(!ok)return false;

  }
  if(!state)state=AC7Capture::CreateNGXState(device);if(!state)return false;
  if(!scratch||width!=desc.Width||height!=desc.Height){
   scratch.Reset();uav.Reset();desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;desc.Usage=D3D11_USAGE_DEFAULT;desc.CPUAccessFlags=desc.MiscFlags=0;
   if(FAILED(device->CreateTexture2D(&desc,nullptr,&scratch))||FAILED(device->CreateUnorderedAccessView(scratch.Get(),nullptr,&uav)))return false;width=desc.Width;height=desc.Height;
  }
  AC7Capture::ScopedNGXState isolate(context,state.Get());ID3D11UnorderedAccessView* output=uav.Get();
  context->CSSetShader(shader.Get(),nullptr,0);context->CSSetShaderResources(0,1,&source);context->CSSetUnorderedAccessViews(0,1,&output,nullptr);context->Dispatch((width+7)/8,(height+7)/8,1);
  output=nullptr;source=nullptr;context->CSSetShaderResources(0,1,&source);context->CSSetUnorderedAccessViews(0,1,&output,nullptr);context->CopyResource(target,scratch.Get());return true;
 }
};
}

