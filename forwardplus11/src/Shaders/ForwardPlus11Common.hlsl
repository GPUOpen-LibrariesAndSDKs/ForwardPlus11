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
// File: ForwardPlus11Common.hlsl
//
// HLSL file for the ForwardPlus11 sample. Common code.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register( b0 )
{
    matrix  g_mWorldViewProjection     : packoffset( c0 );
    matrix  g_mWorldView               : packoffset( c4 );
    matrix  g_mWorld                   : packoffset( c8 );
    float4  g_MaterialAmbientColorUp   : packoffset( c12 );
    float4  g_MaterialAmbientColorDown : packoffset( c13 );
}

cbuffer cbPerFrame : register( b1 )
{
    matrix              g_mProjection           : packoffset( c0 );
    matrix              g_mProjectionInv        : packoffset( c4 );
    float3              g_vCameraPos            : packoffset( c8 );
    float               g_fAlphaTest            : packoffset( c8.w );
    uint                g_uNumLights            : packoffset( c9 );
    uint                g_uWindowWidth          : packoffset( c9.y );
    uint                g_uWindowHeight         : packoffset( c9.z );
    uint                g_uMaxNumLightsPerTile  : packoffset( c9.w );
};

//--------------------------------------------------------------------------------------
// Miscellaneous constants
//--------------------------------------------------------------------------------------
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

//--------------------------------------------------------------------------------------
// Light culling constants.
// These must match their counterparts in ForwardPlusUtil.h
//--------------------------------------------------------------------------------------
#define TILE_RES 16
#define MAX_NUM_LIGHTS_PER_TILE 544

//-----------------------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------------------
uint GetTileIndex(float2 ScreenPos)
{
    float fTileRes = (float)TILE_RES;
    uint nNumCellsX =  (g_uWindowWidth + TILE_RES - 1)/TILE_RES;
    uint nTileIdx = floor(ScreenPos.x/fTileRes)+floor(ScreenPos.y/fTileRes)*nNumCellsX;
    return nTileIdx;
}

