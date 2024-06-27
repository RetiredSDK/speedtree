///////////////////////////////////////////////////////////////////////  
//  Utility Shaders
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com
//
//  *** Release Version 4.1 ***


///////////////////////////////////////////////////////////////////////  
//  Modulate_Float
//
//  Returns x % y (some compilers generate way too many instructions when
//  using the native '%' operator)

float Modulate_Float(float x, float y)
{
    return x - (int(x / y) * y);
}


///////////////////////////////////////////////////////////////////////  
//  WindEffect
//
//  New with 4.0 is a two-weight wind system that allows the tree model
//  to bend at more than one branch level.
//
//  In order to keep the vertex size small, the wind parameters have been
//  compressed as detailed here:
//
//      vWindInfo.x = (wind_matrix_index1 * 10.0) / NUM_WIND_MATRICES  + wind_weight1
//      vWindInfo.y = (wind_matrix_index2 * 10.0) / NUM_WIND_MATRICES  + wind_weight2
//
//  * Note: NUM_WIND_MATRICES cannot be larger than 10 in this case
//
//  * Caution: Negative wind weights will not work with this scheme.  We rely on the
//             fact that the SpeedTreeRT library clamps wind weights to [0.0, 1.0]

#define TWO_WEIGHT_WIND

float3 WindEffect(float3 vPosition, float2 vWindInfo)
{
    // decode both wind weights and matrix indices at the same time in order to save
    // vertex instructions
    vWindInfo.xy += g_fWindMatrixOffset.xx;
    float2 vWeights = frac(vWindInfo.xy);
    float2 vIndices = (vWindInfo - vWeights) * 0.05f * NUM_WIND_MATRICES;
    
    // first-level wind effect - interpolate between static position and fully-blown
    // wind position by the wind weight value
#ifdef IDV_OPENGL
    float3 vWindEffect = lerp(vPosition.xyz, mul(vPosition, float3x3(g_amWindMatrices[int(vIndices.x)])), vWeights.x);
#else
    float3 vWindEffect = lerp(vPosition.xyz, mul(vPosition, g_amWindMatrices[int(vIndices.x)]), vWeights.x);
#endif
    
    // second-level wind effect - interpolate between first-level wind position and 
    // the fully-blown wind position by the second wind weight value
#ifdef TWO_WEIGHT_WIND
    #ifdef IDV_OPENGL
        return lerp(vWindEffect, mul(vWindEffect, float3x3(g_amWindMatrices[int(vIndices.y)])), vWeights.y);
    #else
        return lerp(vWindEffect, mul(vWindEffect, g_amWindMatrices[int(vIndices.y)]), vWeights.y);
    #endif
#else
    return vWindEffect;
#endif
}



///////////////////////////////////////////////////////////////////////  
//  LightDiffuse
//
//  Very simple lighting equation, used by the fronds and leaves (branches
//  and billboards are normal mapped).

float3 LightDiffuse(float3 vVertex,
                    float3 vNormal,
                    float3 vLightDir,
                    float3 vLightColor,
                    float3 vDiffuseMaterial)
{
    return vDiffuseMaterial * vLightColor * max(dot(vNormal, vLightDir), 0.0f);
}


///////////////////////////////////////////////////////////////////////  
//  LightDiffuse_Capped
//
//  Slightly modified version of LightDiffuse, used by the leaf shader in
//  order to cap the dot contribution

float3 LightDiffuse_Capped(float3 vVertex,
                           float3 vNormal,
                           float3 vLightDir,
                           float3 vLightColor,
                           float3 vDiffuseMaterial)
{
    float fDotProduct = max(dot(vNormal, vLightDir), 0.0f);
    fDotProduct = lerp(g_fLeafLightingAdjust, 1.0f, fDotProduct);
    
    return vDiffuseMaterial * vLightColor * fDotProduct;
}


///////////////////////////////////////////////////////////////////////  
//  FogValue
//
//  Simple LINEAR fog computation.  If an exponential equation is desired,
//  it can be placed here - all of the shaders call this one function.

#ifdef USE_FOG
float FogValue(float fPoint)
{
    float fFogEnd = g_vFogParams.y;
    float fFogDist = g_vFogParams.z;
    
    return saturate((fFogEnd - fPoint) / fFogDist);
}
#endif


///////////////////////////////////////////////////////////////////////  
//  RotationMatrix_zAxis
//
//  Constructs a Z-axis rotation matrix

float3x3 RotationMatrix_zAxis(float fAngle)
{
    // compute sin/cos of fAngle
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float3x3(vSinCos.y, -vSinCos.x, 0.0f, 
                    vSinCos.x, vSinCos.y, 0.0f, 
                    0.0f, 0.0f, 1.0f);
}


///////////////////////////////////////////////////////////////////////  
//  Rotate_zAxis
//
//  Returns an updated .xy value

float2 Rotate_zAxis(float fAngle, float3 vPoint)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float2(dot(vSinCos.yx, vPoint.xy), dot(float2(-vSinCos.x, vSinCos.y), vPoint.xy));
}


///////////////////////////////////////////////////////////////////////  
//  RotationMatrix_yAxis
//
//  Constructs a Y-axis rotation matrix

float3x3 RotationMatrix_yAxis(float fAngle)
{
    // compute sin/cos of fAngle
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float3x3(vSinCos.y, 0.0f, vSinCos.x,
                    0.0f, 1.0f, 0.0f,
                    -vSinCos.x, 0.0f, vSinCos.y);
}


///////////////////////////////////////////////////////////////////////  
//  Rotate_yAxis
//
//  Returns an updated .xz value

float2 Rotate_yAxis(float fAngle, float3 vPoint)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float2(dot(float2(vSinCos.y, -vSinCos.x), vPoint.xz), dot(vSinCos.xy, vPoint.xz));
}


///////////////////////////////////////////////////////////////////////  
//  RotationMatrix_xAxis
//
//  Constructs a X-axis rotation matrix

float3x3 RotationMatrix_xAxis(float fAngle)
{
    // compute sin/cos of fAngle
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float3x3(1.0f, 0.0f, 0.0f,
                    0.0f, vSinCos.y, -vSinCos.x,
                    0.0f, vSinCos.x, vSinCos.y);
}


///////////////////////////////////////////////////////////////////////  
//  Rotate_xAxis
//
//  Returns an updated .yz value

float2 Rotate_xAxis(float fAngle, float3 vPoint)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    
    return float2(dot(vSinCos.yx, vPoint.yz), dot(float2(-vSinCos.x, vSinCos.y), vPoint.yz));
}


#ifdef INTERMEDIATE_FRONDBRANCH_FADING
///////////////////////////////////////////////////////////////////////  
//  ComputeFadeValue

float ComputeFadeValue(float fHint)
{
    float fCurrentLod = g_vBranchLodValues.x;
    float fLodRadius = g_vBranchLodValues.y;
    
    float fValue = (fCurrentLod - fHint) / fLodRadius;
    
    // we don't scale the fade from 0.0 to 1.0 because the range typically used by
    // SpeedTree for its alpha fizzle/fade effect is c_fOpaqueAlpha (84) to 
    // c_fClearAlpha (255), making the low end 84 / 255 instead of 0.
    //
    // 0.33f = c_fOpaqueAlpha / 255.0f
    return lerp(0.33f, 1.0f, clamp(fValue, 0.0f, 1.0f));
}
#endif
