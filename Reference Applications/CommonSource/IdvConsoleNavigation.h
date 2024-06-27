///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Include Files

#pragma once
#include <memory.h>
#include "IdvVectorMath.h"
#include <string.h>


///////////////////////////////////////////////////////////////////////  
// class CNavigation

class CNavigation  
{
public: 
                        CNavigation( );

        const float*    GetCameraPos(void) const        { return m_afCameraPos; }
        const float*    GetCameraTransform(void);
        void            SetCameraPos(const float* pPos) { memcpy(m_afCameraPos, pPos, 3 * sizeof(float)); }
        
        void            MoveFB(float fDist);
        void            MoveLR(float fDist);
        void            TurnLR(float fDist);
        void            TurnUD(float fDist);

private:
        float           m_afCameraPos[3];
        float           m_fAzimuth;
        float           m_fPitch;
        idv::Transform  m_cTransform;
};
