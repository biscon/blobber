#pragma once
#include <string>
namespace blobber {
inline std::string withSpellLighting(const char* shader) {
    static constexpr const char* common = R"(
uniform sampler2D spellLightData;uniform int spellLightCount;
vec4 spellData(int i){return texelFetch(spellLightData,ivec2(i%32,i/32),0);}
bool lightSlab(float at,float delta,float low,float high,inout float enter,inout float leave){
if(abs(delta)<1e-7)return at>=low&&at<=high;
float a=(low-at)/delta,b=(high-at)/delta;enter=max(enter,min(a,b));leave=min(leave,max(a,b));return leave-enter>1e-6;}
bool lightClear(int i,vec3 target){
vec3 source=spellData(i*3).xyz,delta=target-source;ivec2 cell=ivec2(floor(source.xz));
ivec2 origin=ivec2(spellData(i*3+2).xy);ivec2 stepDir=ivec2(delta.x>=0.0?1:-1,delta.z>=0.0?1:-1);
vec2 inv=vec2(abs(delta.x)>1e-7?abs(1.0/delta.x):1e20,abs(delta.z)>1e-7?abs(1.0/delta.z):1e20);
vec2 edge=vec2(cell)+vec2(stepDir.x>0?1:0,stepDir.y>0?1:0);
vec2 next=vec2(abs(delta.x)>1e-7?(edge.x-source.x)/delta.x:1e20,abs(delta.z)>1e-7?(edge.y-source.z)/delta.z:1e20);
float at=0.0;
for(int s=0;s<8;++s){ivec2 c=cell-origin;if(any(lessThan(c,ivec2(0)))||any(greaterThanEqual(c,ivec2(5))))return false;
vec4 tile=spellData(24+i*25+c.y*5+c.x);float end=min(1.0,min(next.x,next.y));
float a=source.y+delta.y*at,b=source.y+delta.y*end;
if(tile.z>0.0||min(a,b)<tile.x-0.0001||max(a,b)>tile.y+0.0001)return false;
if(tile.w>0.0)for(int k=0;k<4;++k){int q=224+(int(tile.w)-1+k)*3;
vec4 o=spellData(q),x=spellData(q+1),z=spellData(q+2);vec3 d=source-o.xyz;float enter=0.00001,leave=0.99999;
if(lightSlab(dot(d,x.xyz),dot(delta,x.xyz),0,1,enter,leave)&&lightSlab(d.y,delta.y,0,o.w,enter,leave)&&
lightSlab(dot(d,z.xyz),dot(delta,z.xyz),0,1,enter,leave))return false;}
if(end>=1.0)return true;
if(abs(next.x-next.y)<1e-6){int other=c.y+stepDir.y;if(other<0||other>=5)return false;
vec4 side=spellData(24+i*25+other*5+c.x);if(side.z>0.0||side.w>0.0||b<side.x-0.0001||b>side.y+0.0001)return false;}
if(next.x<=next.y){at=next.x;next.x+=inv.x;cell.x+=stepDir.x;}else{at=next.y;next.y+=inv.y;cell.y+=stepDir.y;}}
return false;}
vec3 spellIllumination(vec3 p,vec3 normal,bool creature){vec3 result=vec3(0);
for(int i=0;i<8;++i){if(i>=spellLightCount)break;vec4 l=spellData(i*3),c=spellData(i*3+1);vec3 delta=l.xyz-p;
float d=length(delta);if(d>=l.w)continue;float face=creature?0.55:max(0.0,dot(normal,delta/max(d,0.001)));
if(face<=0.0||!lightClear(i,p+normal*0.002))continue;float fade=1.0-d/l.w;result+=c.rgb*c.a*fade*fade*face;}
return min(result,vec3(1.25));}
)";
    std::string result(shader);
    result.insert(result.find('\n') + 1, common);
    return result;
}
} // namespace blobber
