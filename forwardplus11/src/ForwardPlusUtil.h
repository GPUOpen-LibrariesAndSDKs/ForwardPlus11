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
// File: ForwardPlusUtil.h
//
// Helper functions for the ForwardPlus11 sample.
//--------------------------------------------------------------------------------------

#pragma once

#include "..\\..\\DXUT\\Core\\DXUT.h"

// Forward declarations
namespace AMD
{
    class ShaderCache;
}
class CDXUTSDKMesh;
class CDXUTTextHelper;

namespace ForwardPlus11
{
    static const int MAX_NUM_LIGHTS = 2*1024;

    class ForwardPlusUtil
    {
    public:
        // Constructor / destructor
        ForwardPlusUtil();
        ~ForwardPlusUtil();

        static void CalculateSceneMinMax( CDXUTSDKMesh &Mesh, DirectX::XMVECTOR *pBBoxMinOut, DirectX::XMVECTOR *pBBoxMaxOut );
        static void InitLights( const DirectX::XMVECTOR &BBoxMin, const DirectX::XMVECTOR &BBoxMax );

        void AddShadersToCache( AMD::ShaderCache *pShaderCache );

        void RenderLegend( CDXUTTextHelper *pTxtHelper, int nLineHeight, DirectX::XMFLOAT4 Color, bool bGrayscaleMode );

        // Various hook functions
        HRESULT OnCreateDevice( ID3D11Device* pd3dDevice );
        void OnDestroyDevice();
        HRESULT OnResizedSwapChain( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, int nLineHeight );
        void OnReleasingSwapChain();
        void OnRender( float fElapsedTime, unsigned uNumPointLights, unsigned uNumSpotLights );

        unsigned GetNumTilesX();
        unsigned GetNumTilesY();
        unsigned GetMaxNumLightsPerTile();

        ID3D11ShaderResourceView * const * GetPointLightBufferCenterAndRadiusSRVParam() { return &m_pPointLightBufferCenterAndRadiusSRV; }
        ID3D11ShaderResourceView * const * GetPointLightBufferColorSRVParam()  { return &m_pPointLightBufferColorSRV; }
        ID3D11ShaderResourceView * const * GetSpotLightBufferCenterAndRadiusSRVParam() { return &m_pSpotLightBufferCenterAndRadiusSRV; }
        ID3D11ShaderResourceView * const * GetSpotLightBufferColorSRVParam()  { return &m_pSpotLightBufferColorSRV; }
        ID3D11ShaderResourceView * const * GetSpotLightBufferSpotParamsSRVParam()  { return &m_pSpotLightBufferSpotParamsSRV; }
        ID3D11ShaderResourceView * const * GetSpotLightBufferSpotMatricesSRVParam()  { return &m_pSpotLightBufferSpotMatricesSRV; }

        ID3D11ShaderResourceView * const * GetLightIndexBufferSRVParam() { return &m_pLightIndexBufferSRV; }
        ID3D11UnorderedAccessView * const * GetLightIndexBufferUAVParam() { return &m_pLightIndexBufferUAV; }

    private:

        // Light culling constants.
        // These must match their counterparts in ForwardPlus11Common.hlsl
        static const unsigned TILE_RES = 16;
        static const unsigned MAX_NUM_LIGHTS_PER_TILE = 544;

        // forward rendering render target width and height
        unsigned                    m_uWidth;
        unsigned                    m_uHeight;

        // point lights
        ID3D11Buffer*               m_pPointLightBufferCenterAndRadius;
        ID3D11ShaderResourceView*   m_pPointLightBufferCenterAndRadiusSRV;
        ID3D11Buffer*               m_pPointLightBufferColor;
        ID3D11ShaderResourceView*   m_pPointLightBufferColorSRV;

        // spot lights
        ID3D11Buffer*               m_pSpotLightBufferCenterAndRadius;
        ID3D11ShaderResourceView*   m_pSpotLightBufferCenterAndRadiusSRV;
        ID3D11Buffer*               m_pSpotLightBufferColor;
        ID3D11ShaderResourceView*   m_pSpotLightBufferColorSRV;
        ID3D11Buffer*               m_pSpotLightBufferSpotParams;
        ID3D11ShaderResourceView*   m_pSpotLightBufferSpotParamsSRV;

        // these are only used for debug drawing the spot lights
        ID3D11Buffer*               m_pSpotLightBufferSpotMatrices;
        ID3D11ShaderResourceView*   m_pSpotLightBufferSpotMatricesSRV;

        // buffers for light culling
        ID3D11Buffer*               m_pLightIndexBuffer;
        ID3D11ShaderResourceView*   m_pLightIndexBufferSRV;
        ID3D11UnorderedAccessView*  m_pLightIndexBufferUAV;

        // sprite quad VB (for debug drawing the lights)
        ID3D11Buffer*               m_pQuadForLightsVB;

        // sprite quad VB (for debug drawing the lights-per-tile legend texture)
        ID3D11Buffer*               m_pQuadForLegendVB;

        // cone VB and IB (for debug drawing the spot lights)
        ID3D11Buffer*               m_pConeForSpotLightsVB;
        ID3D11Buffer*               m_pConeForSpotLightsIB;

        // debug draw shaders for the point lights
        ID3D11VertexShader*         m_pDebugDrawPointLightsVS;
        ID3D11PixelShader*          m_pDebugDrawPointLightsPS;
        ID3D11InputLayout*          m_pDebugDrawPointLightsLayout11;

        // debug draw shaders for the spot lights
        ID3D11VertexShader*         m_pDebugDrawSpotLightsVS;
        ID3D11PixelShader*          m_pDebugDrawSpotLightsPS;
        ID3D11InputLayout*          m_pDebugDrawSpotLightsLayout11;

        // debug draw shaders for the lights-per-tile legend
        ID3D11VertexShader*         m_pDebugDrawLegendForNumLightsPerTileVS;
        ID3D11PixelShader*          m_pDebugDrawLegendForNumLightsPerTileGrayscalePS;
        ID3D11PixelShader*          m_pDebugDrawLegendForNumLightsPerTileRadarColorsPS;
        ID3D11InputLayout*          m_pDebugDrawLegendForNumLightsLayout11;

        // state
        ID3D11BlendState*           m_pAdditiveBlendingState;
        ID3D11DepthStencilState*    m_pDisableDepthWrite;
        ID3D11DepthStencilState*    m_pDisableDepthTest;
        ID3D11RasterizerState*      m_pDisableCullingRS;
    };

} // namespace ForwardPlus11

//--------------------------------------------------------------------------------------
// EOF
//--------------------------------------------------------------------------------------
