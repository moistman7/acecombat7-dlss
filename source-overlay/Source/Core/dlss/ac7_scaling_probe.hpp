#pragma once
#include "ac7_capture.hpp"
namespace AC7ScaleProbe {
inline bool Enabled() {
 static const bool enabled=[](){wchar_t exe[32768]={};GetModuleFileNameW(nullptr,exe,32768);return _wcsicmp(std::filesystem::path(exe).filename().c_str(),L"Ace7Game.exe")==0 && std::filesystem::exists(std::filesystem::path(exe).parent_path()/L"ac7-scaling-probe.enabled");}();
 return enabled;
}
inline std::ofstream trace;
inline unsigned remaining=0,request=0,index=0;
inline ULONGLONG last_poll=0;
inline ULONGLONG last_marker_poll=0;
inline bool key_down=false;
inline bool scene_armed=false;
inline void Resource(ID3D11Resource* resource) {
 if(!resource){trace<<"null";return;}
 Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
 if(FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))){trace<<"buffer";return;}
 D3D11_TEXTURE2D_DESC desc;texture->GetDesc(&desc);
 trace<<std::hex<<reinterpret_cast<uintptr_t>(resource)<<std::dec<<':'<<desc.Width<<'x'<<desc.Height<<':'<<unsigned(desc.Format);
}
inline void Draw(ID3D11DeviceContext* c,uint32_t hash,bool compute) noexcept {
 if(!Enabled())return;
 try {
  const auto now=GetTickCount64();
  if(now-last_poll>=20){last_poll=now;bool down=AC7Capture::Foreground()&&(GetAsyncKeyState(VK_F8)&0x8000);bool edge=down&&!key_down;key_down=down;
   const auto marker=AC7Capture::Root()/L"scaling-probe.request";
   const auto scene_marker=AC7Capture::Root()/L"scaling-probe-scene.request";
   bool marker_requested=false;
   if(remaining==0&&request<6&&now-last_marker_poll>=250){last_marker_poll=now;std::error_code ec;marker_requested=std::filesystem::exists(marker,ec);if(std::filesystem::exists(scene_marker,ec)){scene_armed=true;std::filesystem::remove(scene_marker,ec);}}
   if((edge||marker_requested)&&remaining==0&&request<6){std::filesystem::create_directories(AC7Capture::Root());trace.close();trace.open(AC7Capture::Root()/("scaling-probe-pid"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++request)+".txt"));
    if(trace){if(marker_requested){std::error_code ec;std::filesystem::remove(marker,ec);}trace<<"AC7 scaling probe: resource pointer:widthxheight:DXGI; no pixel readbacks\n";remaining=1500;index=0;}}
  }
  // Wait for a known AC7 scene TAA pass rather than spending the trace on
  // startup movies/Slate-only title frames. Other requests still start now.
  if(scene_armed&&remaining==0&&request<6&&(hash==0x7DEC23DFu||hash==0x640B9ECBu)){
   std::filesystem::create_directories(AC7Capture::Root());trace.close();trace.open(AC7Capture::Root()/("scaling-probe-pid"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(++request)+".txt"));
   if(trace){scene_armed=false;trace<<"AC7 scene scaling probe: starts at TAA; resource pointer:widthxheight:DXGI; no pixel readbacks\n";remaining=5000;index=0;}
  }
  if(!remaining||!trace)return;
  --remaining;trace<<index++<<' '<<(compute?"CS":"PS")<<' '<<std::hex<<hash<<std::dec;
  D3D11_VIEWPORT vp={};UINT n=1;c->RSGetViewports(&n,&vp);trace<<" vp="<<vp.TopLeftX<<','<<vp.TopLeftY<<','<<vp.Width<<','<<vp.Height;
  trace<<" out=";Microsoft::WRL::ComPtr<ID3D11Resource> resource;
  if(compute){Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> view;c->CSGetUnorderedAccessViews(0,1,&view);if(view)view->GetResource(&resource);}
  else{Microsoft::WRL::ComPtr<ID3D11RenderTargetView> view;c->OMGetRenderTargets(1,&view,nullptr);if(view)view->GetResource(&resource);}
  Resource(resource.Get());
  for(UINT slot=0;slot<8;++slot){Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;if(compute)c->CSGetShaderResources(slot,1,&view);else c->PSGetShaderResources(slot,1,&view);if(!view)continue;resource.Reset();view->GetResource(&resource);trace<<" t"<<slot<<'=';Resource(resource.Get());}
  trace<<'\n';if(remaining==0){trace<<"complete\n";trace.close();}else trace.flush();
 }catch(...){remaining=0;trace.close();}
}
}
