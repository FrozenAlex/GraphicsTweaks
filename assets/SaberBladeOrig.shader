#version 300 es
#ifndef UNITY_RUNTIME_INSTANCING_ARRAY_SIZE
	#define UNITY_RUNTIME_INSTANCING_ARRAY_SIZE 2
#endif
precision highp float;
precision highp int;
#define HLSLCC_ENABLE_UNIFORM_BUFFERS 1
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
#define UNITY_UNIFORM
#else
#define UNITY_UNIFORM uniform
#endif
#define UNITY_SUPPORTS_UNIFORM_LOCATION 1
#if UNITY_SUPPORTS_UNIFORM_LOCATION
#define UNITY_LOCATION(x) layout(location = x)
#define UNITY_BINDING(x) layout(binding = x, std140)
#else
#define UNITY_LOCATION(x)
#define UNITY_BINDING(x) layout(std140)
#endif
uniform 	vec4 _Time;
uniform 	int unity_BaseInstanceID;
uniform 	vec4 _MainTex_ST;
uniform 	mediump vec2 _MainTexSpeed;
uniform 	mediump float _Brightness;
uniform 	vec4 _TimeHelperOffset;
struct PropsArray_Type {
	mediump vec4 _TintColor;
};
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
UNITY_BINDING(0) uniform UnityInstancing_Props {
#endif
	UNITY_UNIFORM PropsArray_Type                PropsArray[128];
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
};
#endif
UNITY_LOCATION(0) uniform mediump sampler2D _MainTex;
in highp  vec2 vs_TEXCOORD0;
in highp  float vs_TEXCOORD1;
flat in highp  uint vs_SV_InstanceID0;
layout(location = 0) out mediump vec4 SV_Target0;
vec4 u_xlat0;
mediump float u_xlat16_0;
mediump vec4 u_xlat16_1;
vec4 u_xlat2;
float u_xlat3;
vec2 u_xlat6;
int u_xlati6;
void main()
{
    u_xlat0.x = _Time.x + _TimeHelperOffset.x;
    u_xlat0.xy = u_xlat0.xx * _MainTexSpeed.xy;
    u_xlat6.xy = vs_TEXCOORD0.xy * _MainTex_ST.xy + _MainTex_ST.zw;
    u_xlat0.xy = u_xlat0.xy * _MainTex_ST.xy + u_xlat6.xy;
    u_xlat16_0 = texture(_MainTex, u_xlat0.xy).x;
    u_xlat3 = u_xlat16_0 * _Brightness + vs_TEXCOORD1;
    u_xlati6 = int(vs_SV_InstanceID0) + unity_BaseInstanceID;
    u_xlat16_1.xyz = vec3(u_xlat16_0) * vec3(vec3(_Brightness, _Brightness, _Brightness)) + PropsArray[u_xlati6]._TintColor.xyz;
    u_xlat16_1.xyz = clamp(u_xlat16_1.xyz, 0.0, 1.0);
    u_xlat16_1.w = PropsArray[u_xlati6]._TintColor.w;
    u_xlat16_1.w = clamp(u_xlat16_1.w, 0.0, 1.0);
    u_xlat2 = (-u_xlat16_1) + vec4(1.0, 1.0, 1.0, 1.0);
    u_xlat0 = vec4(u_xlat3) * u_xlat2 + u_xlat16_1;
    SV_Target0 = u_xlat0;
    return;
}