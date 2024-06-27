///////////////////////////////////////////////////////////////////////  
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

#pragma once
#include "SpeedTreeMemory.h"


///////////////////////////////////////////////////////////////////////  
//  struct SSpeedTreeBillboard declaration

struct SSpeedTreeBillboard
{
#pragma pack(push, 1)
        // SVertex is used for both vertical (standard) and horizontal billboards
        struct SVertex
        {
            float           m_afPos[4];             // vert: xyz = tree pos, w = cluster corner
                                                    // horz: xyz = tree pos, w = ground offset
            float           m_afTexCoord0[4];       // vert: x = width, y = height, z = azimuth, w = fade value 
                                                    // horz: x = corner-x, y = corner-y, z = azimuth, w = fade value 
            float           m_afMiscParams[4];      // vert: x = tree scalar, y = texcoord offset, z = num images, z = transition %
                                                    // horz: x = tree scalar, y = s-texcoord, z = t-texcoord
            float           m_afLightAdjusts[3];    // vert: x = bright side adjustment, y = dark side adjustment, z = ambient scalar
                                                    // horz: x = bright side adjustment, y = dark side adjustment, z = ambient scalar
        };

static  int NumVerticesPerBillboard(void)
        {
#if IDV_OPENGL | _XBOX
            return 4;
#elif IDV_DIRECTX9 
            return 6;
#else
#error IDV_OPENGL, IDV_DIRECTX9, or _XBOX must be #defined
#endif
        }

static  int Size(void)
        {
            return NumVerticesPerBillboard( ) * sizeof(SVertex);
        }

        inline void UpdateVertical(const float* pPos, float fAzimuth, float fFade, float fScale)
        {
            // update the four positions
            float* pVertPos = m_sVert0.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            pVertPos = m_sVert1.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            pVertPos = m_sVert2.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            pVertPos = m_sVert3.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            // update the four azimuths
            m_sVert0.m_afTexCoord0[2] = fAzimuth;
            m_sVert1.m_afTexCoord0[2] = fAzimuth;
            m_sVert2.m_afTexCoord0[2] = fAzimuth;
            m_sVert3.m_afTexCoord0[2] = fAzimuth;

            // update the four fades
            m_sVert0.m_afTexCoord0[3] = fFade;
            m_sVert1.m_afTexCoord0[3] = fFade;
            m_sVert2.m_afTexCoord0[3] = fFade;
            m_sVert3.m_afTexCoord0[3] = fFade;

            // update the four scales
            m_sVert0.m_afMiscParams[0] = fScale;
            m_sVert1.m_afMiscParams[0] = fScale;
            m_sVert2.m_afMiscParams[0] = fScale;
            m_sVert3.m_afMiscParams[0] = fScale;

#if IDV_DIRECTX9 && !_XBOX
            pVertPos = m_sVert4.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            pVertPos = m_sVert5.m_afPos;
            *pVertPos++ = pPos[0];
            *pVertPos++ = pPos[1];
            *pVertPos++ = pPos[2];

            m_sVert4.m_afTexCoord0[2] = fAzimuth;
            m_sVert5.m_afTexCoord0[2] = fAzimuth;

            m_sVert4.m_afTexCoord0[3] = fFade;
            m_sVert5.m_afTexCoord0[3] = fFade;

            m_sVert4.m_afMiscParams[0] = fScale;
            m_sVert5.m_afMiscParams[0] = fScale;
#endif
        }

#ifdef HORZ_BILLBOARDS
        inline void UpdateHorizontal(const CSpeedTreeRT::SGeometry::SHorzBillboard& sHorzBB, const float* pPos, float fAzimuth, float fFade, float fScale)
        {
            const float* pOffsets = sHorzBB.m_pCoords;

            if (pOffsets)
            {
                // update the four positions
                float* pVertPos = m_sVert0.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                pVertPos = m_sVert1.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                pVertPos = m_sVert2.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                pVertPos = m_sVert3.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                // update the four azimuths
                m_sVert0.m_afTexCoord0[2] = fAzimuth;
                m_sVert1.m_afTexCoord0[2] = fAzimuth;
                m_sVert2.m_afTexCoord0[2] = fAzimuth;
                m_sVert3.m_afTexCoord0[2] = fAzimuth;

                // update the four fades
                m_sVert0.m_afTexCoord0[3] = fFade;
                m_sVert1.m_afTexCoord0[3] = fFade;
                m_sVert2.m_afTexCoord0[3] = fFade;
                m_sVert3.m_afTexCoord0[3] = fFade;

                // update the four scales
                m_sVert0.m_afMiscParams[0] = fScale;
                m_sVert1.m_afMiscParams[0] = fScale;
                m_sVert2.m_afMiscParams[0] = fScale;
                m_sVert3.m_afMiscParams[0] = fScale;

#if IDV_DIRECTX9 & !_XBOX
                pVertPos = m_sVert4.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                pVertPos = m_sVert5.m_afPos;
                *pVertPos++ = pPos[0];
                *pVertPos++ = pPos[1];
                *pVertPos++ = pPos[2];

                m_sVert4.m_afTexCoord0[2] = fAzimuth;
                m_sVert5.m_afTexCoord0[2] = fAzimuth;

                m_sVert4.m_afTexCoord0[3] = fFade;
                m_sVert5.m_afTexCoord0[3] = fFade;

                m_sVert4.m_afMiscParams[0] = fScale;
                m_sVert5.m_afMiscParams[0] = fScale;
#endif
            }
            else
            {
                // no horizontal billboard present, so fade it all the way out
                m_sVert0.m_afTexCoord0[3] = 0.0f;
                m_sVert1.m_afTexCoord0[3] = 0.0f;
                m_sVert2.m_afTexCoord0[3] = 0.0f;
                m_sVert3.m_afTexCoord0[3] = 0.0f;
            }
        }
#endif

        SVertex         m_sVert0;
        SVertex         m_sVert1;
        SVertex         m_sVert2;
        SVertex         m_sVert3;
#if IDV_DIRECTX9 && !_XBOX
        SVertex         m_sVert4;
        SVertex         m_sVert5;
#endif
};

#pragma pack (pop)


///////////////////////////////////////////////////////////////////////  
//  SpeedTree memory management
//
//  Used to capture allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

DefineAllocator(st_allocator_stbillboard);
typedef std::vector<SSpeedTreeBillboard, st_allocator_stbillboard<SSpeedTreeBillboard> > st_vector_stbillboard;

