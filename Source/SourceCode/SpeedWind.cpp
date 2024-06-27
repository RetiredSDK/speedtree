///////////////////////////////////////////////////////////////////////  
//  SpeedWind.cpp
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


/////////////////////////////////////////////////////////////////////////////
// Preprocessor

#include "SpeedTreeRT.h"
#include "Debug.h"
#include "LibVector_Source/IdvVector.h"
#include "UpVector.h"
#include <fstream>
#include <sstream>
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// constants

const int c_nRandomBufferSize = 1024;


/////////////////////////////////////////////////////////////////////////////
// SpeedTree memory management

DefineAllocator(st_allocator_stMatrix);
typedef std::vector<CSpeedWindMatrix, st_allocator_stMatrix<CSpeedWindMatrix> > st_vector_stMatrix;


/////////////////////////////////////////////////////////////////////////////
// class CSpeedWindDef

class CSpeedWindDef
{
public:
                                        CSpeedWindDef(CSpeedWind* pFather);
                                        ~CSpeedWindDef( );

        // parameter setting
        void                            Reset(void);
        void                            SetQuantities(int nNumWindMatrices, int nNumLeafAngles);
        void                            SetWindResponse(float fResponse, float fReponseLimit);
        void                            SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ);
        void                            SetMaxBendAngle(float fMaxBendAngle);
        void                            SetExponents(float fBranchExponent, float fLeafExponent);
        void                            SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax);
        void                            SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);
        void                            SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed);

        // parameter getting
        void                            GetQuantities(int& iNumWindMatrices, int& iNumLeafAngles) const;
        void                            GetWindResponse(float& fResponse, float& fReponseLimit) const;
        void                            GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const;
        float                           GetMaxBendAngle(void) const;
        void                            GetExponents(float& fBranchExponent, float& fLeafExponent) const;
        void                            GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const;
        void                            GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;
        void                            GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const;

        // render interface
        void                            Advance(float fCurrentTime, bool bUpdateMatrices = true, bool bUpdateLeafAngleMatrices = false, float fCameraX = 0.0f, float fCameraY = 0.0f, float fCameraZ = 1.0f);
        void                            UpdateSpeedTreeRT(void);
        void                            UpdateTree(CSpeedTreeRT* pTree);
        float                           GetFinalStrength(void);
        unsigned int                    GetNumWindMatrices(void) const;
        float*                          GetWindMatrix(unsigned int uiIndex) const;
        unsigned int                    GetNumLeafAngleMatrices(void) const;
        float*                          GetLeafAngleMatrix(unsigned int uiIndex) const;
        bool                            GetRustleAngles(float* pAngles) const;
        bool                            GetRockAngles(float* pAngles) const;

        // file I/O
        bool                            Load(const st_string& strFilename);
        bool                            Load(char* pData, unsigned int uiNumBytes);
        bool                            Save(const st_string& strFilename) const;
        char*                           Save(unsigned int& uiNumBytes) const;

        // blending SpeedWinds into this one
        void                            InterpolateParameters(CSpeedWind* pWind1, CSpeedWind* pWind2, float fInterpolation);
        void                            BlendParameters(CSpeedWind** pWinds, float* pWeights, unsigned int uiNumWinds);

        // internal use only
        void                            SetUseRandomBuffer(bool bUse, unsigned int uiSeed);
    
private:
        bool                            Load(std::istream& isData);
        bool                            Save(std::ostream& osData) const;

        CSpeedWind*                     m_pFather;

        struct SOscillationParams
        {
                                        SOscillationParams(float fLowWindAngle = 10.0f, float fHighWindAngle = 4.0f, float fLowWindSpeed = 0.01f, float fHighWindSpeed = 0.3f);

                void                    Advance(float fDeltaTime, float fWindStrendth);

                void                    Reset(void) { m_fAdjustedTime = 0.0f; }

                float                   m_fLowWindAngle;
                float                   m_fHighWindAngle;
                float                   m_fLowWindSpeed;
                float                   m_fHighWindSpeed;
                float                   m_fAdjustedTime;
                st_vector_float         m_vOutputs;
        };

        class CSpeedWindController
        {
        public:
                                        CSpeedWindController( );

                void                    Advance(float fDeltaTime, float fP, float fMaxA);
                void                    Reset(float fValue);

        public:
                float                   m_fWantedValue;
                float                   m_fCurrentValue;
                float                   m_fLastDelta;
                float                   m_fLastDeltaTime;
        };

        // SpeedWind parameters
        SOscillationParams              m_sBranchHorizontal;
        SOscillationParams              m_sBranchVertical;
        SOscillationParams              m_sLeafRocking;
        SOscillationParams              m_sLeafRustling;
        float                           m_fMaxBendAngle;
        float                           m_fWindResponse;
        float                           m_fWindResponseLimit;
        float                           m_fGustingStrengthMin;
        float                           m_fGustingStrengthMax;
        float                           m_fGustingFrequency;
        float                           m_fGustingDurationMin;
        float                           m_fGustingDurationMax;
        float                           m_fBranchExponent;
        float                           m_fLeafExponent;

        // SpeedWind internals
        float                           m_fLastTime;
        float                           m_fWindStrength;
        float                           m_fCurrentGustingStrength;
        float                           m_fGustStopTime;

        // SpeedWind outputs
        CSpeedWindController            m_cFinalWindStrength;
        CSpeedWindController            m_cFinalWindDirectionX;
        CSpeedWindController            m_cFinalWindDirectionY;
        CSpeedWindController            m_cFinalWindDirectionZ;
        float                           m_fFinalBendAngle;

        st_vector_stMatrix              m_vWindMatrices;
        st_vector_stMatrix              m_vLeafAngleMatrices;

        // offline rendering support
        st_vector_int                   m_vRandomBuffer;
        int                             m_nRandomIndex;
        bool                            m_bUseRandomBuffer;

        int                             Random(void);

};


/////////////////////////////////////////////////////////////////////////////
//  CSpeedWind redirects

CSpeedWind::CSpeedWind( ) { m_pDefinition = new ("CSpeedWind::CSpeedWind") CSpeedWindDef(this); }
CSpeedWind::~CSpeedWind( ) { st_delete<CSpeedWindDef>(m_pDefinition, "CSpeedWind::~CSpeedWind"); }
CSpeedWind::CSpeedWind(const CSpeedWind& cCopy)
{
    m_pDefinition = new ("CSpeedWind::CSpeedWind(copy)") CSpeedWindDef(this);
    *m_pDefinition = *cCopy.m_pDefinition;
}

// mutators
void CSpeedWind::Reset(void) { m_pDefinition->Reset( ); }
void CSpeedWind::SetQuantities(int nNumWindMatrices, int nNumLeafAngles) { m_pDefinition->SetQuantities(nNumWindMatrices, nNumLeafAngles); }
void CSpeedWind::SetWindResponse(float fResponse, float fReponseLimit) { m_pDefinition->SetWindResponse(fResponse, fReponseLimit); }
void CSpeedWind::SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ) 
    { m_pDefinition->SetWindStrengthAndDirection(fWindStrength, fWindDirectionX, fWindDirectionY, fWindDirectionZ); }
void CSpeedWind::SetMaxBendAngle(float fMaxBendAngle) { m_pDefinition->SetMaxBendAngle(fMaxBendAngle); }
void CSpeedWind::SetExponents(float fBranchExponent, float fLeafExponent) { m_pDefinition->SetExponents(fBranchExponent, fLeafExponent); }
void CSpeedWind::SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax) 
    { m_pDefinition->SetGusting(fGustStrengthMin, fGustStrengthMax, fGustFrequency, fGustDurationMin, fGustDurationMax); }
void CSpeedWind::SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetBranchHorizontal(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetBranchVertical(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetLeafRocking(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) 
    { m_pDefinition->SetLeafRustling(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }

// inspectors
void CSpeedWind::GetQuantities(int& nNumWindMatrices, int& nNumLeafAngles) const { m_pDefinition->GetQuantities(nNumWindMatrices, nNumLeafAngles); }
void CSpeedWind::GetWindResponse(float& fResponse, float& fReponseLimit) const { m_pDefinition->GetWindResponse(fResponse, fReponseLimit); }
void CSpeedWind::GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const
    { m_pDefinition->GetWindStrengthAndDirection(fWindStrength, fWindDirectionX, fWindDirectionY, fWindDirectionZ); }
float CSpeedWind::GetMaxBendAngle(void) const { return m_pDefinition->GetMaxBendAngle( ); }
void CSpeedWind::GetExponents(float& fBranchExponent, float& fLeafExponent) const { m_pDefinition->GetExponents(fBranchExponent, fLeafExponent); }
void CSpeedWind::GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const 
    { m_pDefinition->GetGusting(fGustStrengthMin, fGustStrengthMax, fGustFrequency, fGustDurationMin, fGustDurationMax); }
void CSpeedWind::GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetBranchHorizontal(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetBranchVertical(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const 
    { m_pDefinition->GetLeafRocking(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }
void CSpeedWind::GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
    { m_pDefinition->GetLeafRustling(fLowWindAngle, fHighWindAngle, fLowWindSpeed, fHighWindSpeed); }

// render interface
void CSpeedWind::Advance(float fCurrentTime, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ) 
    { m_pDefinition->Advance(fCurrentTime, bUpdateMatrices, bUpdateLeafAngleMatrices, fCameraX, fCameraY, fCameraZ); }
void CSpeedWind::UpdateSpeedTreeRT(void) { m_pDefinition->UpdateSpeedTreeRT( ); }
void CSpeedWind::UpdateTree(CSpeedTreeRT* pTree) { m_pDefinition->UpdateTree(pTree); }
float CSpeedWind::GetFinalStrength(void) { return m_pDefinition->GetFinalStrength( ); }
unsigned int CSpeedWind::GetNumWindMatrices(void) const { return m_pDefinition->GetNumWindMatrices( ); }
float* CSpeedWind::GetWindMatrix(unsigned int uiIndex) const { return m_pDefinition->GetWindMatrix(uiIndex); }
unsigned int CSpeedWind::GetNumLeafAngleMatrices(void) const { return m_pDefinition->GetNumLeafAngleMatrices( ); }
float* CSpeedWind::GetLeafAngleMatrix(unsigned int uiIndex) const { return m_pDefinition->GetLeafAngleMatrix(uiIndex); }
bool CSpeedWind::GetRustleAngles(float* pAngles) const { return m_pDefinition->GetRustleAngles(pAngles); }
bool CSpeedWind::GetRockAngles(float* pAngles) const { return m_pDefinition->GetRockAngles(pAngles); }

// file I/O
bool CSpeedWind::Load(const char* pFilename) { return m_pDefinition->Load(pFilename); }
bool CSpeedWind::Load(char* pData, unsigned int uiNumBytes) { return m_pDefinition->Load(pData, uiNumBytes); }
bool CSpeedWind::Save(const char* pFilename) const { return m_pDefinition->Save(pFilename); }
char* CSpeedWind::Save(unsigned int& uiNumBytes) const { return m_pDefinition->Save(uiNumBytes); }

// blending SpeedWinds into this one
void CSpeedWind::InterpolateParameters(CSpeedWind* pWind1, CSpeedWind* pWind2, float fInterpolation) { m_pDefinition->InterpolateParameters(pWind1, pWind2, fInterpolation); }
void CSpeedWind::BlendParameters(CSpeedWind** pWinds, float* pWeights, unsigned int uiNumWinds) { m_pDefinition->BlendParameters(pWinds, pWeights, uiNumWinds); }

// offline rendering support
void CSpeedWind::SetUseRandomBuffer(bool bUse, unsigned int uiSeed) { m_pDefinition->SetUseRandomBuffer(bUse, uiSeed); }


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::CSpeedWindMatrix

CSpeedWindMatrix::CSpeedWindMatrix( )
{
    LoadIdentity( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::~CSpeedWindMatrix

CSpeedWindMatrix::~CSpeedWindMatrix( )
{
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::LoadIdentity

inline void CSpeedWindMatrix::LoadIdentity(void)
{
    m_afData[0][0] = 1.0f;
    m_afData[0][1] = 0.0f;
    m_afData[0][2] = 0.0f;
    m_afData[0][3] = 0.0f;
    m_afData[1][0] = 0.0f;
    m_afData[1][1] = 1.0f;
    m_afData[1][2] = 0.0f;
    m_afData[1][3] = 0.0f;
    m_afData[2][0] = 0.0f;
    m_afData[2][1] = 0.0f;
    m_afData[2][2] = 1.1f;
    m_afData[2][3] = 0.0f;
    m_afData[3][0] = 0.0f;
    m_afData[3][1] = 0.0f;
    m_afData[3][2] = 0.0f;
    m_afData[3][3] = 1.0f;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::SetRotation

inline void CSpeedWindMatrix::SetRotation(float fAngle, float fX, float fY, float fZ)
{
    float fS = sinf(fAngle / 57.29578f);
    float fC = cosf(fAngle / 57.29578f);
    float fT = 1.0f - fC;

    m_afData[0][0] = fT * fX * fX + fC;
    m_afData[0][1] = fT * fX * fY + fS * fZ;
    m_afData[0][2] = fT * fX * fZ - fS * fY;
    m_afData[0][3] = 0.0;
    m_afData[1][0] = fT * fX * fY - fS * fZ;
    m_afData[1][1] = fT * fY * fY + fC;
    m_afData[1][2] = fT * fY * fZ + fS * fX;
    m_afData[1][3] = 0.0;
    m_afData[2][0] = fT * fX * fZ + fS * fY;
    m_afData[2][1] = fT * fY * fZ - fS * fX;
    m_afData[2][2] = fT * fZ * fZ + fC;
    m_afData[2][3] = 0.0f;
    m_afData[3][0] = 0.0f;
    m_afData[3][1] = 0.0f;
    m_afData[3][2] = 0.0f;
    m_afData[3][3] = 1.0f;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::RotateX

inline void CSpeedWindMatrix::RotateX(float fAngle)
{
    float fCosine = cosf(fAngle / 57.29578f);
    float fSine = sinf(fAngle / 57.29578f);

    CSpeedWindMatrix cRotMatrix;
    cRotMatrix.m_afData[0][0] = 1.0f;
    cRotMatrix.m_afData[0][1] = 0.0f;
    cRotMatrix.m_afData[0][2] = 0.0f;
    cRotMatrix.m_afData[1][0] = 0.0f;
    cRotMatrix.m_afData[1][1] = fCosine;
    cRotMatrix.m_afData[1][2] = fSine;
    cRotMatrix.m_afData[2][0] = 0.0f;
    cRotMatrix.m_afData[2][1] = -fSine;
    cRotMatrix.m_afData[2][2] = fCosine;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::RotateY

inline void CSpeedWindMatrix::RotateY(float fAngle)
{
    float fCosine = cosf(fAngle / 57.29578f);
    float fSine = sinf(fAngle / 57.29578f);

    CSpeedWindMatrix cRotMatrix;
    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = 0.0f;
    cRotMatrix.m_afData[0][2] = -fSine;
    cRotMatrix.m_afData[1][0] = 0.0f;
    cRotMatrix.m_afData[1][1] = 1.0f;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = fSine;
    cRotMatrix.m_afData[2][1] = 0.0f;
    cRotMatrix.m_afData[2][2] = fCosine;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::RotateZ

inline void CSpeedWindMatrix::RotateZ(float fAngle)
{
    float fCosine = cosf(fAngle / 57.29578f);
    float fSine = sinf(fAngle / 57.29578f);

    CSpeedWindMatrix cRotMatrix;
    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = fSine;
    cRotMatrix.m_afData[0][2] = 0.0f;
    cRotMatrix.m_afData[1][0] = -fSine;
    cRotMatrix.m_afData[1][1] = fCosine;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = 0.0f;
    cRotMatrix.m_afData[2][1] = 0.0f;
    cRotMatrix.m_afData[2][2] = 1.0f;

    // this function can be further optimized by hardcoding
    // the multiplication here and removing terms with 0.0 multipliers.

    *this = cRotMatrix * *this;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::ScaleZ

inline void CSpeedWindMatrix::ScaleZ(float fAmount)
{
    m_afData[2][0] *= fAmount;
    m_afData[2][1] *= fAmount;
    m_afData[2][2] *= fAmount;
    m_afData[2][3] *= fAmount;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindMatrix::operator*

inline CSpeedWindMatrix CSpeedWindMatrix::operator*(const CSpeedWindMatrix& cTrans) const
{
    CSpeedWindMatrix cTemp;
    
    cTemp.m_afData[0][0] = m_afData[0][0] * cTrans.m_afData[0][0] +
                           m_afData[0][1] * cTrans.m_afData[1][0] +
                           m_afData[0][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[0][1] = m_afData[0][0] * cTrans.m_afData[0][1] +
                           m_afData[0][1] * cTrans.m_afData[1][1] +
                           m_afData[0][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[0][2] = m_afData[0][0] * cTrans.m_afData[0][2] +
                           m_afData[0][1] * cTrans.m_afData[1][2] +
                           m_afData[0][2] * cTrans.m_afData[2][2];
    cTemp.m_afData[1][0] = m_afData[1][0] * cTrans.m_afData[0][0] +
                           m_afData[1][1] * cTrans.m_afData[1][0] +
                           m_afData[1][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[1][1] = m_afData[1][0] * cTrans.m_afData[0][1] +
                           m_afData[1][1] * cTrans.m_afData[1][1] +
                           m_afData[1][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[1][2] = m_afData[1][0] * cTrans.m_afData[0][2] +
                           m_afData[1][1] * cTrans.m_afData[1][2] +
                           m_afData[1][2] * cTrans.m_afData[2][2];
    cTemp.m_afData[2][0] = m_afData[2][0] * cTrans.m_afData[0][0] +
                           m_afData[2][1] * cTrans.m_afData[1][0] +
                           m_afData[2][2] * cTrans.m_afData[2][0];
    cTemp.m_afData[2][1] = m_afData[2][0] * cTrans.m_afData[0][1] +
                           m_afData[2][1] * cTrans.m_afData[1][1] +
                           m_afData[2][2] * cTrans.m_afData[2][1];
    cTemp.m_afData[2][2] = m_afData[2][0] * cTrans.m_afData[0][2] +
                           m_afData[2][1] * cTrans.m_afData[1][2] +
                           m_afData[2][2] * cTrans.m_afData[2][2];

    return cTemp;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::CSpeedWindController

CSpeedWindDef::CSpeedWindController::CSpeedWindController(void) :
    m_fWantedValue(0.0f),
    m_fCurrentValue(0.0f),
    m_fLastDelta(0.0f),
    m_fLastDeltaTime(0.0f)
{
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::Advance

void CSpeedWindDef::CSpeedWindController::Advance(float fDeltaTime, float fP, float fMaxA)
{
    const float c_fMaxControllableDeltaTime = 0.03f;

    if (fDeltaTime > c_fMaxControllableDeltaTime)
        fDeltaTime = c_fMaxControllableDeltaTime;

    float fDelta = fP * fDeltaTime * (m_fWantedValue - m_fCurrentValue);

    if (fMaxA != 0.0f)
    {
        if (fabs(fDelta) > fabs(m_fLastDelta) && fDeltaTime > 0.0f && m_fLastDeltaTime > 0.0f)
        {
            float fAccel = fDelta / fDeltaTime - m_fLastDelta / m_fLastDeltaTime;
            if (fAccel < -fMaxA)
                fAccel = -fMaxA;
            if (fAccel > fMaxA)
                fAccel = fMaxA;

            fDelta = m_fLastDelta + fAccel * fDeltaTime;
        }

        m_fLastDelta = fDelta;
        m_fLastDeltaTime = fDeltaTime;
    }

    m_fCurrentValue += fDelta;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindController::Reset

void CSpeedWindDef::CSpeedWindController::Reset(float fValue)
{
    m_fCurrentValue = fValue;
    m_fWantedValue = fValue;
    m_fLastDelta = 0.0f;
    m_fLastDeltaTime = 0.0f;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SOscillationParams::SOscillationParams

CSpeedWindDef::SOscillationParams::SOscillationParams(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed) :
    m_fLowWindAngle(fLowWindAngle),
    m_fHighWindAngle(fHighWindAngle),
    m_fLowWindSpeed(fLowWindSpeed),
    m_fHighWindSpeed(fHighWindSpeed),
    m_fAdjustedTime(0.0f)
{
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SOscillationParams::Advance

void CSpeedWindDef::SOscillationParams::Advance(float fDeltaTime, float fWindStrendth)
{
    float fAngle = VecInterpolate(m_fLowWindAngle, m_fHighWindAngle, fWindStrendth);
    float fSpeed = VecInterpolate(m_fLowWindSpeed, m_fHighWindSpeed, fWindStrendth);
    m_fAdjustedTime += fDeltaTime * fSpeed;

    unsigned int uiSize = (unsigned int) m_vOutputs.size( );
    for (unsigned int i = 0; i < uiSize; ++i)
        m_vOutputs[i] = fAngle * sinf(m_fAdjustedTime + (float)i);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::CSpeedWindDef

CSpeedWindDef::CSpeedWindDef(CSpeedWind* pFather) :
    m_pFather(pFather),
    m_fMaxBendAngle(0.0f),
    m_fWindResponse(1.0f),
    m_fWindResponseLimit(0.0f),
    m_fGustingStrengthMin(0.0f),
    m_fGustingStrengthMax(0.0f),
    m_fGustingFrequency(0.0f),
    m_fGustingDurationMin(0.0f),
    m_fGustingDurationMax(0.0f),
    m_fBranchExponent(1.0f),
    m_fLeafExponent(1.0f),
    m_fLastTime(0.0f),
    m_fWindStrength(0.0f),
    m_fCurrentGustingStrength(0.0f),
    m_fGustStopTime(0.0f),
    m_fFinalBendAngle(0.0f),
    m_nRandomIndex(0),
    m_bUseRandomBuffer(false)
{
    SetQuantities(1, 1);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::~CSpeedWindDef

CSpeedWindDef::~CSpeedWindDef( )
{
    m_pFather = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetQuantities

void CSpeedWindDef::SetQuantities(int iNumWindMatrices, int iNumLeafAngles)
{
    if (iNumWindMatrices < 1)
        iNumWindMatrices = 1;

    if (iNumLeafAngles < 1)
        iNumLeafAngles = 1;

    m_vWindMatrices.clear( );
    m_vWindMatrices.resize(iNumWindMatrices);

    m_sBranchHorizontal.m_vOutputs.clear( );
    m_sBranchVertical.m_vOutputs.clear( );
    m_sBranchHorizontal.m_vOutputs.resize(iNumWindMatrices);
    m_sBranchVertical.m_vOutputs.resize(iNumWindMatrices);

    m_sLeafRocking.m_vOutputs.clear( );
    m_sLeafRustling.m_vOutputs.clear( );
    m_vLeafAngleMatrices.clear( );
    m_sLeafRocking.m_vOutputs.resize(iNumLeafAngles);
    m_sLeafRustling.m_vOutputs.resize(iNumLeafAngles);
    m_vLeafAngleMatrices.resize(iNumLeafAngles);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetWindResponse

void CSpeedWindDef::SetWindResponse(float fResponse, float fReponseLimit)
{
    m_fWindResponse = fResponse;
    m_fWindResponseLimit = fReponseLimit;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetWindStrengthAndDirection

void CSpeedWindDef::SetWindStrengthAndDirection(float fWindStrength, float fWindDirectionX, float fWindDirectionY, float fWindDirectionZ)
{
    m_fWindStrength = st_clamp<float>(fWindStrength, 0.0f, 1.0f);

    float fSum = fWindDirectionX * fWindDirectionX + fWindDirectionY * fWindDirectionY + fWindDirectionZ * fWindDirectionZ;
    if (fSum != 0.0f)
    {
        fSum = sqrt(fSum);
        fWindDirectionX /= fSum;
        fWindDirectionY /= fSum;
        fWindDirectionZ /= fSum;
    }
    m_cFinalWindDirectionX.m_fWantedValue = fWindDirectionX;
    m_cFinalWindDirectionY.m_fWantedValue = fWindDirectionY;
    m_cFinalWindDirectionZ.m_fWantedValue = fWindDirectionZ;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetMaxBendAngle

void CSpeedWindDef::SetMaxBendAngle(float fMaxBendAngle)
{
    m_fMaxBendAngle = fMaxBendAngle;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetExponents

void CSpeedWindDef::SetExponents(float fBranchExponent, float fLeafExponent)
{
    m_fBranchExponent = fBranchExponent;
    m_fLeafExponent = fLeafExponent;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetGusting

void CSpeedWindDef::SetGusting(float fGustStrengthMin, float fGustStrengthMax, float fGustFrequency, float fGustDurationMin, float fGustDurationMax)
{
    m_fGustingStrengthMin = fGustStrengthMin;
    m_fGustingStrengthMax = fGustStrengthMax;
    m_fGustingFrequency = fGustFrequency;
    m_fGustingDurationMin = fGustDurationMin;
    m_fGustingDurationMax = fGustDurationMax;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetBranchHorizontal

void CSpeedWindDef::SetBranchHorizontal(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sBranchHorizontal.m_fLowWindAngle = fLowWindAngle;
    m_sBranchHorizontal.m_fHighWindAngle = fHighWindAngle;
    m_sBranchHorizontal.m_fLowWindSpeed = fLowWindSpeed;
    m_sBranchHorizontal.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetBranchVertical

void CSpeedWindDef::SetBranchVertical(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sBranchVertical.m_fLowWindAngle = fLowWindAngle;
    m_sBranchVertical.m_fHighWindAngle = fHighWindAngle;
    m_sBranchVertical.m_fLowWindSpeed = fLowWindSpeed;
    m_sBranchVertical.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetLeafRocking

void CSpeedWindDef::SetLeafRocking(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sLeafRocking.m_fLowWindAngle = fLowWindAngle;
    m_sLeafRocking.m_fHighWindAngle = fHighWindAngle;
    m_sLeafRocking.m_fLowWindSpeed = fLowWindSpeed;
    m_sLeafRocking.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetLeafRustling

void CSpeedWindDef::SetLeafRustling(float fLowWindAngle, float fHighWindAngle, float fLowWindSpeed, float fHighWindSpeed)
{
    m_sLeafRustling.m_fLowWindAngle = fLowWindAngle;
    m_sLeafRustling.m_fHighWindAngle = fHighWindAngle;
    m_sLeafRustling.m_fLowWindSpeed = fLowWindSpeed;
    m_sLeafRustling.m_fHighWindSpeed = fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Advance

void CSpeedWindDef::Advance(float fCurrentTime, bool bUpdateMatrices, bool bUpdateLeafAngleMatrices, float fCameraX, float fCameraY, float fCameraZ)
{
    float fDeltaTime = fCurrentTime - m_fLastTime;
    m_fLastTime = fCurrentTime;

    // stop current gust if needed
    if (fCurrentTime > m_fGustStopTime)
    {
        m_fCurrentGustingStrength = 0.0f;
    
        // make new gusts
        if (m_fGustingFrequency * fDeltaTime > (float)Random( ) / (float)RAND_MAX)
        {
            m_fCurrentGustingStrength = (float)Random( ) * (m_fGustingStrengthMax - m_fGustingStrengthMin) / (float)RAND_MAX + m_fGustingStrengthMin;
            m_fGustStopTime = fCurrentTime + (float)Random( ) * (m_fGustingDurationMax - m_fGustingDurationMin) / (float)RAND_MAX + m_fGustingDurationMin;
        }
    }

    // compute the actual wind strength due to gusting
    m_cFinalWindStrength.m_fWantedValue = m_fWindStrength + m_fCurrentGustingStrength;
    m_cFinalWindStrength.m_fWantedValue = st_clamp<float>(m_cFinalWindStrength.m_fWantedValue, 0.0f, 1.0f);
    m_cFinalWindStrength.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindStrength.m_fCurrentValue = st_clamp<float>(m_cFinalWindStrength.m_fCurrentValue, 0.0f, 1.0f);

    // smooth out the direction
    m_cFinalWindDirectionX.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindDirectionY.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);
    m_cFinalWindDirectionZ.Advance(fDeltaTime, m_fWindResponse, m_fWindResponseLimit);

    // compute the branch and leaf wind strengths due to exponents
    float fBranchStrength = powf(m_cFinalWindStrength.m_fCurrentValue, m_fBranchExponent);
    float fLeafStrength = powf(m_cFinalWindStrength.m_fCurrentValue, m_fLeafExponent);

    // update the main tree bend angle (and compensate for unnormalized, over the pole movement)
    float fSum = m_cFinalWindDirectionX.m_fCurrentValue * m_cFinalWindDirectionX.m_fCurrentValue + 
                 m_cFinalWindDirectionY.m_fCurrentValue * m_cFinalWindDirectionY.m_fCurrentValue + 
                 m_cFinalWindDirectionZ.m_fCurrentValue * m_cFinalWindDirectionZ.m_fCurrentValue;
    if (fSum != 0.0f)
        fSum = sqrt(fSum);
    m_fFinalBendAngle = fBranchStrength * m_fMaxBendAngle * fSum;

    // update the oscillating parts
    m_sBranchHorizontal.Advance(fDeltaTime, fBranchStrength);
    m_sBranchVertical.Advance(fDeltaTime, fBranchStrength);
    m_sLeafRocking.Advance(fDeltaTime, fLeafStrength);
    m_sLeafRustling.Advance(fDeltaTime, fLeafStrength);

    if (bUpdateMatrices)
    {
        // build wind matrices
        float fWindAngle = -static_cast<float>(atan2(m_cFinalWindDirectionX.m_fCurrentValue, m_cFinalWindDirectionY.m_fCurrentValue));
        float fXRot = -cosf(fWindAngle);
        float fYRot = -sinf(fWindAngle);

        CSpeedWindMatrix cTemp, cExp;
        unsigned int uiSize = (unsigned int)m_vWindMatrices.size( );
        for (unsigned int i = 0; i < uiSize; ++i)
        {
            // vertical oscillation and bend angle are in the same direction, so do them at the same time
            m_vWindMatrices[i].SetRotation(m_fFinalBendAngle + m_sBranchVertical.m_vOutputs[i], fXRot, fYRot, 0.0f);

            // handle horizontal oscillation
            cTemp.SetRotation(m_sBranchHorizontal.m_vOutputs[i], -fYRot, fXRot, 0.0f);

            // final matrix
            m_vWindMatrices[i] = m_vWindMatrices[i] * cTemp;
        }
    }

    if (bUpdateLeafAngleMatrices)
    {
        // build leaf angle matrices
        float afAdjustedCameraDir[3] = 
        {
#ifdef UPVECTOR_POS_Z
            fCameraX, fCameraY, fCameraZ
#endif
#ifdef UPVECTOR_NEG_Z
            -fCameraX, fCameraY, -fCameraZ
#endif
#ifdef UPVECTOR_POS_Y
            -fCameraX, fCameraZ, fCameraY
#endif
#ifdef UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
            fCameraY, fCameraX, fCameraZ
#endif
        };

        float fAzimuth = VecRad2Deg(atan2f(afAdjustedCameraDir[1], afAdjustedCameraDir[0]));
        float fPitch = VecRad2Deg(-asinf(afAdjustedCameraDir[2]));

        unsigned int uiSize = (unsigned int) m_vLeafAngleMatrices.size( );
        for (unsigned int i = 0; i < uiSize; ++i)
        {
            CSpeedWindMatrix& cMatrix = m_vLeafAngleMatrices[i];

            cMatrix.LoadIdentity( );
#ifdef UPVECTOR_POS_Z
                cMatrix.RotateZ(fAzimuth);
                cMatrix.RotateY(fPitch);
                cMatrix.RotateZ(m_sLeafRustling.m_vOutputs[i]);
                cMatrix.RotateX(m_sLeafRocking.m_vOutputs[i]);
#endif
#ifdef UPVECTOR_NEG_Z
                cMatrix.RotateZ(-fAzimuth);
                cMatrix.RotateY(fPitch);
                cMatrix.RotateZ(m_sLeafRustling.m_vOutputs[i]);
                cMatrix.RotateX(m_sLeafRocking.m_vOutputs[i]);
#endif
#ifdef UPVECTOR_POS_Y
                cMatrix.RotateY(fAzimuth);
                cMatrix.RotateZ(fPitch);
                cMatrix.RotateY(-m_sLeafRustling.m_vOutputs[i]);
                cMatrix.RotateX(m_sLeafRocking.m_vOutputs[i]);
#endif
#ifdef UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
                cMatrix.RotateZ(-fAzimuth);
                cMatrix.RotateX(-fPitch);
                cMatrix.RotateZ(m_sLeafRustling.m_vOutputs[i]);
                cMatrix.RotateY(m_sLeafRocking.m_vOutputs[i]);
#endif
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::UpdateTree

void CSpeedWindDef::UpdateTree(CSpeedTreeRT* pTree)
{
    pTree->SetWindStrengthAndLeafAngles(m_cFinalWindStrength.m_fCurrentValue, 
                                        &(m_sLeafRocking.m_vOutputs[0]), 
                                        &(m_sLeafRustling.m_vOutputs[0]), 
                                        int(m_sLeafRocking.m_vOutputs.size( )));
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::UpdateSpeedTreeRT

void CSpeedWindDef::UpdateSpeedTreeRT(void)
{
    unsigned int uiSize = (unsigned int) m_vWindMatrices.size( );
    for (unsigned int i = 0; i < uiSize; ++i)
        CSpeedTreeRT::SetWindMatrix(i, (const float*)m_vWindMatrices[i].m_afData);
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetFinalStrength

float CSpeedWindDef::GetFinalStrength(void)
{
    return m_cFinalWindStrength.m_fCurrentValue;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetNumWindMatrices

unsigned int CSpeedWindDef::GetNumWindMatrices(void) const
{
    return (unsigned int) m_vWindMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindMatrix

float* CSpeedWindDef::GetWindMatrix(unsigned int uiIndex) const
{
    float* pReturn = NULL;

    if (uiIndex < m_vWindMatrices.size( ))
        pReturn = ((float*)m_vWindMatrices[uiIndex].m_afData);

    return pReturn;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetNumWindMatrices

unsigned int CSpeedWindDef::GetNumLeafAngleMatrices(void) const
{
    return (unsigned int) m_vLeafAngleMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafAngleMatrix

float* CSpeedWindDef::GetLeafAngleMatrix(unsigned int uiIndex) const
{
    float* pReturn = NULL;

    if (uiIndex < m_vWindMatrices.size( ))
        pReturn = ((float*)m_vLeafAngleMatrices[uiIndex].m_afData);

    return pReturn;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Load

bool CSpeedWindDef::Load(const st_string& strFilename)
{
    bool bSuccess = false;

    ifstream isData(strFilename.c_str( ));
    if (isData.is_open( ))
    {
        bSuccess = Load(isData);
        isData.close( );
    }

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Load

bool CSpeedWindDef::Load(char* pData, unsigned int uiNumBytes)
{
    st_assert(pData);

    bool bSuccess = false;
    // convert to a stream
    stringstream ssWrapper(ios_base::in | ios_base::out | ios_base::binary);
    ssWrapper.write(pData, uiNumBytes);
    bSuccess = Load(ssWrapper);

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Load

bool CSpeedWindDef::Load(istream& isData)
{
    bool bSuccess = false;

#ifndef STRIP_EXCEPTION_HANDLING    
    isData.exceptions(istream::failbit | istream::badbit | istream::eofbit);

    try
    {
#endif
        st_string strToken;
        float fTemp0, fTemp1, fTemp2, fTemp3;
        fTemp0 = fTemp1 = fTemp2 = fTemp3 = 0.0f;

        int iLinesRead = 0;
        while (iLinesRead < 12)
        {
            isData >> strToken;

            if (strToken == "NumWindMatricesAndLeafAngles")
            {
                isData >> fTemp0 >> fTemp1;
                SetQuantities((int)fTemp0, (int)fTemp1);
                ++iLinesRead;
            }
            else if (strToken == "WindResponseAndLimit")
            {
                isData >> fTemp0 >> fTemp1;
                SetWindResponse(fTemp0, fTemp1);
                ++iLinesRead;
            }
            else if (strToken == "MaxBendAngle")
            {
                isData >> m_fMaxBendAngle;
                ++iLinesRead;
            }
            else if (strToken == "BranchExponent")
            {
                isData >> m_fBranchExponent;
                ++iLinesRead;
            }
            else if (strToken == "LeafExponent")
            {
                isData >> m_fLeafExponent;
                ++iLinesRead;
            }
            else if (strToken == "GustStrengthFreqDuration")
            {
                // old gusting parameters
                isData >> fTemp0 >> fTemp1 >> fTemp2;
                SetGusting(fTemp0 * 0.25f, fTemp0 * 1.25f, fTemp1, fTemp2 * 0.5f, fTemp2 * 1.5f);
                iLinesRead += 3;
            }
            else if (strToken == "GustStrengthMinMax")
            {
                isData >> m_fGustingStrengthMin >> m_fGustingStrengthMax;
                ++iLinesRead;
            }
            else if (strToken == "GustDurationMinMax")
            {
                isData >> m_fGustingDurationMin >> m_fGustingDurationMax;
                ++iLinesRead;
            }
            else if (strToken == "GustFrequency")
            {
                isData >> m_fGustingFrequency;
                ++iLinesRead;
            }
            else if (strToken == "BranchOscillationX_LaLsHaHs")
            {
                isData >> fTemp0 >> fTemp1 >> fTemp2 >> fTemp3;
                SetBranchHorizontal(fTemp0, fTemp2, fTemp1, fTemp3);
                ++iLinesRead;
            }
            else if (strToken == "BranchOscillationY_LaLsHaHs")
            {
                isData >> fTemp0 >> fTemp1 >> fTemp2 >> fTemp3;
                SetBranchVertical(fTemp0, fTemp2, fTemp1, fTemp3);
                ++iLinesRead;
            }
            else if (strToken == "LeafRocking_LaLsHaHs")
            {
                isData >> fTemp0 >> fTemp1 >> fTemp2 >> fTemp3;
                SetLeafRocking(fTemp0, fTemp2, fTemp1, fTemp3);
                ++iLinesRead;
            }
            else if (strToken == "LeafRustling_LaLsHaHs")
            {
                isData >> fTemp0 >> fTemp1 >> fTemp2 >> fTemp3;
                SetLeafRustling(fTemp0, fTemp2, fTemp1, fTemp3);
                ++iLinesRead;
            }
        }

        bSuccess = true;

#ifndef STRIP_EXCEPTION_HANDLING    
    }

    catch (istream::failure e)
    {
        bSuccess = false;
    }
#endif

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Save

bool CSpeedWindDef::Save(const st_string& strFilename) const
{
    bool bSuccess = false;

    ofstream osData(strFilename.c_str( ));
    if (osData.is_open( ))
    {
        bSuccess = Save(osData);
        osData.close( );
    }

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Save

char* CSpeedWindDef::Save(unsigned int& uiNumBytes) const
{
    uiNumBytes = 0;
    char* pSaveData = NULL;

    ostringstream osData;
    if (Save(osData))
    {
        st_string strData = st_string(osData.str( ).c_str( ));
        uiNumBytes = static_cast<unsigned int>(strData.length( ));
        pSaveData = st_new_array<char>(uiNumBytes, "CSpeedWindDef::Save");
        for (unsigned int i = 0; i < uiNumBytes; ++i)
            pSaveData[i] = strData[i];
    }

    return pSaveData;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Save

bool CSpeedWindDef::Save(ostream& osData) const
{
    bool bSuccess = false;

#ifndef STRIP_EXCEPTION_HANDLING
    osData.exceptions(ostream::failbit | ostream::badbit);

    try
    {
#endif
        osData << "NumWindMatricesAndLeafAngles " << (unsigned int) m_vWindMatrices.size( ) << " " << (unsigned int) m_sLeafRocking.m_vOutputs.size( ) << endl;
        osData << "WindResponseAndLimit " << m_fWindResponse << " " << m_fWindResponseLimit << endl;
        osData << "MaxBendAngle " << m_fMaxBendAngle << endl;
        osData << "BranchExponent " << m_fBranchExponent << endl;
        osData << "LeafExponent " << m_fLeafExponent << endl;
        osData << "GustStrengthMinMax " << m_fGustingStrengthMin << " " << m_fGustingStrengthMax << endl;
        osData << "GustDurationMinMax " << m_fGustingDurationMin << " " << m_fGustingDurationMax << endl;
        osData << "GustFrequency " << m_fGustingFrequency << endl;
        osData << "BranchOscillationX_LaLsHaHs " << m_sBranchHorizontal.m_fLowWindAngle << " "
                                                    << m_sBranchHorizontal.m_fLowWindSpeed << " "
                                                    << m_sBranchHorizontal.m_fHighWindAngle  << " "
                                                    << m_sBranchHorizontal.m_fHighWindSpeed << endl;
        osData << "BranchOscillationY_LaLsHaHs " << m_sBranchVertical.m_fLowWindAngle << " "
                                                    << m_sBranchVertical.m_fLowWindSpeed << " "
                                                    << m_sBranchVertical.m_fHighWindAngle  << " "
                                                    << m_sBranchVertical.m_fHighWindSpeed << endl;
        osData << "LeafRocking_LaLsHaHs " << m_sLeafRocking.m_fLowWindAngle << " "
                                            << m_sLeafRocking.m_fLowWindSpeed << " "
                                            << m_sLeafRocking.m_fHighWindAngle  << " "
                                            << m_sLeafRocking.m_fHighWindSpeed << endl;
        osData << "LeafRustling_LaLsHaHs " << m_sLeafRustling.m_fLowWindAngle << " "
                                            << m_sLeafRustling.m_fLowWindSpeed << " "
                                            << m_sLeafRustling.m_fHighWindAngle  << " "
                                            << m_sLeafRustling.m_fHighWindSpeed << endl;

        bSuccess = true;

#ifndef STRIP_EXCEPTION_HANDLING
    }
    catch (ostream::failure e)
    {
        bSuccess = false;
    }
#endif

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Reset

void CSpeedWindDef::Reset(void)
{
    unsigned int i = 0;
    unsigned int uiSize = (unsigned int) m_vWindMatrices.size( );
    for (i = 0; i < uiSize; ++i)
        m_vWindMatrices[i].LoadIdentity( );

    uiSize = (unsigned int) m_vLeafAngleMatrices.size( );
    for (i = 0; i < uiSize; ++i)
        m_vLeafAngleMatrices[i].LoadIdentity( );

    m_cFinalWindStrength.Reset(0.0f);
    m_cFinalWindDirectionX.Reset(0.0f);
    m_cFinalWindDirectionY.Reset(0.0f);
    m_cFinalWindDirectionZ.Reset(0.0f);

    m_nRandomIndex = 0;
    m_fCurrentGustingStrength = 0.0f;
    m_fGustStopTime = 0.0f;
    m_fLastTime = 0.0f;

    m_sBranchHorizontal.Reset( );
    m_sBranchVertical.Reset( );
    m_sLeafRocking.Reset( );
    m_sLeafRustling.Reset( );}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetQuantities

void CSpeedWindDef::GetQuantities(int& iNumWindMatrices, int& iNumLeafAngles) const
{
    iNumWindMatrices = (int)m_vWindMatrices.size( );
    iNumLeafAngles = (int)m_vLeafAngleMatrices.size( );
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindResponse

void CSpeedWindDef::GetWindResponse(float& fResponse, float& fReponseLimit) const
{
    fResponse = m_fWindResponse;
    fReponseLimit = m_fWindResponseLimit;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetWindStrengthAndDirection

void CSpeedWindDef::GetWindStrengthAndDirection(float& fWindStrength, float& fWindDirectionX, float& fWindDirectionY, float& fWindDirectionZ) const
{
    fWindStrength = m_fWindStrength;
    fWindDirectionX = m_cFinalWindDirectionX.m_fWantedValue;
    fWindDirectionY = m_cFinalWindDirectionY.m_fWantedValue;
    fWindDirectionZ = m_cFinalWindDirectionZ.m_fWantedValue;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetMaxBendAngle

float CSpeedWindDef::GetMaxBendAngle(void) const
{
    return m_fMaxBendAngle;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetExponents

void CSpeedWindDef::GetExponents(float& fBranchExponent, float& fLeafExponent) const
{
    fBranchExponent = m_fBranchExponent;
    fLeafExponent = m_fLeafExponent;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetGusting

void CSpeedWindDef::GetGusting(float& fGustStrengthMin, float& fGustStrengthMax, float& fGustFrequency, float& fGustDurationMin, float& fGustDurationMax) const
{
    fGustStrengthMin = m_fGustingStrengthMin;
    fGustStrengthMax = m_fGustingStrengthMax;
    fGustFrequency = m_fGustingFrequency;
    fGustDurationMin = m_fGustingDurationMin;
    fGustDurationMax = m_fGustingDurationMax;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetBranchHorizontal

void CSpeedWindDef::GetBranchHorizontal(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sBranchHorizontal.m_fLowWindAngle;
    fHighWindAngle = m_sBranchHorizontal.m_fHighWindAngle;
    fLowWindSpeed = m_sBranchHorizontal.m_fLowWindSpeed;
    fHighWindSpeed = m_sBranchHorizontal.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetBranchVertical

void CSpeedWindDef::GetBranchVertical(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sBranchVertical.m_fLowWindAngle;
    fHighWindAngle = m_sBranchVertical.m_fHighWindAngle;
    fLowWindSpeed = m_sBranchVertical.m_fLowWindSpeed;
    fHighWindSpeed = m_sBranchVertical.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafRocking

void CSpeedWindDef::GetLeafRocking(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sLeafRocking.m_fLowWindAngle;
    fHighWindAngle = m_sLeafRocking.m_fHighWindAngle;
    fLowWindSpeed = m_sLeafRocking.m_fLowWindSpeed;
    fHighWindSpeed = m_sLeafRocking.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetLeafRustling

void CSpeedWindDef::GetLeafRustling(float& fLowWindAngle, float& fHighWindAngle, float& fLowWindSpeed, float& fHighWindSpeed) const
{
    fLowWindAngle = m_sLeafRustling.m_fLowWindAngle;
    fHighWindAngle = m_sLeafRustling.m_fHighWindAngle;
    fLowWindSpeed = m_sLeafRustling.m_fLowWindSpeed;
    fHighWindSpeed = m_sLeafRustling.m_fHighWindSpeed;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetRustleAngles

bool CSpeedWindDef::GetRustleAngles(float* pAngles) const
{
    bool bSuccess = false;

    int nNumLeaves = int(m_sLeafRustling.m_vOutputs.size( ));

    st_assert(pAngles);
    memcpy(pAngles, &m_sLeafRustling.m_vOutputs[0], nNumLeaves * sizeof(float));
    bSuccess = true;

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::GetRockAngles

bool CSpeedWindDef::GetRockAngles(float* pAngles) const
{
    bool bSuccess = false;

    int nNumLeaves = int(m_sLeafRocking.m_vOutputs.size( ));

    st_assert(pAngles);
    memcpy(pAngles, &m_sLeafRocking.m_vOutputs[0], nNumLeaves * sizeof(float));
    bSuccess = true;

    return bSuccess;
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::InterpolateParameters

void CSpeedWindDef::InterpolateParameters(CSpeedWind* pWind1, CSpeedWind* pWind2, float fInterpolation)
{
#define VecInterpolate_WIND_PARAM(a) a = VecInterpolate(pWind1->m_pDefinition->a, pWind2->m_pDefinition->a, fInterpolation)

    if (pWind1 != NULL && pWind2 != NULL)
    {
        VecInterpolate_WIND_PARAM(m_fWindStrength);
        VecInterpolate_WIND_PARAM(m_cFinalWindDirectionX.m_fWantedValue);
        VecInterpolate_WIND_PARAM(m_cFinalWindDirectionY.m_fWantedValue);
        VecInterpolate_WIND_PARAM(m_cFinalWindDirectionZ.m_fWantedValue);

        // normalize direction
        float fSum = m_cFinalWindDirectionX.m_fWantedValue * m_cFinalWindDirectionX.m_fWantedValue + 
                    m_cFinalWindDirectionY.m_fWantedValue * m_cFinalWindDirectionY.m_fWantedValue + 
                    m_cFinalWindDirectionZ.m_fWantedValue * m_cFinalWindDirectionZ.m_fWantedValue;
        if (fSum != 0.0f)
        {
            fSum = sqrt(fSum);
            m_cFinalWindDirectionX.m_fWantedValue /= fSum;
            m_cFinalWindDirectionY.m_fWantedValue /= fSum;
            m_cFinalWindDirectionZ.m_fWantedValue /= fSum;
        }

        VecInterpolate_WIND_PARAM(m_sBranchHorizontal.m_fLowWindAngle);
        VecInterpolate_WIND_PARAM(m_sBranchHorizontal.m_fHighWindAngle);
        VecInterpolate_WIND_PARAM(m_sBranchHorizontal.m_fLowWindSpeed);
        VecInterpolate_WIND_PARAM(m_sBranchHorizontal.m_fHighWindSpeed);

        VecInterpolate_WIND_PARAM(m_sBranchVertical.m_fLowWindAngle);
        VecInterpolate_WIND_PARAM(m_sBranchVertical.m_fHighWindAngle);
        VecInterpolate_WIND_PARAM(m_sBranchVertical.m_fLowWindSpeed);
        VecInterpolate_WIND_PARAM(m_sBranchVertical.m_fHighWindSpeed);

        VecInterpolate_WIND_PARAM(m_sLeafRocking.m_fLowWindAngle);
        VecInterpolate_WIND_PARAM(m_sLeafRocking.m_fHighWindAngle);
        VecInterpolate_WIND_PARAM(m_sLeafRocking.m_fLowWindSpeed);
        VecInterpolate_WIND_PARAM(m_sLeafRocking.m_fHighWindSpeed);

        VecInterpolate_WIND_PARAM(m_sLeafRustling.m_fLowWindAngle);
        VecInterpolate_WIND_PARAM(m_sLeafRustling.m_fHighWindAngle);
        VecInterpolate_WIND_PARAM(m_sLeafRustling.m_fLowWindSpeed);
        VecInterpolate_WIND_PARAM(m_sLeafRustling.m_fHighWindSpeed);

        VecInterpolate_WIND_PARAM(m_fMaxBendAngle);
        VecInterpolate_WIND_PARAM(m_fWindResponse);
        VecInterpolate_WIND_PARAM(m_fWindResponseLimit);
        VecInterpolate_WIND_PARAM(m_fGustingStrengthMin);
        VecInterpolate_WIND_PARAM(m_fGustingStrengthMax);
        VecInterpolate_WIND_PARAM(m_fGustingFrequency);
        VecInterpolate_WIND_PARAM(m_fGustingDurationMin);
        VecInterpolate_WIND_PARAM(m_fGustingDurationMax);
        VecInterpolate_WIND_PARAM(m_fBranchExponent);
        VecInterpolate_WIND_PARAM(m_fLeafExponent);
        
    }

#undef VecInterpolate_WIND_PARAM
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::BlendParameters

void CSpeedWindDef::BlendParameters(CSpeedWind** pWinds, float* pWeights, unsigned int uiNumWinds)
{
    // zero out our data
    m_fWindStrength = 0.0f;
    m_cFinalWindDirectionX.m_fWantedValue = 0.0f;
    m_cFinalWindDirectionY.m_fWantedValue = 0.0f;
    m_cFinalWindDirectionZ.m_fWantedValue = 0.0f;

    m_sBranchHorizontal.m_fLowWindAngle = 0.0f;
    m_sBranchHorizontal.m_fHighWindAngle = 0.0f;
    m_sBranchHorizontal.m_fLowWindSpeed = 0.0f;
    m_sBranchHorizontal.m_fHighWindSpeed = 0.0f;

    m_sBranchVertical.m_fLowWindAngle = 0.0f;
    m_sBranchVertical.m_fHighWindAngle = 0.0f;
    m_sBranchVertical.m_fLowWindSpeed = 0.0f;
    m_sBranchVertical.m_fHighWindSpeed = 0.0f;

    m_sLeafRocking.m_fLowWindAngle = 0.0f;
    m_sLeafRocking.m_fHighWindAngle = 0.0f;
    m_sLeafRocking.m_fLowWindSpeed = 0.0f;
    m_sLeafRocking.m_fHighWindSpeed = 0.0f;

    m_sLeafRustling.m_fLowWindAngle = 0.0f;
    m_sLeafRustling.m_fHighWindAngle = 0.0f;
    m_sLeafRustling.m_fLowWindSpeed = 0.0f;
    m_sLeafRustling.m_fHighWindSpeed = 0.0f;

    m_fMaxBendAngle = 0.0f;
    m_fWindResponse = 0.0f;
    m_fWindResponseLimit = 0.0f;
    m_fGustingStrengthMin = 0.0f;
    m_fGustingStrengthMax = 0.0f;
    m_fGustingFrequency = 0.0f;
    m_fGustingDurationMin = 0.0f;
    m_fGustingDurationMax = 0.0f;
    m_fBranchExponent = 0.0f;
    m_fLeafExponent = 0.0f;

    
#define SUM_WIND_PARAM(a) a = pWinds[i]->m_pDefinition->a * pWeights[i]

    // fill data based on winds and weights
    for (unsigned int i = 0; i < uiNumWinds; ++i)
    {
        SUM_WIND_PARAM(m_fWindStrength);
        SUM_WIND_PARAM(m_cFinalWindDirectionX.m_fWantedValue);
        SUM_WIND_PARAM(m_cFinalWindDirectionY.m_fWantedValue);
        SUM_WIND_PARAM(m_cFinalWindDirectionZ.m_fWantedValue);

        SUM_WIND_PARAM(m_sBranchHorizontal.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sBranchHorizontal.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sBranchVertical.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sBranchVertical.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sBranchVertical.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sBranchVertical.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sLeafRocking.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sLeafRocking.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sLeafRocking.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sLeafRocking.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_sLeafRustling.m_fLowWindAngle);
        SUM_WIND_PARAM(m_sLeafRustling.m_fHighWindAngle);
        SUM_WIND_PARAM(m_sLeafRustling.m_fLowWindSpeed);
        SUM_WIND_PARAM(m_sLeafRustling.m_fHighWindSpeed);

        SUM_WIND_PARAM(m_fMaxBendAngle);
        SUM_WIND_PARAM(m_fWindResponse);
        SUM_WIND_PARAM(m_fWindResponseLimit);
        SUM_WIND_PARAM(m_fGustingStrengthMin);
        SUM_WIND_PARAM(m_fGustingStrengthMax);
        SUM_WIND_PARAM(m_fGustingFrequency);
        SUM_WIND_PARAM(m_fGustingDurationMin);
        SUM_WIND_PARAM(m_fGustingDurationMax);
        SUM_WIND_PARAM(m_fBranchExponent);
        SUM_WIND_PARAM(m_fLeafExponent);
    }

#undef SUM_WIND_PARAM

    // normalize direction
    float fSum = m_cFinalWindDirectionX.m_fWantedValue * m_cFinalWindDirectionX.m_fWantedValue + 
                m_cFinalWindDirectionY.m_fWantedValue * m_cFinalWindDirectionY.m_fWantedValue + 
                m_cFinalWindDirectionZ.m_fWantedValue * m_cFinalWindDirectionZ.m_fWantedValue;
    if (fSum != 0.0f)
    {
        fSum = sqrt(fSum);
        m_cFinalWindDirectionX.m_fWantedValue /= fSum;
        m_cFinalWindDirectionY.m_fWantedValue /= fSum;
        m_cFinalWindDirectionZ.m_fWantedValue /= fSum;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::Random

int CSpeedWindDef::Random(void)
{
    if (m_bUseRandomBuffer)
    {
        st_assert(!m_vRandomBuffer.empty( ));
        return m_vRandomBuffer[m_nRandomIndex++ % c_nRandomBufferSize];
    }
    else
    {
        return rand( );
    }
}


/////////////////////////////////////////////////////////////////////////////
// CSpeedWindDef::SetUseRandomBuffer

void CSpeedWindDef::SetUseRandomBuffer(bool bUse, unsigned int uiSeed)
{
    m_bUseRandomBuffer = bUse;

    if (m_bUseRandomBuffer && uiSeed != 0)
    {
        m_vRandomBuffer.resize(c_nRandomBufferSize);

        // non-zero seed = reset
        srand(uiSeed);
        for (int i = 0; i < c_nRandomBufferSize; ++i)
            m_vRandomBuffer[i] = rand( );
    }
}

