//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

//--------------------------------------------------------------------------------------
// File: ForwardPlus11.hlsl
//
// HLSL file for the ForwardPlus11 sample. Depth pre-pass and forward rendering.
//--------------------------------------------------------------------------------------


#include "ForwardPlus11Common.hlsl"


//-----------------------------------------------------------------------------------------
// Textures, Samplers, and Buffers
//-----------------------------------------------------------------------------------------
Texture2D    g_TxDiffuse : register( t0 );
Texture2D    g_TxNormal  : register( t1 );
SamplerState g_Sampler   : register( s0 );

// Save two slots for CDXUTSDKMesh diffuse and normal, 
// so start with the third slot, t2
Buffer<float4> g_PointLightBufferCenterAndRadius : register( t2 );
Buffer<float4> g_PointLightBufferColor           : register( t3 );
Buffer<float4> g_SpotLightBufferCenterAndRadius  : register( t4 );
Buffer<float4> g_SpotLightBufferColor            : register( t5 );
Buffer<float4> g_SpotLightBufferSpotParams       : register( t6 );
Buffer<uint>   g_PerTileLightIndexBuffer         : register( t7 );

//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------
struct VS_INPUT_SCENE
{
    float3 Position     : POSITION;  // vertex position
    float3 Normal       : NORMAL;    // vertex normal vector
    float2 TextureUV    : TEXCOORD0; // vertex texture coords
    float3 Tangent      : TANGENT;   // vertex tangent vector
};

struct VS_OUTPUT_SCENE
{
    float4 Position     : SV_POSITION; // vertex position
    float3 Normal       : NORMAL;      // vertex normal vector
    float2 TextureUV    : TEXCOORD0;   // vertex texture coords
    float3 Tangent      : TEXCOORD1;   // vertex tangent vector
    float3 PositionWS   : TEXCOORD2;   // vertex position (world space)
};

struct VS_OUTPUT_POSITION_ONLY
{
    float4 Position     : SV_POSITION; // vertex position 
};

struct VS_OUTPUT_POSITION_AND_TEX
{
    float4 Position     : SV_POSITION; // vertex position 
    float2 TextureUV    : TEXCOORD0;   // vertex texture coords
};

//--------------------------------------------------------------------------------------
// This shader just transforms position (e.g. for depth pre-pass)
//--------------------------------------------------------------------------------------
VS_OUTPUT_POSITION_ONLY RenderScenePositionOnlyVS( VS_INPUT_SCENE Input )
{
    VS_OUTPUT_POSITION_ONLY Output;
    
    // Transform the position from object space to homogeneous projection space
    Output.Position = mul( float4(Input.Position,1), g_mWorldViewProjection );
    
    return Output;
}

//--------------------------------------------------------------------------------------
// This shader just transforms position and passes through tex coord 
// (e.g. for depth pre-pass with alpha test)
//--------------------------------------------------------------------------------------
VS_OUTPUT_POSITION_AND_TEX RenderScenePositionAndTexVS( VS_INPUT_SCENE Input )
{
    VS_OUTPUT_POSITION_AND_TEX Output;
    
    // Transform the position from object space to homogeneous projection space
    Output.Position = mul( float4(Input.Position,1), g_mWorldViewProjection );
    
    // Just copy the texture coordinate through
    Output.TextureUV = Input.TextureUV; 
    
    return Output;
}

//--------------------------------------------------------------------------------------
// This shader transforms position, calculates world-space position, normal, 
// and tangent, and passes tex coords through to the pixel shader.
//--------------------------------------------------------------------------------------
VS_OUTPUT_SCENE RenderSceneVS( VS_INPUT_SCENE Input )
{
    VS_OUTPUT_SCENE Output;
    
    // Transform the position from object space to homogeneous projection space
    Output.Position = mul( float4(Input.Position,1), g_mWorldViewProjection );

    // Position, normal, and tangent in world space
    Output.PositionWS = mul( Input.Position, (float3x3)g_mWorld );
    Output.Normal = mul( Input.Normal, (float3x3)g_mWorld );
    Output.Tangent = mul( Input.Tangent, (float3x3)g_mWorld );
    
    // Just copy the texture coordinate through
    Output.TextureUV = Input.TextureUV; 
    
    return Output;
}

//--------------------------------------------------------------------------------------
// This shader does alpha testing.
//--------------------------------------------------------------------------------------
float4 RenderSceneAlphaTestOnlyPS( VS_OUTPUT_POSITION_AND_TEX Input ) : SV_TARGET
{ 
    float4 DiffuseTex = g_TxDiffuse.Sample( g_Sampler, Input.TextureUV );
    float fAlpha = DiffuseTex.a;
    if( fAlpha < g_fAlphaTest ) discard;
    return DiffuseTex;
}

//--------------------------------------------------------------------------------------
// This shader calculates diffuse and specular lighting for all lights.
//--------------------------------------------------------------------------------------
float4 RenderScenePS( VS_OUTPUT_SCENE Input ) : SV_TARGET
{ 
    float3 vPositionWS = Input.PositionWS;

    float3 AccumDiffuse = float3(0,0,0);
    float3 AccumSpecular = float3(0,0,0);

    float4 DiffuseTex = g_TxDiffuse.Sample( g_Sampler, Input.TextureUV );

#if ( USE_ALPHA_TEST == 1 )
    float fSpecMask = 0.0f;
    float fAlpha = DiffuseTex.a;
    if( fAlpha < g_fAlphaTest ) discard;
#else
    float fSpecMask = DiffuseTex.a;
#endif

    // get normal from normal map
    float3 vNorm = g_TxNormal.Sample( g_Sampler, Input.TextureUV ).xyz;
    vNorm *= 2;
    vNorm -= float3(1,1,1);
    
    // transform normal into world space
    float3 vBinorm = normalize( cross( Input.Normal, Input.Tangent ) );
    float3x3 BTNMatrix = float3x3( vBinorm, Input.Tangent, Input.Normal );
    vNorm = normalize(mul( vNorm, BTNMatrix ));

    float3 vViewDir = normalize( g_vCameraPos - vPositionWS );

#if ( USE_LIGHT_CULLING == 1 )
    uint nTileIndex = GetTileIndex(Input.Position.xy);
    uint nIndex = g_uMaxNumLightsPerTile*nTileIndex;
    uint nNextLightIndex = g_PerTileLightIndexBuffer[nIndex];
#else
    uint nIndex;
    uint nNumPointLights = g_uNumLights & 0xFFFFu;
#endif

    // loop over the point lights

    [loop]
#if ( USE_LIGHT_CULLING == 1 )
    while ( nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL )
#else
    for ( nIndex = 0; nIndex < nNumPointLights; nIndex++ )
#endif
    {
#if ( USE_LIGHT_CULLING == 1 )
        uint nLightIndex = nNextLightIndex;
        nIndex++;
        nNextLightIndex = g_PerTileLightIndexBuffer[nIndex];
#else
        uint nLightIndex = nIndex;
#endif
        float4 CenterAndRadius = g_PointLightBufferCenterAndRadius[nLightIndex];

        float3 vToLight = CenterAndRadius.xyz - vPositionWS.xyz;
        float3 vLightDir = normalize(vToLight);
        float fLightDistance = length(vToLight);

        float3 LightColorDiffuse = float3(0,0,0);
        float3 LightColorSpecular = float3(0,0,0);

        float fRad = CenterAndRadius.w;
        if( fLightDistance < fRad )
        {
            float x = fLightDistance / fRad;
            // fake inverse squared falloff:
            // -(1/k)*(1-(k+1)/(1+k*x^2))
            // k=20: -(1/20)*(1 - 21/(1+20*x^2))
            float fFalloff = -0.05 + 1.05/(1+20*x*x);
            LightColorDiffuse = g_PointLightBufferColor[nLightIndex].rgb * saturate(dot(vLightDir,vNorm)) * fFalloff;

            float3 vHalfAngle = normalize( vViewDir + vLightDir );
            LightColorSpecular = g_PointLightBufferColor[nLightIndex].rgb * pow( saturate(dot( vHalfAngle, vNorm )), 8 ) * fFalloff;
        }

        AccumDiffuse += LightColorDiffuse;
        AccumSpecular += LightColorSpecular;
    }

#if ( USE_LIGHT_CULLING == 1 )
    // move past the first sentinel to get to the spot lights
    nIndex++;
    nNextLightIndex = g_PerTileLightIndexBuffer[nIndex];
#else
    uint nNumSpotLights = (g_uNumLights & 0xFFFF0000u) >> 16;
#endif

    // loop over the spot lights

    [loop]
#if ( USE_LIGHT_CULLING == 1 )
    while ( nNextLightIndex != LIGHT_INDEX_BUFFER_SENTINEL )
#else
    for ( nIndex = 0; nIndex < nNumSpotLights; nIndex++ )
#endif
    {
#if ( USE_LIGHT_CULLING == 1 )
        uint nLightIndex = nNextLightIndex;
        nIndex++;
        nNextLightIndex = g_PerTileLightIndexBuffer[nIndex];
#else
        uint nLightIndex = nIndex;
#endif
        float4 BoundingSphereCenterAndRadius = g_SpotLightBufferCenterAndRadius[nLightIndex];
        float4 SpotParams = g_SpotLightBufferSpotParams[nLightIndex];

        // reconstruct z component of the light dir from x and y
        float3 SpotLightDir;
        SpotLightDir.xy = SpotParams.xy;
        SpotLightDir.z = sqrt(1 - SpotLightDir.x*SpotLightDir.x - SpotLightDir.y*SpotLightDir.y);

        // the sign bit for cone angle is used to store the sign for the z component of the light dir
        SpotLightDir.z = (SpotParams.z > 0) ? SpotLightDir.z : -SpotLightDir.z;

        // calculate the light position from the bounding sphere (we know the top of the cone is 
        // r_bounding_sphere units away from the bounding sphere center along the negated light direction)
        float3 LightPosition = BoundingSphereCenterAndRadius.xyz - BoundingSphereCenterAndRadius.w*SpotLightDir;

        float3 vToLight = LightPosition.xyz - vPositionWS.xyz;
        float3 vToLightNormalized = normalize(vToLight);
        float fLightDistance = length(vToLight);
        float fCosineOfCurrentConeAngle = dot(-vToLightNormalized, SpotLightDir);

        float3 LightColorDiffuse = float3(0,0,0);
        float3 LightColorSpecular = float3(0,0,0);

        float fRad = SpotParams.w;
        float fCosineOfConeAngle = (SpotParams.z > 0) ? SpotParams.z : -SpotParams.z;
        if( fLightDistance < fRad && fCosineOfCurrentConeAngle > fCosineOfConeAngle)
        {
            float fRadialAttenuation = (fCosineOfCurrentConeAngle - fCosineOfConeAngle) / (1.0 - fCosineOfConeAngle);
            fRadialAttenuation = fRadialAttenuation * fRadialAttenuation;

            float x = fLightDistance / fRad;
            // fake inverse squared falloff:
            // -(1/k)*(1-(k+1)/(1+k*x^2))
            // k=20: -(1/20)*(1 - 21/(1+20*x^2))
            float fFalloff = -0.05 + 1.05/(1+20*x*x);
            LightColorDiffuse = g_SpotLightBufferColor[nLightIndex].rgb * saturate(dot(vToLightNormalized,vNorm)) * fFalloff * fRadialAttenuation;

            float3 vHalfAngle = normalize( vViewDir + vToLightNormalized );
            LightColorSpecular = g_SpotLightBufferColor[nLightIndex].rgb * pow( saturate(dot( vHalfAngle, vNorm )), 8 ) * fFalloff * fRadialAttenuation;
        }

        AccumDiffuse += LightColorDiffuse;
        AccumSpecular += LightColorSpecular;
    }

    // pump up the lights
    AccumDiffuse *= 2;
    AccumSpecular *= 8;

    // This is a poor man's ambient cubemap (blend between an up color and a down color)
    float fAmbientBlend = 0.5f * vNorm.y + 0.5;
    float3 Ambient = g_MaterialAmbientColorUp.rgb * fAmbientBlend + g_MaterialAmbientColorDown.rgb * (1-fAmbientBlend);

    // modulate mesh texture with lighting
    float3 DiffuseAndAmbient = AccumDiffuse + Ambient;
    return float4(DiffuseTex.xyz*(DiffuseAndAmbient + AccumSpecular*fSpecMask),1);
}
