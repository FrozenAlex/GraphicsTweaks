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
uniform 	int unity_BaseInstanceID;
uniform 	mediump float _CustomFogAttenuation;
uniform 	mediump float _CustomFogOffset;
uniform 	mediump float _CustomFogHeightFogStartY;
uniform 	mediump float _CustomFogHeightFogHeight;
uniform 	mediump float _BaseColorBoost;
uniform 	mediump float _BaseColorBoostThreshold;
uniform 	float _FogStartOffset;
uniform 	float _FogScale;
uniform 	mediump float _FogHeightScale;
uniform 	mediump float _FogHeightOffset;
uniform 	float _CutoutTexScale;
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
UNITY_BINDING(0) uniform UnityStereoGlobals {
#endif
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoMatrixP[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoMatrixV[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoMatrixInvV[8];
	UNITY_UNIFORM vec4                hlslcc_mtx4x4unity_StereoMatrixVP[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoCameraProjection[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoCameraInvProjection[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoWorldToCamera[8];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXhlslcc_mtx4x4unity_StereoCameraToWorld[8];
	UNITY_UNIFORM vec3                unity_StereoWorldSpaceCameraPos[2];
	UNITY_UNIFORM vec4 Xhlslcc_UnusedXunity_StereoScaleOffset[2];
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
};
#endif
struct unity_Builtins0Array_Type {
	vec4 hlslcc_mtx4x4unity_ObjectToWorldArray[4];
	vec4 hlslcc_mtx4x4unity_WorldToObjectArray[4];
};
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
UNITY_BINDING(1) uniform UnityInstancing_PerDraw0 {
#endif
	UNITY_UNIFORM unity_Builtins0Array_Type                unity_Builtins0Array[128];
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
};
#endif
struct PropsArray_Type {
	mediump vec4 _Color;
	mediump vec4 _SizeParams;
	float _Cutout;
	vec4 _CutoutTexOffset;
};
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
UNITY_BINDING(2) uniform UnityInstancing_Props {
#endif
	UNITY_UNIFORM PropsArray_Type                PropsArray[128];
#if HLSLCC_ENABLE_UNIFORM_BUFFERS
};
#endif
UNITY_LOCATION(0) uniform mediump sampler3D _CutoutTex;
UNITY_LOCATION(1) uniform mediump sampler2D _BloomPrePassTexture;
in highp  vec3 vs_TEXCOORD1;
in mediump  vec4 vs_TEXCOORD2;
flat in highp  uint vs_SV_InstanceID0;
layout(location = 0) out mediump vec4 SV_Target0;
float u_xlat0;
ivec2 u_xlati0;
mediump float u_xlat16_1;
mediump float u_xlat16_2;
vec3 u_xlat3;
mediump vec3 u_xlat16_3;
bool u_xlatb3;
mediump vec3 u_xlat16_4;
mediump float u_xlat16_7;
void main()
{
    u_xlati0.x = int(vs_SV_InstanceID0) + unity_BaseInstanceID;
    u_xlati0.xy = ivec2(u_xlati0.x << (int(2) & int(0x1F)), u_xlati0.x << (int(3) & int(0x1F)));
    u_xlat3.xyz = vs_TEXCOORD1.xyz + (-unity_Builtins0Array[u_xlati0.y / 8].hlslcc_mtx4x4unity_ObjectToWorldArray[3].xyz);
    u_xlat3.xyz = u_xlat3.xyz + PropsArray[u_xlati0.x / 4]._CutoutTexOffset.xyz;
    u_xlat3.xyz = u_xlat3.xyz * vec3(_CutoutTexScale);
    u_xlat16_3.x = texture(_CutoutTex, u_xlat3.xyz).w;
    u_xlat3.x = (-PropsArray[u_xlati0.x / 4]._Cutout) * 1.10000002 + u_xlat16_3.x;
    u_xlat3.x = u_xlat3.x + 0.100000001;
    u_xlatb3 = u_xlat3.x<0.0;
    if(u_xlatb3){discard;}
    u_xlat3.xyz = vs_TEXCOORD1.xyz + (-unity_StereoWorldSpaceCameraPos[0].xyz);
    u_xlat3.x = dot(u_xlat3.xyz, u_xlat3.xyz);
    u_xlat16_1 = u_xlat3.x + (-_FogStartOffset);
    u_xlat16_1 = max(u_xlat16_1, 0.0);
    u_xlat16_1 = u_xlat16_1 * _FogScale + (-_CustomFogOffset);
    u_xlat16_1 = max(u_xlat16_1, 0.0);
    u_xlat16_1 = u_xlat16_1 * _CustomFogAttenuation + 1.0;
    u_xlat16_1 = float(1.0) / u_xlat16_1;
    u_xlat3.x = vs_TEXCOORD1.y * _FogHeightScale + _FogHeightOffset;
    u_xlat16_4.x = _CustomFogHeightFogHeight + _CustomFogHeightFogStartY;
    u_xlat16_4.x = u_xlat3.x + (-u_xlat16_4.x);
    u_xlat16_4.x = u_xlat16_4.x / _CustomFogHeightFogHeight;
    u_xlat16_4.x = clamp(u_xlat16_4.x, 0.0, 1.0);
    u_xlat16_7 = u_xlat16_4.x * u_xlat16_4.x;
    u_xlat16_4.x = (-u_xlat16_4.x) * 2.0 + 3.0;
    u_xlat16_4.x = u_xlat16_4.x * u_xlat16_7;
    u_xlat16_1 = u_xlat16_4.x * (-u_xlat16_1) + 1.0;
    u_xlat16_4.xy = vs_TEXCOORD2.xy / vs_TEXCOORD2.ww;
    u_xlat16_3.xyz = texture(_BloomPrePassTexture, u_xlat16_4.xy).xyz;
    u_xlat16_4.xyz = u_xlat16_3.xyz + (-PropsArray[u_xlati0.x / 4]._Color.xyz);
    SV_Target0.xyz = vec3(u_xlat16_1) * u_xlat16_4.xyz + PropsArray[u_xlati0.x / 4]._Color.xyz;
    u_xlat0 = PropsArray[u_xlati0.x / 4]._Color.w + PropsArray[u_xlati0.x / 4]._Color.w;
    SV_Target0.w = u_xlat16_1 * (-u_xlat0) + u_xlat0;
    return;
}