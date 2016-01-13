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
// File: ForwardPlusUtil.cpp
//
// Helper functions for the ForwardPlus11 sample.
//--------------------------------------------------------------------------------------

#include "..\\..\\DXUT\\Core\\DXUT.h"
#include "..\\..\\DXUT\\Optional\\SDKmisc.h"

#include "..\\..\\AMD_SDK\\inc\\AMD_SDK.h"

#include "ForwardPlusUtil.h"

#pragma warning( disable : 4100 ) // disable unreference formal parameter warnings for /W4 builds

using namespace DirectX;

struct SpriteVertex
{
    XMFLOAT3 v3Pos;
    XMFLOAT2 v2TexCoord;
};

// static arrays for sprite quad vertex data
static SpriteVertex         g_QuadForLightsVertexData[6];
static SpriteVertex         g_QuadForLegendVertexData[6];

struct ConeVertex
{
    XMFLOAT3 v3Pos;
    XMFLOAT3 v3Norm;
    XMFLOAT2 v2TexCoord;
};

// static arrays for cone vertex and index data (for visualizing spot lights)
static const int            g_nConeNumTris = 90;
static const int            g_nConeNumVertices = 2*g_nConeNumTris;
static const int            g_nConeNumIndices = 3*g_nConeNumTris;
static ConeVertex           g_ConeForSpotLightsVertexData[g_nConeNumVertices];
static unsigned short       g_ConeForSpotLightsIndexData[g_nConeNumIndices];

// these are half-precision (i.e. 16-bit) float values, 
// stored as unsigned shorts
struct SpotParams
{
    unsigned short fLightDirX;
    unsigned short fLightDirY;
    unsigned short fCosineOfConeAngleAndLightDirZSign;
    unsigned short fFalloffRadius;
};

// static arrays for the point light data
static XMFLOAT4             g_PointLightDataArrayCenterAndRadius[ForwardPlus11::MAX_NUM_LIGHTS];
static DWORD                g_PointLightDataArrayColor[ForwardPlus11::MAX_NUM_LIGHTS];

// static arrays for the spot light data
static XMFLOAT4             g_SpotLightDataArrayCenterAndRadius[ForwardPlus11::MAX_NUM_LIGHTS];
static DWORD                g_SpotLightDataArrayColor[ForwardPlus11::MAX_NUM_LIGHTS];
static SpotParams           g_SpotLightDataArraySpotParams[ForwardPlus11::MAX_NUM_LIGHTS];

// rotation matrices used when visualizing the spot lights
static XMMATRIX             g_SpotLightDataArraySpotMatrices[ForwardPlus11::MAX_NUM_LIGHTS];

// constants for the legend for the lights-per-tile visualization
static const int g_nLegendNumLines = 17;
static const int g_nLegendTextureWidth = 32;
static const int g_nLegendPaddingLeft = 5;
static const int g_nLegendPaddingBottom = 2*AMD::HUD::iElementDelta;

// miscellaneous constants
static const float TWO_PI = 6.28318530718f;

static float GetRandFloat( float fRangeMin, float fRangeMax )
{
    // Generate random numbers in the half-closed interval
    // [rangeMin, rangeMax). In other words,
    // rangeMin <= random number < rangeMax
    return  (float)rand() / (RAND_MAX + 1) * (fRangeMax - fRangeMin) + fRangeMin;
}

static DWORD GetRandColor()
{
    static unsigned uCounter = 0;
    uCounter++;

    XMFLOAT4 Color;
    if( uCounter%2 == 0 )
    {
        // since green contributes the most to perceived brightness, 
        // cap it's min value to avoid overly dim lights
        Color = XMFLOAT4(GetRandFloat(0.0f,1.0f),GetRandFloat(0.27f,1.0f),GetRandFloat(0.0f,1.0f),1.0f);
    }
    else
    {
        // else ensure the red component has a large value, again 
        // to avoid overly dim lights
        Color = XMFLOAT4(GetRandFloat(0.9f,1.0f),GetRandFloat(0.0f,1.0f),GetRandFloat(0.0f,1.0f),1.0f);
    }

    DWORD dwR = (DWORD)(Color.x * 255.0f + 0.5f);
    DWORD dwG = (DWORD)(Color.y * 255.0f + 0.5f);
    DWORD dwB = (DWORD)(Color.z * 255.0f + 0.5f);
    DWORD dwA = (DWORD)(Color.w * 255.0f + 0.5f);

    // ABGR format (i.e. DXGI_FORMAT_R8G8B8A8_UNORM)
    return (dwA << 24) | (dwB << 16) | (dwG << 8) | dwR;
}

static XMFLOAT3 GetRandLightDirection()
{
    static unsigned uCounter = 0;
    uCounter++;

    XMFLOAT3 vLightDir;
    vLightDir.x = GetRandFloat(-1.0f,1.0f);
    vLightDir.y = GetRandFloat( 0.1f,1.0f);
    vLightDir.z = GetRandFloat(-1.0f,1.0f);

    if( uCounter%2 == 0 )
    {
        vLightDir.y = -vLightDir.y;
    }

    XMFLOAT3 vResult;
    XMVECTOR NormalizedLightDir = XMVector3Normalize( XMLoadFloat3( &vLightDir) );
    XMStoreFloat3( &vResult, NormalizedLightDir );

    return vResult;
}

static SpotParams PackSpotParams(const XMFLOAT3& vLightDir, float fCosineOfConeAngle, float fFalloffRadius)
{
    assert( fCosineOfConeAngle > 0.0f );
    assert( fFalloffRadius > 0.0f );

    SpotParams PackedParams;
    PackedParams.fLightDirX = AMD::ConvertF32ToF16( vLightDir.x );
    PackedParams.fLightDirY = AMD::ConvertF32ToF16( vLightDir.y );
    PackedParams.fCosineOfConeAngleAndLightDirZSign = AMD::ConvertF32ToF16( fCosineOfConeAngle );
    PackedParams.fFalloffRadius = AMD::ConvertF32ToF16( fFalloffRadius );

    // put the sign bit for light dir z in the sign bit for the cone angle
    // (we can do this because we know the cone angle is always positive)
    if( vLightDir.z < 0.0f )
    {
        PackedParams.fCosineOfConeAngleAndLightDirZSign |= 0x8000;
    }
    else
    {
        PackedParams.fCosineOfConeAngleAndLightDirZSign &= 0x7FFF;
    }

    return PackedParams;
}

namespace ForwardPlus11
{

    //--------------------------------------------------------------------------------------
    // Constructor
    //--------------------------------------------------------------------------------------
    ForwardPlusUtil::ForwardPlusUtil()
        :m_uWidth(0)
        ,m_uHeight(0)
        ,m_pPointLightBufferCenterAndRadius(NULL)
        ,m_pPointLightBufferCenterAndRadiusSRV(NULL)
        ,m_pPointLightBufferColor(NULL)
        ,m_pPointLightBufferColorSRV(NULL)
        ,m_pSpotLightBufferCenterAndRadius(NULL)
        ,m_pSpotLightBufferCenterAndRadiusSRV(NULL)
        ,m_pSpotLightBufferColor(NULL)
        ,m_pSpotLightBufferColorSRV(NULL)
        ,m_pSpotLightBufferSpotParams(NULL)
        ,m_pSpotLightBufferSpotParamsSRV(NULL)
        ,m_pSpotLightBufferSpotMatrices(NULL)
        ,m_pSpotLightBufferSpotMatricesSRV(NULL)
        ,m_pLightIndexBuffer(NULL)
        ,m_pLightIndexBufferSRV(NULL)
        ,m_pLightIndexBufferUAV(NULL)
        ,m_pQuadForLightsVB(NULL)
        ,m_pQuadForLegendVB(NULL)
        ,m_pConeForSpotLightsVB(NULL)
        ,m_pConeForSpotLightsIB(NULL)
        ,m_pDebugDrawPointLightsVS(NULL)
        ,m_pDebugDrawPointLightsPS(NULL)
        ,m_pDebugDrawPointLightsLayout11(NULL)
        ,m_pDebugDrawSpotLightsVS(NULL)
        ,m_pDebugDrawSpotLightsPS(NULL)
        ,m_pDebugDrawSpotLightsLayout11(NULL)
        ,m_pDebugDrawLegendForNumLightsPerTileVS(NULL)
        ,m_pDebugDrawLegendForNumLightsPerTileGrayscalePS(NULL)
        ,m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS(NULL)
        ,m_pDebugDrawLegendForNumLightsLayout11(NULL)
        ,m_pAdditiveBlendingState(NULL)
        ,m_pDisableDepthWrite(NULL)
        ,m_pDisableDepthTest(NULL)
        ,m_pDisableCullingRS(NULL)
    {
    }


    //--------------------------------------------------------------------------------------
    // Destructor
    //--------------------------------------------------------------------------------------
    ForwardPlusUtil::~ForwardPlusUtil()
    {
        SAFE_RELEASE(m_pPointLightBufferCenterAndRadius);
        SAFE_RELEASE(m_pPointLightBufferCenterAndRadiusSRV);
        SAFE_RELEASE(m_pPointLightBufferColor);
        SAFE_RELEASE(m_pPointLightBufferColorSRV);
        SAFE_RELEASE(m_pSpotLightBufferCenterAndRadius);
        SAFE_RELEASE(m_pSpotLightBufferCenterAndRadiusSRV);
        SAFE_RELEASE(m_pSpotLightBufferColor);
        SAFE_RELEASE(m_pSpotLightBufferColorSRV);
        SAFE_RELEASE(m_pSpotLightBufferSpotParams);
        SAFE_RELEASE(m_pSpotLightBufferSpotParamsSRV);
        SAFE_RELEASE(m_pSpotLightBufferSpotMatrices);
        SAFE_RELEASE(m_pSpotLightBufferSpotMatricesSRV);
        SAFE_RELEASE(m_pLightIndexBuffer);
        SAFE_RELEASE(m_pLightIndexBufferSRV);
        SAFE_RELEASE(m_pLightIndexBufferUAV);
        SAFE_RELEASE(m_pQuadForLightsVB);
        SAFE_RELEASE(m_pQuadForLegendVB);
        SAFE_RELEASE(m_pConeForSpotLightsVB);
        SAFE_RELEASE(m_pConeForSpotLightsIB);
        SAFE_RELEASE(m_pDebugDrawPointLightsVS);
        SAFE_RELEASE(m_pDebugDrawPointLightsPS);
        SAFE_RELEASE(m_pDebugDrawPointLightsLayout11);
        SAFE_RELEASE(m_pDebugDrawSpotLightsVS);
        SAFE_RELEASE(m_pDebugDrawSpotLightsPS);
        SAFE_RELEASE(m_pDebugDrawSpotLightsLayout11);
        SAFE_RELEASE(m_pDebugDrawLegendForNumLightsPerTileVS);
        SAFE_RELEASE(m_pDebugDrawLegendForNumLightsPerTileGrayscalePS);
        SAFE_RELEASE(m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS);
        SAFE_RELEASE(m_pDebugDrawLegendForNumLightsLayout11);
        SAFE_RELEASE(m_pAdditiveBlendingState);
        SAFE_RELEASE(m_pDisableDepthWrite);
        SAFE_RELEASE(m_pDisableDepthTest);
        SAFE_RELEASE(m_pDisableCullingRS);
    }

    //--------------------------------------------------------------------------------------
    // Device creation hook function
    //--------------------------------------------------------------------------------------
    HRESULT ForwardPlusUtil::OnCreateDevice( ID3D11Device* pd3dDevice )
    {
        HRESULT hr;

        D3D11_SUBRESOURCE_DATA InitData;

        // Create the point light buffer (center and radius)
        D3D11_BUFFER_DESC LightBufferDesc;
        ZeroMemory( &LightBufferDesc, sizeof(LightBufferDesc) );
        LightBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        LightBufferDesc.ByteWidth = sizeof( g_PointLightDataArrayCenterAndRadius );
        LightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        InitData.pSysMem = g_PointLightDataArrayCenterAndRadius;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pPointLightBufferCenterAndRadius ) );
        DXUT_SetDebugName( m_pPointLightBufferCenterAndRadius, "PointLightBufferCenterAndRadius" );

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        SRVDesc.Buffer.ElementOffset = 0;
        SRVDesc.Buffer.ElementWidth = MAX_NUM_LIGHTS;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pPointLightBufferCenterAndRadius, &SRVDesc, &m_pPointLightBufferCenterAndRadiusSRV ) );

        // Create the point light buffer (color)
        LightBufferDesc.ByteWidth = sizeof( g_PointLightDataArrayColor );
        InitData.pSysMem = g_PointLightDataArrayColor;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pPointLightBufferColor ) );
        DXUT_SetDebugName( m_pPointLightBufferColor, "PointLightBufferColor" );

        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pPointLightBufferColor, &SRVDesc, &m_pPointLightBufferColorSRV ) );

        // Create the spot light buffer (center and radius)
        ZeroMemory( &LightBufferDesc, sizeof(LightBufferDesc) );
        LightBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        LightBufferDesc.ByteWidth = sizeof( g_SpotLightDataArrayCenterAndRadius );
        LightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        InitData.pSysMem = g_SpotLightDataArrayCenterAndRadius;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pSpotLightBufferCenterAndRadius ) );
        DXUT_SetDebugName( m_pSpotLightBufferCenterAndRadius, "SpotLightBufferCenterAndRadius" );

        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        SRVDesc.Buffer.ElementOffset = 0;
        SRVDesc.Buffer.ElementWidth = MAX_NUM_LIGHTS;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pSpotLightBufferCenterAndRadius, &SRVDesc, &m_pSpotLightBufferCenterAndRadiusSRV ) );

        // Create the spot light buffer (color)
        LightBufferDesc.ByteWidth = sizeof( g_SpotLightDataArrayColor );
        InitData.pSysMem = g_SpotLightDataArrayColor;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pSpotLightBufferColor ) );
        DXUT_SetDebugName( m_pSpotLightBufferColor, "SpotLightBufferColor" );

        SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pSpotLightBufferColor, &SRVDesc, &m_pSpotLightBufferColorSRV ) );

        // Create the spot light buffer (spot light parameters)
        LightBufferDesc.ByteWidth = sizeof( g_SpotLightDataArraySpotParams );
        InitData.pSysMem = g_SpotLightDataArraySpotParams;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pSpotLightBufferSpotParams ) );
        DXUT_SetDebugName( m_pSpotLightBufferSpotParams, "SpotLightBufferSpotParams" );

        SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pSpotLightBufferSpotParams, &SRVDesc, &m_pSpotLightBufferSpotParamsSRV ) );

        // Create the light buffer (spot light matrices, only used for debug drawing the spot lights)
        LightBufferDesc.ByteWidth = sizeof( g_SpotLightDataArraySpotMatrices );
        InitData.pSysMem = g_SpotLightDataArraySpotMatrices;
        V_RETURN( pd3dDevice->CreateBuffer( &LightBufferDesc, &InitData, &m_pSpotLightBufferSpotMatrices ) );
        DXUT_SetDebugName( m_pSpotLightBufferSpotMatrices, "SpotLightBufferSpotMatrices" );

        SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pSpotLightBufferSpotMatrices, &SRVDesc, &m_pSpotLightBufferSpotMatricesSRV ) );

        // Create the vertex buffer for the sprites (a single quad)
        D3D11_BUFFER_DESC VBDesc;
        ZeroMemory( &VBDesc, sizeof(VBDesc) );
        VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
        VBDesc.ByteWidth = sizeof( g_QuadForLightsVertexData );
        VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        InitData.pSysMem = g_QuadForLightsVertexData;
        V_RETURN( pd3dDevice->CreateBuffer( &VBDesc, &InitData, &m_pQuadForLightsVB ) );
        DXUT_SetDebugName( m_pQuadForLightsVB, "QuadForLightsVB" );

        // Create the vertex buffer for the cone
        VBDesc.ByteWidth = sizeof( g_ConeForSpotLightsVertexData );
        InitData.pSysMem = g_ConeForSpotLightsVertexData;
        V_RETURN( pd3dDevice->CreateBuffer( &VBDesc, &InitData, &m_pConeForSpotLightsVB ) );
        DXUT_SetDebugName( m_pConeForSpotLightsVB, "ConeForSpotLightsVB" );

        // Create the index buffer for the cone
        D3D11_BUFFER_DESC IBDesc;
        ZeroMemory( &IBDesc, sizeof(IBDesc) );
        IBDesc.Usage = D3D11_USAGE_IMMUTABLE;
        IBDesc.ByteWidth = sizeof( g_ConeForSpotLightsIndexData );
        IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        InitData.pSysMem = g_ConeForSpotLightsIndexData;
        V_RETURN( pd3dDevice->CreateBuffer( &IBDesc, &InitData, &m_pConeForSpotLightsIB ) );
        DXUT_SetDebugName( m_pConeForSpotLightsIB, "ConeForSpotLightsIB" );

        // Create blend states 
        D3D11_BLEND_DESC BlendStateDesc;
        BlendStateDesc.AlphaToCoverageEnable = FALSE;
        BlendStateDesc.IndependentBlendEnable = FALSE;
        BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
        BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE; 
        BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; 
        BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO; 
        BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE; 
        BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        V_RETURN( pd3dDevice->CreateBlendState( &BlendStateDesc, &m_pAdditiveBlendingState ) );

        // Disable depth test write
        D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
        DepthStencilDesc.DepthEnable = TRUE; 
        DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
        DepthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;  // we are using inverted 32-bit float depth for better precision
        DepthStencilDesc.StencilEnable = FALSE; 
        DepthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK; 
        DepthStencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK; 
        V_RETURN( pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &m_pDisableDepthWrite ) );

        // Disable depth test
        DepthStencilDesc.DepthEnable = FALSE; 
        V_RETURN( pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &m_pDisableDepthTest ) );

        // Disable culling
        D3D11_RASTERIZER_DESC RasterizerDesc;
        RasterizerDesc.FillMode = D3D11_FILL_SOLID;
        RasterizerDesc.CullMode = D3D11_CULL_NONE;  // disable culling
        RasterizerDesc.FrontCounterClockwise = FALSE;
        RasterizerDesc.DepthBias = 0;
        RasterizerDesc.DepthBiasClamp = 0.0f;
        RasterizerDesc.SlopeScaledDepthBias = 0.0f;
        RasterizerDesc.DepthClipEnable = TRUE;
        RasterizerDesc.ScissorEnable = FALSE;
        RasterizerDesc.MultisampleEnable = FALSE;
        RasterizerDesc.AntialiasedLineEnable = FALSE;
        V_RETURN( pd3dDevice->CreateRasterizerState( &RasterizerDesc, &m_pDisableCullingRS ) );

        return hr;
    }


    //--------------------------------------------------------------------------------------
    // Device destruction hook function
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::OnDestroyDevice()
    {
        SAFE_RELEASE( m_pPointLightBufferCenterAndRadius );
        SAFE_RELEASE( m_pPointLightBufferCenterAndRadiusSRV );
        SAFE_RELEASE( m_pPointLightBufferColor );
        SAFE_RELEASE( m_pPointLightBufferColorSRV );
        SAFE_RELEASE( m_pSpotLightBufferCenterAndRadius );
        SAFE_RELEASE( m_pSpotLightBufferCenterAndRadiusSRV );
        SAFE_RELEASE( m_pSpotLightBufferColor );
        SAFE_RELEASE( m_pSpotLightBufferColorSRV );
        SAFE_RELEASE( m_pSpotLightBufferSpotParams );
        SAFE_RELEASE( m_pSpotLightBufferSpotParamsSRV );
        SAFE_RELEASE( m_pSpotLightBufferSpotMatrices );
        SAFE_RELEASE( m_pSpotLightBufferSpotMatricesSRV );

        SAFE_RELEASE( m_pQuadForLightsVB );
        SAFE_RELEASE( m_pQuadForLegendVB );
        SAFE_RELEASE( m_pConeForSpotLightsVB );
        SAFE_RELEASE( m_pConeForSpotLightsIB );

        SAFE_RELEASE( m_pDebugDrawPointLightsVS );
        SAFE_RELEASE( m_pDebugDrawPointLightsPS );
        SAFE_RELEASE( m_pDebugDrawPointLightsLayout11 );

        SAFE_RELEASE( m_pDebugDrawSpotLightsVS );
        SAFE_RELEASE( m_pDebugDrawSpotLightsPS );
        SAFE_RELEASE( m_pDebugDrawSpotLightsLayout11 );

        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileVS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileGrayscalePS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsLayout11 );

        SAFE_RELEASE( m_pAdditiveBlendingState );
        SAFE_RELEASE( m_pDisableDepthWrite );
        SAFE_RELEASE( m_pDisableDepthTest );
        SAFE_RELEASE( m_pDisableCullingRS );
    }


    //--------------------------------------------------------------------------------------
    // Resized swap chain hook function
    //--------------------------------------------------------------------------------------
    HRESULT ForwardPlusUtil::OnResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, int nLineHeight )
    {
        HRESULT hr;

        m_uWidth = pBackBufferSurfaceDesc->Width;
        m_uHeight = pBackBufferSurfaceDesc->Height;

        // depends on m_uWidth and m_uHeight, so don't do this 
        // until you have updated them (see above)
        unsigned uNumTiles = GetNumTilesX()*GetNumTilesY();
        unsigned uMaxNumLightsPerTile = GetMaxNumLightsPerTile();

        D3D11_BUFFER_DESC BufferDesc;
        ZeroMemory( &BufferDesc, sizeof(BufferDesc) );
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.ByteWidth = 4 * uMaxNumLightsPerTile * uNumTiles;
        BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        V_RETURN( pd3dDevice->CreateBuffer( &BufferDesc, NULL, &m_pLightIndexBuffer ) );
        DXUT_SetDebugName( m_pLightIndexBuffer, "LightIndexBuffer" );

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
        ZeroMemory( &SRVDesc, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );
        SRVDesc.Format = DXGI_FORMAT_R32_UINT;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        SRVDesc.Buffer.ElementOffset = 0;
        SRVDesc.Buffer.ElementWidth = uMaxNumLightsPerTile * uNumTiles;
        V_RETURN( pd3dDevice->CreateShaderResourceView( m_pLightIndexBuffer, &SRVDesc, &m_pLightIndexBufferSRV ) );

        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
        ZeroMemory( &UAVDesc, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );
        UAVDesc.Format = DXGI_FORMAT_R32_UINT;
        UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        UAVDesc.Buffer.FirstElement = 0;
        UAVDesc.Buffer.NumElements = uMaxNumLightsPerTile * uNumTiles;
        V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_pLightIndexBuffer, &UAVDesc, &m_pLightIndexBufferUAV ) );

        // initialize the vertex buffer data for a quad (for drawing the lights-per-tile legend)
        const float kTextureHeight = (float)g_nLegendNumLines * (float)nLineHeight;
        const float kTextureWidth = (float)g_nLegendTextureWidth;
        const float kPaddingLeft = (float)g_nLegendPaddingLeft;
        const float kPaddingBottom = (float)g_nLegendPaddingBottom;
        float fLeft = kPaddingLeft;
        float fRight = kPaddingLeft + kTextureWidth;
        float fTop = (float)m_uHeight - kPaddingBottom - kTextureHeight;
        float fBottom =(float)m_uHeight - kPaddingBottom;
        g_QuadForLegendVertexData[0].v3Pos = XMFLOAT3( fLeft,  fBottom, 0.0f );
        g_QuadForLegendVertexData[0].v2TexCoord = XMFLOAT2( 0.0f, 0.0f );
        g_QuadForLegendVertexData[1].v3Pos = XMFLOAT3( fLeft,  fTop, 0.0f );
        g_QuadForLegendVertexData[1].v2TexCoord = XMFLOAT2( 0.0f, 1.0f );
        g_QuadForLegendVertexData[2].v3Pos = XMFLOAT3( fRight, fBottom, 0.0f );
        g_QuadForLegendVertexData[2].v2TexCoord = XMFLOAT2( 1.0f, 0.0f );
        g_QuadForLegendVertexData[3].v3Pos = XMFLOAT3( fLeft,  fTop, 0.0f );
        g_QuadForLegendVertexData[3].v2TexCoord = XMFLOAT2( 0.0f, 1.0f );
        g_QuadForLegendVertexData[4].v3Pos = XMFLOAT3( fRight,  fTop, 0.0f );
        g_QuadForLegendVertexData[4].v2TexCoord = XMFLOAT2( 1.0f, 1.0f );
        g_QuadForLegendVertexData[5].v3Pos = XMFLOAT3( fRight, fBottom, 0.0f );
        g_QuadForLegendVertexData[5].v2TexCoord = XMFLOAT2( 1.0f, 0.0f );

        // Create the vertex buffer for the sprite (a single quad)
        D3D11_SUBRESOURCE_DATA InitData;
        D3D11_BUFFER_DESC VBDesc;
        ZeroMemory( &VBDesc, sizeof(VBDesc) );
        VBDesc.Usage = D3D11_USAGE_IMMUTABLE;
        VBDesc.ByteWidth = sizeof( g_QuadForLegendVertexData );
        VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        InitData.pSysMem = g_QuadForLegendVertexData;
        V_RETURN( pd3dDevice->CreateBuffer( &VBDesc, &InitData, &m_pQuadForLegendVB ) );
        DXUT_SetDebugName( m_pQuadForLegendVB, "QuadForLegendVB" );

        return S_OK;
    }

    //--------------------------------------------------------------------------------------
    // Releasing swap chain hook function
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::OnReleasingSwapChain()
    {
        SAFE_RELEASE(m_pLightIndexBuffer);
        SAFE_RELEASE(m_pLightIndexBufferSRV);
        SAFE_RELEASE(m_pLightIndexBufferUAV);
        SAFE_RELEASE(m_pQuadForLegendVB);
    }

    //--------------------------------------------------------------------------------------
    // Render hook function, to draw the lights (as instanced quads)
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::OnRender( float fElapsedTime, unsigned uNumPointLights, unsigned uNumSpotLights )
    {
        ID3D11ShaderResourceView* pNULLSRV = NULL;
        ID3D11SamplerState* pNULLSampler = NULL;

        ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();

        // save blend state (for later restore)
        ID3D11BlendState* pBlendStateStored11 = NULL;
        FLOAT afBlendFactorStored11[4];
        UINT uSampleMaskStored11;
        pd3dImmediateContext->OMGetBlendState( &pBlendStateStored11, afBlendFactorStored11, &uSampleMaskStored11 );

        // save depth state (for later restore)
        ID3D11DepthStencilState* pDepthStencilStateStored11 = NULL;
        UINT uStencilRefStored11;
        pd3dImmediateContext->OMGetDepthStencilState( &pDepthStencilStateStored11, &uStencilRefStored11 );

        // additive blending, enable depth test but don't write depth, disable culling
        float BlendFactor[4] = { 0, 0, 0, 0 };
        pd3dImmediateContext->OMSetBlendState( m_pAdditiveBlendingState, BlendFactor, 0xFFFFFFFF );
        pd3dImmediateContext->OMSetDepthStencilState( m_pDisableDepthWrite, 0x00 );
        //pd3dImmediateContext->RSSetState( m_pDisableCullingRS );

        // point lights
        {
            // Set the input layout
            pd3dImmediateContext->IASetInputLayout( m_pDebugDrawPointLightsLayout11 );

            // Set vertex buffer
            UINT uStride = sizeof( SpriteVertex );
            UINT uOffset = 0;
            pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pQuadForLightsVB, &uStride, &uOffset );

            // Set primitive topology
            pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

            pd3dImmediateContext->VSSetShader( m_pDebugDrawPointLightsVS, NULL, 0 );
            pd3dImmediateContext->VSSetShaderResources( 2, 1, &m_pPointLightBufferCenterAndRadiusSRV );
            pd3dImmediateContext->VSSetShaderResources( 3, 1, &m_pPointLightBufferColorSRV );
            pd3dImmediateContext->PSSetShader( m_pDebugDrawPointLightsPS, NULL, 0 );
            pd3dImmediateContext->PSSetShaderResources( 0, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetShaderResources( 1, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetSamplers( 0, 1, &pNULLSampler );

            pd3dImmediateContext->DrawInstanced(6,uNumPointLights,0,0);

            // restore to default
            pd3dImmediateContext->VSSetShaderResources( 2, 1, &pNULLSRV );
            pd3dImmediateContext->VSSetShaderResources( 3, 1, &pNULLSRV );
        }

        // spot lights
        {
            // Set the input layout
            pd3dImmediateContext->IASetInputLayout( m_pDebugDrawSpotLightsLayout11 );

            // Set vertex buffer
            UINT uStride = sizeof( ConeVertex );
            UINT uOffset = 0;
            pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pConeForSpotLightsVB, &uStride, &uOffset );
            pd3dImmediateContext->IASetIndexBuffer(m_pConeForSpotLightsIB, DXGI_FORMAT_R16_UINT, 0 );

            // Set primitive topology
            pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

            pd3dImmediateContext->VSSetShader( m_pDebugDrawSpotLightsVS, NULL, 0 );
            pd3dImmediateContext->VSSetShaderResources( 4, 1, &m_pSpotLightBufferCenterAndRadiusSRV );
            pd3dImmediateContext->VSSetShaderResources( 5, 1, &m_pSpotLightBufferColorSRV );
            pd3dImmediateContext->VSSetShaderResources( 6, 1, &m_pSpotLightBufferSpotParamsSRV );
            pd3dImmediateContext->VSSetShaderResources( 8, 1, &m_pSpotLightBufferSpotMatricesSRV );
            pd3dImmediateContext->PSSetShader( m_pDebugDrawSpotLightsPS, NULL, 0 );
            pd3dImmediateContext->PSSetShaderResources( 0, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetShaderResources( 1, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetSamplers( 0, 1, &pNULLSampler );

            pd3dImmediateContext->DrawIndexedInstanced(g_nConeNumIndices,uNumSpotLights,0,0,0);

            // restore to default
            pd3dImmediateContext->VSSetShaderResources( 4, 1, &pNULLSRV );
            pd3dImmediateContext->VSSetShaderResources( 5, 1, &pNULLSRV );
            pd3dImmediateContext->VSSetShaderResources( 6, 1, &pNULLSRV );
            pd3dImmediateContext->VSSetShaderResources( 8, 1, &pNULLSRV );
        }

        // restore to default
        //pd3dImmediateContext->RSSetState( NULL );

        // restore to previous
        pd3dImmediateContext->OMSetDepthStencilState( pDepthStencilStateStored11, uStencilRefStored11 );
        pd3dImmediateContext->OMSetBlendState( pBlendStateStored11, afBlendFactorStored11, uSampleMaskStored11 );
        SAFE_RELEASE( pDepthStencilStateStored11 );
        SAFE_RELEASE( pBlendStateStored11 );

    }

    //--------------------------------------------------------------------------------------
    // Calculate AABB around all meshes in the scene
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::CalculateSceneMinMax( CDXUTSDKMesh &Mesh, XMVECTOR *pBBoxMinOut, XMVECTOR *pBBoxMaxOut )
    {
        *pBBoxMaxOut = Mesh.GetMeshBBoxCenter( 0 ) + Mesh.GetMeshBBoxExtents( 0 );
        *pBBoxMinOut = Mesh.GetMeshBBoxCenter( 0 ) - Mesh.GetMeshBBoxExtents( 0 );

        for( unsigned i = 1; i < Mesh.GetNumMeshes(); i++ )
        {
            XMVECTOR vNewMax = Mesh.GetMeshBBoxCenter( i ) + Mesh.GetMeshBBoxExtents( i );
            XMVECTOR vNewMin = Mesh.GetMeshBBoxCenter( i ) - Mesh.GetMeshBBoxExtents( i );

            *pBBoxMaxOut = XMVectorMax(*pBBoxMaxOut, vNewMax);
            *pBBoxMinOut = XMVectorMin(*pBBoxMinOut, vNewMin);
        }

    }

    //--------------------------------------------------------------------------------------
    // Fill in the data for the lights (center, radius, and color).
    // Also fill in the vertex data for the sprite quad.
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::InitLights( const XMVECTOR &BBoxMin, const XMVECTOR &BBoxMax )
    {
        // init the random seed to 1, so that results are deterministic 
        // across different runs of the sample
        srand(1);

        // scale the size of the lights based on the size of the scene
        XMVECTOR BBoxExtents = 0.5f * (BBoxMax - BBoxMin);
        float fRadius = 0.075f * XMVectorGetX(XMVector3Length(BBoxExtents));

        // For point lights, the radius of the bounding sphere for the light (used for culling) 
        // and the falloff distance of the light (used for lighting) are the same. Not so for 
        // spot lights. A spot light is a right circular cone. The height of the cone is the 
        // falloff distance. We want to fit the cone of the spot light inside the bounding sphere. 
        // From calculus, we know the cone with maximum volume that can fit inside a sphere has height:
        // h_cone = (4/3)*r_sphere
        float fSpotLightFalloffRadius = 1.333333333333f * fRadius;

        XMFLOAT3 vBBoxMin, vBBoxMax;
        XMStoreFloat3( &vBBoxMin, BBoxMin );
        XMStoreFloat3( &vBBoxMax, BBoxMax );

        // initialize the point light data
        for (int i = 0; i < MAX_NUM_LIGHTS; i++)
        {
            g_PointLightDataArrayCenterAndRadius[i] = XMFLOAT4(GetRandFloat(vBBoxMin.x,vBBoxMax.x), GetRandFloat(vBBoxMin.y,vBBoxMax.y), GetRandFloat(vBBoxMin.z,vBBoxMax.z), fRadius);
            g_PointLightDataArrayColor[i] = GetRandColor();
        }

        // initialize the spot light data
        for (int i = 0; i < MAX_NUM_LIGHTS; i++)
        {
            g_SpotLightDataArrayCenterAndRadius[i] = XMFLOAT4(GetRandFloat(vBBoxMin.x,vBBoxMax.x), GetRandFloat(vBBoxMin.y,vBBoxMax.y), GetRandFloat(vBBoxMin.z,vBBoxMax.z), fRadius);
            g_SpotLightDataArrayColor[i] = GetRandColor();

            XMFLOAT3 vLightDir = GetRandLightDirection();

            // Okay, so we fit a max-volume cone inside our bounding sphere for the spot light. We need to find 
            // the cone angle for that cone. Google on "cone inside sphere" (without the quotes) to find info 
            // on how to derive these formulas for the height and radius of the max-volume cone inside a sphere.
            // h_cone = (4/3)*r_sphere
            // r_cone = sqrt(8/9)*r_sphere
            // tan(theta) = r_cone/h_cone = sqrt(2)/2 = 0.7071067811865475244
            // theta = 35.26438968 degrees
            // store the cosine of this angle: cosine(35.26438968 degrees) = 0.816496580927726

            // random direction, cosine of cone angle, falloff radius calcuated above
            g_SpotLightDataArraySpotParams[i] = PackSpotParams(vLightDir, 0.816496580927726f, fSpotLightFalloffRadius);

            // build a "rotate from one vector to another" matrix, to point the spot light 
            // cone along its light direction
            XMVECTOR s = XMVectorSet(0.0f,-1.0f,0.0f,0.0f);
            XMVECTOR t = XMLoadFloat3( &vLightDir );
            XMFLOAT3 v;
            XMStoreFloat3( &v, XMVector3Cross(s,t) );
            float e = XMVectorGetX(XMVector3Dot(s,t));
            float h = 1.0f / (1.0f + e);

            XMFLOAT4X4 f4x4Rotation;
            XMStoreFloat4x4( &f4x4Rotation, XMMatrixIdentity() );
            f4x4Rotation._11 = e + h*v.x*v.x;
            f4x4Rotation._12 = h*v.x*v.y - v.z;
            f4x4Rotation._13 = h*v.x*v.z + v.y;
            f4x4Rotation._21 = h*v.x*v.y + v.z;
            f4x4Rotation._22 = e + h*v.y*v.y;
            f4x4Rotation._23 = h*v.y*v.z - v.x;
            f4x4Rotation._31 = h*v.x*v.z - v.y;
            f4x4Rotation._32 = h*v.y*v.z + v.x;
            f4x4Rotation._33 = e + h*v.z*v.z;
            XMMATRIX mRotation = XMLoadFloat4x4( &f4x4Rotation );

            g_SpotLightDataArraySpotMatrices[i] = XMMatrixTranspose(mRotation);
        }

        // initialize the vertex buffer data for a quad (for drawing the lights)
        float fQuadHalfSize = 0.167f * fRadius;
        g_QuadForLightsVertexData[0].v3Pos = XMFLOAT3(-fQuadHalfSize, -fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[0].v2TexCoord = XMFLOAT2( 0.0f, 0.0f );
        g_QuadForLightsVertexData[1].v3Pos = XMFLOAT3(-fQuadHalfSize,  fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[1].v2TexCoord = XMFLOAT2( 0.0f, 1.0f );
        g_QuadForLightsVertexData[2].v3Pos = XMFLOAT3( fQuadHalfSize, -fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[2].v2TexCoord = XMFLOAT2( 1.0f, 0.0f );
        g_QuadForLightsVertexData[3].v3Pos = XMFLOAT3(-fQuadHalfSize,  fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[3].v2TexCoord = XMFLOAT2( 0.0f, 1.0f );
        g_QuadForLightsVertexData[4].v3Pos = XMFLOAT3( fQuadHalfSize,  fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[4].v2TexCoord = XMFLOAT2( 1.0f, 1.0f );
        g_QuadForLightsVertexData[5].v3Pos = XMFLOAT3( fQuadHalfSize, -fQuadHalfSize, 0.0f );
        g_QuadForLightsVertexData[5].v2TexCoord = XMFLOAT2( 1.0f, 0.0f );

        // initialize the vertex and index buffer data for a cone (for drawing the spot lights)
        {
            // h_cone = (4/3)*r_sphere
            // r_cone = sqrt(8/9)*r_sphere
            // Here, r_sphere is half of our point light quad size
            float fConeHeight = 1.333333333333f * fQuadHalfSize;
            float fConeRadius = 0.942809041582f * fQuadHalfSize;

            for (int i = 0; i < g_nConeNumTris; i++)
            {
                // We want to calculate points along the circle at the end of the cone.
                // The parametric equations for this circle are:
                // x=r_cone*cosine(t)
                // z=r_cone*sine(t)
                float t = ((float)i / (float)g_nConeNumTris) * TWO_PI;
                g_ConeForSpotLightsVertexData[2*i+1].v3Pos = XMFLOAT3( fConeRadius*cos(t), -fConeHeight, fConeRadius*sin(t) );
                g_ConeForSpotLightsVertexData[2*i+1].v2TexCoord = XMFLOAT2( 0.0f, 1.0f );

                // normal = (h_cone*cosine(t), r_cone, h_cone*sine(t))
                XMFLOAT3 vNormal = XMFLOAT3( fConeHeight*cos(t), fConeRadius, fConeHeight*sin(t) );
                XMStoreFloat3( &vNormal, XMVector3Normalize( XMLoadFloat3( &vNormal ) ) );
                g_ConeForSpotLightsVertexData[2*i+1].v3Norm = vNormal;
#ifdef _DEBUG
                // check that the normal is actually perpendicular
                float dot = XMVectorGetX( XMVector3Dot( XMLoadFloat3( &g_ConeForSpotLightsVertexData[2*i+1].v3Pos ), XMLoadFloat3( &vNormal ) ) );
                assert( abs(dot) < 0.001f );
#endif
            }

            // create duplicate points for the top of the cone, each with its own normal
            for (int i = 0; i < g_nConeNumTris; i++)
            {
                g_ConeForSpotLightsVertexData[2*i].v3Pos = XMFLOAT3( 0.0f, 0.0f, 0.0f );
                g_ConeForSpotLightsVertexData[2*i].v2TexCoord = XMFLOAT2( 0.0f, 0.0f );

                XMFLOAT3 vNormal;
                XMVECTOR Normal = XMLoadFloat3(&g_ConeForSpotLightsVertexData[2*i+1].v3Norm) + XMLoadFloat3(&g_ConeForSpotLightsVertexData[2*i+3].v3Norm);
                XMStoreFloat3( &vNormal, XMVector3Normalize( Normal ) );
                g_ConeForSpotLightsVertexData[2*i].v3Norm = vNormal;
            }

            // fill in the index buffer for the cone
            for (int i = 0; i < g_nConeNumTris; i++)
            {
                g_ConeForSpotLightsIndexData[3*i+0] = (unsigned short)(2*i);
                g_ConeForSpotLightsIndexData[3*i+1] = (unsigned short)(2*i+3);
                g_ConeForSpotLightsIndexData[3*i+2] = (unsigned short)(2*i+1);
            }

            // fix up the last triangle
            g_ConeForSpotLightsIndexData[3*g_nConeNumTris-2] = 1;
        }
    }

    //--------------------------------------------------------------------------------------
    // Add shaders to the shader cache
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::AddShadersToCache( AMD::ShaderCache *pShaderCache )
    {
        // Ensure all shaders (and input layouts) are released
        SAFE_RELEASE( m_pDebugDrawPointLightsVS );
        SAFE_RELEASE( m_pDebugDrawPointLightsPS );
        SAFE_RELEASE( m_pDebugDrawPointLightsLayout11 );
        SAFE_RELEASE( m_pDebugDrawSpotLightsVS );
        SAFE_RELEASE( m_pDebugDrawSpotLightsPS );
        SAFE_RELEASE( m_pDebugDrawSpotLightsLayout11 );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileVS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileGrayscalePS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS );
        SAFE_RELEASE( m_pDebugDrawLegendForNumLightsLayout11 );

        const D3D11_INPUT_ELEMENT_DESC LayoutForSprites[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        const D3D11_INPUT_ELEMENT_DESC LayoutForCone[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawPointLightsVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"DebugDrawPointLightsVS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, &m_pDebugDrawPointLightsLayout11, (D3D11_INPUT_ELEMENT_DESC*)LayoutForSprites, ARRAYSIZE( LayoutForSprites ) );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawPointLightsPS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"DebugDrawPointLightsPS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, NULL, NULL, 0 );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawSpotLightsVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"DebugDrawSpotLightsVS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, &m_pDebugDrawSpotLightsLayout11, (D3D11_INPUT_ELEMENT_DESC*)LayoutForCone, ARRAYSIZE( LayoutForCone ) );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawSpotLightsPS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"DebugDrawSpotLightsPS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, NULL, NULL, 0 );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawLegendForNumLightsPerTileVS, AMD::ShaderCache::SHADER_TYPE_VERTEX, L"vs_5_0", L"DebugDrawLegendForNumLightsPerTileVS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, &m_pDebugDrawLegendForNumLightsLayout11, (D3D11_INPUT_ELEMENT_DESC*)LayoutForSprites, ARRAYSIZE( LayoutForSprites ) );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawLegendForNumLightsPerTileGrayscalePS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"DebugDrawLegendForNumLightsPerTileGrayscalePS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, NULL, NULL, 0 );

        pShaderCache->AddShader( (ID3D11DeviceChild**)&m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS, AMD::ShaderCache::SHADER_TYPE_PIXEL, L"ps_5_0", L"DebugDrawLegendForNumLightsPerTileRadarColorsPS",
            L"ForwardPlus11DebugDraw.hlsl", 0, NULL, NULL, NULL, 0 );
    }

    //--------------------------------------------------------------------------------------
    // Draw the legend for the lights-per-tile visualization
    //--------------------------------------------------------------------------------------
    void ForwardPlusUtil::RenderLegend( CDXUTTextHelper *pTxtHelper, int nLineHeight, XMFLOAT4 Color, bool bGrayscaleMode )
    {
        // draw the legend texture for the lights-per-tile visualization
        {
            ID3D11ShaderResourceView* pNULLSRV = NULL;
            ID3D11SamplerState* pNULLSampler = NULL;

            // choose pixel shader based on radar vs. grayscale
            ID3D11PixelShader* pPixelShader = bGrayscaleMode ? m_pDebugDrawLegendForNumLightsPerTileGrayscalePS : m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS;

            ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();

            // save depth state (for later restore)
            ID3D11DepthStencilState* pDepthStencilStateStored11 = NULL;
            UINT uStencilRefStored11;
            pd3dImmediateContext->OMGetDepthStencilState( &pDepthStencilStateStored11, &uStencilRefStored11 );

            // disable depth test
            pd3dImmediateContext->OMSetDepthStencilState( m_pDisableDepthTest, 0x00 );

            // Set the input layout
            pd3dImmediateContext->IASetInputLayout( m_pDebugDrawLegendForNumLightsLayout11 );

            // Set vertex buffer
            UINT uStride = sizeof( SpriteVertex );
            UINT uOffset = 0;
            pd3dImmediateContext->IASetVertexBuffers( 0, 1, &m_pQuadForLegendVB, &uStride, &uOffset );

            // Set primitive topology
            pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

            pd3dImmediateContext->VSSetShader( m_pDebugDrawLegendForNumLightsPerTileVS, NULL, 0 );
            pd3dImmediateContext->PSSetShader( pPixelShader, NULL, 0 );
            pd3dImmediateContext->PSSetShaderResources( 0, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetShaderResources( 1, 1, &pNULLSRV );
            pd3dImmediateContext->PSSetSamplers( 0, 1, &pNULLSampler );

            pd3dImmediateContext->Draw(6,0);

            // restore to previous
            pd3dImmediateContext->OMSetDepthStencilState( pDepthStencilStateStored11, uStencilRefStored11 );
            SAFE_RELEASE( pDepthStencilStateStored11 );
        }

        // draw the legend text for the lights-per-tile visualization
        // (twice, to get a drop shadow)
        XMFLOAT4 Colors[2] = { XMFLOAT4(0,0,0,Color.w), Color };
        for( int loopCount = 0; loopCount < 2; loopCount++ )
        {
            // 17 lines times line height
            int nTextureHeight = g_nLegendNumLines*nLineHeight;

            int nMaxNumLightsPerTile = (int)GetMaxNumLightsPerTile();
            WCHAR szBuf[16];

            pTxtHelper->Begin();
            pTxtHelper->SetForegroundColor( Colors[loopCount] );
            pTxtHelper->SetInsertionPos( g_nLegendPaddingLeft+g_nLegendTextureWidth+4-loopCount, (int)m_uHeight-g_nLegendPaddingBottom-nTextureHeight-nLineHeight-3-loopCount );
            pTxtHelper->DrawTextLine( L"Num Lights" );

            int nStartVal, nEndVal;

            if( bGrayscaleMode )
            {
                float fLightsPerBand = (float)(nMaxNumLightsPerTile) / (float)g_nLegendNumLines;

                for( int i = 0; i < g_nLegendNumLines; i++ )
                {
                    nStartVal = (int)((g_nLegendNumLines-1-i)*fLightsPerBand) + 1;
                    nEndVal = (int)((g_nLegendNumLines-i)*fLightsPerBand);
                    swprintf_s( szBuf, 16, L"[%d,%d]", nStartVal, nEndVal );
                    pTxtHelper->DrawTextLine( szBuf );
                }
            }
            else
            {
                // use a log scale to provide more detail when the number of lights is smaller

                // want to find the base b such that the logb of nMaxNumLightsPerTile is 14
                // (because we have 14 radar colors)
                float fLogBase = exp(0.07142857f*log((float)nMaxNumLightsPerTile));

                swprintf_s( szBuf, 16, L"> %d", nMaxNumLightsPerTile );
                pTxtHelper->DrawTextLine( szBuf );

                swprintf_s( szBuf, 16, L"%d", nMaxNumLightsPerTile );
                pTxtHelper->DrawTextLine( szBuf );

                nStartVal = (int)pow(fLogBase,g_nLegendNumLines-4) + 1;
                nEndVal = nMaxNumLightsPerTile-1;
                swprintf_s( szBuf, 16, L"[%d,%d]", nStartVal, nEndVal );
                pTxtHelper->DrawTextLine( szBuf );

                for( int i = 0; i < g_nLegendNumLines-5; i++ )
                {
                    nStartVal = (int)pow(fLogBase,g_nLegendNumLines-5-i) + 1;
                    nEndVal = (int)pow(fLogBase,g_nLegendNumLines-4-i);
                    if( nStartVal == nEndVal )
                    {
                        swprintf_s( szBuf, 16, L"%d", nStartVal );
                    }
                    else
                    {
                        swprintf_s( szBuf, 16, L"[%d,%d]", nStartVal, nEndVal );
                    }
                    pTxtHelper->DrawTextLine( szBuf );
                }

                pTxtHelper->DrawTextLine( L"1" );
                pTxtHelper->DrawTextLine( L"0" );
            }

            pTxtHelper->End();
        }
    }

    //--------------------------------------------------------------------------------------
    // Calculate the number of tiles in the horizontal direction
    //--------------------------------------------------------------------------------------
    unsigned ForwardPlusUtil::GetNumTilesX()
    {
        return (unsigned)( ( m_uWidth + TILE_RES - 1 ) / (float)TILE_RES );
    }

    //--------------------------------------------------------------------------------------
    // Calculate the number of tiles in the vertical direction
    //--------------------------------------------------------------------------------------
    unsigned ForwardPlusUtil::GetNumTilesY()
    {
        return (unsigned)( ( m_uHeight + TILE_RES - 1 ) / (float)TILE_RES );
    }

    //--------------------------------------------------------------------------------------
    // Adjust max number of lights per tile based on screen height.
    // This assumes that the demo has a constant vertical field of view (fovy).
    //
    // Note that the light culling tile size stays fixed as screen size changes.
    // With a constant fovy, reducing the screen height shrinks the projected 
    // view of the scene, and so more lights can fall into our fixed tile size.
    //
    // This function reduces the max lights per tile as screen height increases, 
    // to save memory. It was tuned for this particular demo and is not intended 
    // as a general solution for all scenes.
    //--------------------------------------------------------------------------------------
    unsigned ForwardPlusUtil::GetMaxNumLightsPerTile()
    {
        const unsigned kAdjustmentMultipier = 32;

        // I haven't tested at greater than 1080p, so cap it
        unsigned uHeight = (m_uHeight > 1080) ? 1080 : m_uHeight;

        // adjust max lights per tile down as height increases
        return ( MAX_NUM_LIGHTS_PER_TILE - ( kAdjustmentMultipier * ( uHeight / 120 ) ) );
    }

} // namespace ForwardPlus11

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------
