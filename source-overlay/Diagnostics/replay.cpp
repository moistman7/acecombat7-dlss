#include <Windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <regex>
#include "nvsdk_ngx.h"
#include "nvsdk_ngx_helpers.h"
#include "../Source/Core/includes/super_resolution.h"
#include "../Source/Core/dlss/ac7_capture.hpp"
#include "../Source/Core/dlss/ac7_color_bridge.hpp"
using Microsoft::WRL::ComPtr;namespace fs=std::filesystem;
unsigned field(const std::string&s,const char*n){std::smatch m;if(!std::regex_search(s,m,std::regex(std::string("\"")+n+"\":([0-9]+)")))throw std::runtime_error(n);return std::stoul(m[1]);}
ComPtr<ID3D11Texture2D> load(ID3D11Device*d,fs::path p,const char*n){
 std::ifstream f(p/(std::string(n)+".json"));std::string j((std::istreambuf_iterator<char>(f)),{});
 std::ifstream bin(p/(std::string(n)+".bin"),std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(bin)),{});
 D3D11_TEXTURE2D_DESC td={};td.Width=field(j,"width");td.Height=field(j,"height");td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=(DXGI_FORMAT)field(j,"dxgi_format");td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA initial={data.data(),field(j,"row_pitch"),0};ComPtr<ID3D11Texture2D> t;
 if(FAILED(d->CreateTexture2D(&td,&initial,&t)))throw std::runtime_error(n);return t;
}
float realfield(const std::string&s,const char*n){std::smatch m;if(!std::regex_search(s,m,std::regex(std::string("\"")+n+"\":([-0-9.eE+]+)")))throw std::runtime_error(n);return std::stof(m[1]);}
int main(int argc,char**argv){try{
 if(argc<3)return 1;fs::path capture=argv[1],out=argv[2];fs::create_directories(out);
 std::ifstream evaluation(capture/"evaluation.json");std::string evaluation_json((std::istreambuf_iterator<char>(evaluation)),{});
ComPtr<ID3D11Device>d;ComPtr<ID3D11DeviceContext>c;if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d,nullptr,&c)))return 2;
 auto r=NVSDK_NGX_D3D11_Init_with_ProjectID("d8238c51-1f2f-438d-a309-38c16e33c716",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"1.0",L".",d.Get());std::cout<<"init "<<std::hex<<r<<std::endl;if(NVSDK_NGX_FAILED(r))return 3;
 auto color=load(d.Get(),argc>3?fs::path(argv[3]):capture,"input-color"),depth=load(d.Get(),capture,"depth"),motion=load(d.Get(),capture,"motion-vectors");
 D3D11_TEXTURE2D_DESC td;color->GetDesc(&td);td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;ComPtr<ID3D11Texture2D>output;if(FAILED(d->CreateTexture2D(&td,nullptr,&output)))return 4;
 D3D11_TEXTURE2D_DESC mvdesc;motion->GetDesc(&mvdesc);std::vector<char> zeros(size_t(mvdesc.Width)*mvdesc.Height*8);D3D11_SUBRESOURCE_DATA zi={zeros.data(),mvdesc.Width*8,0};ComPtr<ID3D11Texture2D> zeroMotion;if(FAILED(d->CreateTexture2D(&mvdesc,&zi,&zeroMotion)))return 5;
struct Test{const char*name;float pre,exposure,mv;bool reset;int preset;};
 Test tests[]={{"baseline-reset",1,1,1,true,13},{"baseline-history",1,1,1,false,13},{"pre-001",.01f,1,1,true,13},{"pre-100",100,1,1,true,13},{"exposure-001",1,.01f,1,true,13},{"exposure-100",1,100,1,true,13},{"zero-motion",1,1,0,true,13},{"K-reset",1,1,1,true,11},{"E-reset",1,1,1,true,5},{"M-performance",1,1,1,true,13},{"M-quality",1,1,1,true,13},{"M-nohdr",1,1,1,true,13},{"M-noauto",1,1,1,true,13},{"M-bridge",1,1,1,true,13},{"M-bridge-history",1,1,1,false,13}};
 for(auto test:tests){if(argc>3 && std::string(test.name)!=(argc>4?argv[4]:"baseline-reset"))continue;NVSDK_NGX_Parameter*params=nullptr;NVSDK_NGX_D3D11_AllocateParameters(&params);
 const char*keys[]={NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance};for(auto key:keys)NVSDK_NGX_Parameter_SetUI(params,key,test.preset);
 NVSDK_NGX_DLSS_Create_Params cp={};cp.Feature.InWidth=cp.Feature.InTargetWidth=td.Width;cp.Feature.InHeight=cp.Feature.InTargetHeight=td.Height;cp.Feature.InPerfQualityValue=NVSDK_NGX_PerfQuality_Value_DLAA;cp.InFeatureCreateFlags=NVSDK_NGX_DLSS_Feature_Flags_MVLowRes|NVSDK_NGX_DLSS_Feature_Flags_DepthInverted|NVSDK_NGX_DLSS_Feature_Flags_AutoExposure|NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
 if(std::string(test.name)=="M-performance")cp.Feature.InPerfQualityValue=NVSDK_NGX_PerfQuality_Value_MaxPerf;
if(std::string(test.name)=="M-quality")cp.Feature.InPerfQualityValue=NVSDK_NGX_PerfQuality_Value_MaxQuality;
if(std::string(test.name)=="M-nohdr" || std::string(test.name).find("M-bridge")==0)cp.InFeatureCreateFlags &= ~NVSDK_NGX_DLSS_Feature_Flags_IsHDR;
if(std::string(test.name)=="M-noauto")cp.InFeatureCreateFlags &= ~NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
NVSDK_NGX_Handle*feature=nullptr;r=NGX_D3D11_CREATE_DLSS_EXT(c.Get(),&feature,params,&cp);std::cout<<test.name<<" create "<<std::hex<<r<<std::endl;if(NVSDK_NGX_FAILED(r)){NVSDK_NGX_D3D11_DestroyParameters(params);continue;}
 AC7Capture::ColorBridge bridge;bool useBridge=std::string(test.name).find("M-bridge")==0;if(useBridge && (!bridge.Prepare(d.Get(),td.Width,td.Height)||!bridge.Encode(d.Get(),c.Get(),color.Get())))return 11;
NVSDK_NGX_D3D11_DLSS_Eval_Params ep={};ep.Feature.pInColor=color.Get();ep.Feature.pInOutput=output.Get();ep.pInDepth=depth.Get();ep.pInMotionVectors=std::string(test.name)=="zero-motion"?zeroMotion.Get():motion.Get();ep.InRenderSubrectDimensions.Width=td.Width;ep.InRenderSubrectDimensions.Height=td.Height;ep.InMVScaleX=ep.InMVScaleY=test.mv;ep.InPreExposure=test.pre;ep.InExposureScale=test.exposure;ep.InJitterOffsetX=realfield(evaluation_json,"jitter_x");ep.InJitterOffsetY=realfield(evaluation_json,"jitter_y");
 if(useBridge){ep.Feature.pInColor=bridge.Input();ep.Feature.pInOutput=bridge.Output();}
for(int i=0;i<8;++i){ep.InReset=test.reset||i==0;r=NGX_D3D11_EVALUATE_DLSS_EXT(c.Get(),feature,params,&ep);if(NVSDK_NGX_FAILED(r))break;}
 if(useBridge && !bridge.Decode(d.Get(),c.Get(),output.Get()))return 12;
fs::create_directories(out/test.name);std::ofstream manifest(out/test.name/"test.json");manifest<<"{\"preset_hint\":"<<test.preset<<",\"pre_exposure\":"<<test.pre<<",\"exposure_scale\":"<<test.exposure<<",\"reset_every_evaluation\":"<<(test.reset?"true":"false")<<",\"evaluations\":8,\"ngx_result\":"<<uint32_t(r)<<",\"feature_flags\":"<<cp.InFeatureCreateFlags<<",\"quality_mode\":"<<int(cp.Feature.InPerfQualityValue)<<",\"zero_motion_texture\":"<<(std::string(test.name)=="zero-motion"?"true":"false")<<"}";AC7Capture::Texture(c.Get(),output.Get(),out/test.name,"output-color");std::cout<<test.name<<" eval "<<std::hex<<r<<std::endl;NVSDK_NGX_D3D11_ReleaseFeature(feature);NVSDK_NGX_D3D11_DestroyParameters(params);
 }
 NVSDK_NGX_D3D11_Shutdown1(d.Get());return 0;
 }catch(const std::exception&e){std::cerr<<e.what();return 10;}}

