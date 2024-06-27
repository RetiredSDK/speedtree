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
#include <cassert>
#include <memory.h>
#include <cmath>
#include "SpeedTreeWrapper.h"
#ifdef _XBOX
#include <xboxmath.h>
#endif


///////////////////////////////////////////////////////////////////////  
//  class CSpeedTreeInstance declaration

class CSpeedTreeInstance
{
public:
                                    CSpeedTreeInstance( );
                                    ~CSpeedTreeInstance( );

        // modifiers
        void                        SetParent(CSpeedTreeWrapper* pParent)           { st_assert(pParent); m_pInstanceOf = pParent; }
        void                        SetPos(const float* pPos);
        void                        SetRotation(float fRotation);
        void                        SetScale(float fScale)                          { m_fScale = fScale; }
        void                        SetCulled(bool bFlag)                           { m_bCulled = bFlag; }
        void                        SetWindMatrixOffset(float fOffset)              { m_fWindMatrixOffset = float(int(10.0f * fOffset)); }
        void                        Set3dFade(float fAlphaFade)                     { m_fBranchAlphaValue = m_fFrondAlphaValue = m_afLeafAlphaValues[0] = fAlphaFade; }

        // inspectors
        CSpeedTreeWrapper*          InstanceOf(void) const                          { return m_pInstanceOf; }
        const float*                Pos(void) const                                 { return m_afPos; }
        float                       UpAxisCenter(void) const                        { return m_fUpAxisCenter; }
        float                       Rotation(void) const                            { return m_fRotation; }
        const float*                RotationTrigValues(void) const                  { return m_afRotationTrigValues; }
        float                       Scale(void) const                               { return m_fScale; }
        bool                        IsCulled(void) const                            { return m_bCulled; }
        float                       WindMatrixOffset(void) const                    { return m_fWindMatrixOffset; }

        // lod
        float                       GetLod(void) const                              { return m_fLod; }
        bool                        BillboardIsActive(void) const                   { return m_bBillboardIsActive; }
        bool                        OnlyBillboardIsActive(void) const               { return m_fLod == 0.0f; }
        void                        UpdateLod(float fLod);
        void                        GetBranchLod(int& nLevel, float& fAlpha) const  { nLevel = m_nBranchLodLevel; fAlpha = m_fBranchAlphaValue; }
        void                        GetFrondLod(int& nLevel, float& fAlpha) const   { nLevel = m_nFrondLodLevel; fAlpha = m_fFrondAlphaValue; }
        void                        GetLeafLod(int anLevels[2], float afAlphas[2]) const { anLevels[0] = m_anLeafLodLevels[0]; anLevels[1] = m_anLeafLodLevels[1]; afAlphas[0] = m_afLeafAlphaValues[0]; afAlphas[1] = m_afLeafAlphaValues[1]; }
        float                       GetBillboardFade(void) const                    { return m_fBillboardFadeOut; }

private:
        // general
        CSpeedTreeWrapper*          m_pInstanceOf;              // base tree (SPT) of which this is an instance
        float                       m_afPos[3];                 // 3D position of tree in space
        float                       m_fUpAxisCenter;            // center of tree bounding sphere is needed for culling (don't need to store 
                                                                // all three points since there is overlap with m_afPos)
        float                       m_fRotation;                // rotation of instance along up-axis in radians (0.0 is unrotated)
        float                       m_afRotationTrigValues[4];  // sin(m_fRotation), cos(m_fRotation), -sin(m_fRotation), 0.0f
        float                       m_fScale;                   // relative size of instance (1.0 is unscaled)
        bool                        m_bCulled;                  // true if this instance is not in the frustum
        float                       m_fWindMatrixOffset;        // used to assigned instances of the same tree different wind matrices

        // lod
        float                       m_fLod;                     // 1.0 = highest (fully 3D), 0.0 = lowest (billboard)
        char                        m_nBranchLodLevel;          // 0 is highest LOD, ranges from [0,CSpeedTreeRT::GetNumBranchLodLevels()-1]
        float                       m_fBranchAlphaValue;        // range [84.0 (opaque), 255.0 (invisible)]

        char                        m_nFrondLodLevel;           // 0 is highest LOD, ranges from [0,CSpeedTreeRT::GetNumFrondLodLevels()-1]
        float                       m_fFrondAlphaValue;         // range [84.0 (opaque), 255.0 (invisible)]

        char                        m_anLeafLodLevels[2];       // 0 is highest LOD, ranges from [0,CSpeedTreeRT::GetNumLeafLodLevels()-1]
        float                       m_afLeafAlphaValues[2];     // range [84.0 (opaque), 255.0 (invisible)]

        bool                        m_bBillboardIsActive;       // true if billboard is fading or all the way in
        float                       m_fBillboardFadeOut;        // range [0.0 (not visible), 1.0 (fully visible)]
};


///////////////////////////////////////////////////////////////////////  
//  SpeedTree memory management
//
//  Used to capture allocations in STL containers to report back to
//  SpeedTreeRT's allocation mechanism

DefineAllocator(st_allocator_stinstance);
typedef std::vector<CSpeedTreeInstance, st_allocator_stinstance<CSpeedTreeInstance> > st_vector_stinstance;
DefineAllocator(st_allocator_stinstance_p);
typedef std::vector<CSpeedTreeInstance*, st_allocator_stinstance_p<CSpeedTreeInstance*> > st_vector_stinstance_p;


