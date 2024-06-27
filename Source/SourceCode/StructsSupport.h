///////////////////////////////////////////////////////////////////////  
//  StructsSupport.h
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
//      http://www.idvinc.com

#pragma once
#include "LibVector_Source\IdvVector.h"
#include "LibFilename_Source\IdvFilename.h"

//
//  forward references
//

class CBranch;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvBranchVertex declaration
//
//  Used during tree branch computation, SIdvBranchVertex are used as the
//  branch vertexes place holders before skinning.

struct SIdvBranchVertex
{
        stVec3              m_cDirection;           // direction the branch is growing in
        stVec3              m_cPos;                 // (x,y,z) position of this branch vertex
        float               m_fRadius;              // the computed radius at this point along the branch
        stRotTransform      m_cTrans;               // rotation matrix that governs the direction vector
        float               m_fRunningLength;       // running length of the branch
        float               m_fWindWeight1;         // primary wind weight of all vertices in the corresponding cross-section
        float               m_fWindWeight2;         // primary wind weight of all vertices in the corresponding cross-section
        int                 m_nForkCount;           // counts the occurences of fork branches at this vertex
        int                 m_nCrossSegments;
};

DefineAllocator(st_allocator_branchvertex);
typedef std::vector<SIdvBranchVertex, st_allocator_branchvertex<SIdvBranchVertex> > st_vector_branchvertex;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvLeafTexture declaration
//
//  Used to store attributes of each individual leaf texture

struct SIdvLeafTexture
{
        SIdvLeafTexture( ) :
            m_bBlossom(false),
            m_cColor(0.8f, 0.8f, 0.8f),
            m_fColorVariance(0.2f),
            m_cOrigin(0.5f, 1.0f),
            m_cSize(0.12f, 0.12f),
            m_cSizeUsed(10.0f, 10.0f),
            m_bUseMeshes(false),
            m_nMeshLeafIndex(0),
            m_fHang(0.0f),
            m_fRotate(0.0f)
        {
        }
        ~SIdvLeafTexture( )
        {
            // do nothing
        }

        bool                m_bBlossom;             // flag that specifies if this texture can be used as a blossom texture
        stVec3              m_cColor;               // base color for the leaf texture  
        float               m_fColorVariance;       // color variance allowed for the leaf texture
        st_string           m_strFilename;          // original texture map filename specified in SpeedTreeCAD
        stVec3              m_cOrigin;              // two-dimensional (s, t) origin of the leaf texture map
        stVec3              m_cSize;                // dimension of the texture, [0] = width, [1] = height
        stVec3              m_cSizeUsed;            // adjusted size, based on user-specified leaf size

        bool                m_bUseMeshes;           // flag that specifies if this leaf should be drawn as a mesh
        int                 m_nMeshLeafIndex;       // index to which mesh this leaf uses
        float               m_fHang;                // amount that the normals of this leaf hang down (useful for placing meshes or affecting lighting)
        float               m_fRotate;              // amount that the normals of this leaf are rotated
};

DefineAllocator(st_allocator_leaftexture);
typedef std::vector<SIdvLeafTexture, st_allocator_leaftexture<SIdvLeafTexture> > st_vector_leaftexture;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvMeshVertex

struct SIdvMeshVertex
{
    stVec3  m_cPos;
    stVec3  m_cNormal;
    stVec3  m_cBinormal;
    stVec3  m_cTangent;
    float   m_afUV[2];
};

DefineAllocator(st_allocator_meshvertex);
typedef std::vector<SIdvMeshVertex, st_allocator_meshvertex<SIdvMeshVertex> > st_vector_meshvertex;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvMeshInfo
//
//  Used to store info on leaf meshes

struct SIdvMeshInfo
{
        st_string               m_strName;
        st_vector_meshvertex    m_vVertices;
        st_vector_int           m_vIndices;
};

DefineAllocator(st_allocator_meshinfo);
typedef std::vector<SIdvMeshInfo*, st_allocator_meshinfo<SIdvMeshInfo*> > st_vector_meshinfo;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvBranch declaration
//
//  This structure encapsulates a child branch

struct SIdvBranch
{
        SIdvBranch( ) :
            m_nPreVertexIndex(0),
            m_fPercent(0.0f),
            m_pBranch(NULL)
        {
        }
        ~SIdvBranch( )
        {
            // do not delete m_pBranch (it wasn't allocated here)
            m_pBranch = NULL;
        }

        int         m_nPreVertexIndex;              // the branch vertex of the parent immediately before when this child was spawned
        float       m_fPercent;                     // percent between the two parent's vertexes where this child was created
        CBranch*    m_pBranch;                      // branch this SIdvBranch instance encapsulates
};


///////////////////////////////////////////////////////////////////////  
//  struct SIdvBranchFlare declaration
//
//  This structure encapsulates a child branch

struct SIdvBranchFlare
{
        SIdvBranchFlare( ) :
            m_fAngle(0.0f),
            m_fRadialInfluence(0.0f),
            m_fRadialExponent(1.0f),
            m_fLengthInfluence(0.0f),
            m_fLengthExponent(1.0f),
            m_fDistance(0.0f)
        {
        }
        ~SIdvBranchFlare( )
        {
            // do nothing
        }

        float       m_fAngle;                       // angle of the center of the flare
        float       m_fRadialInfluence;             // angle within which branch vertices are affected by this flare
        float       m_fRadialExponent;              // controls sharpness of flare points 
        float       m_fLengthInfluence;             // distance along the branch that vertices are affected by this flare
        float       m_fLengthExponent;              // controls lengthwise profile of flares
        float       m_fDistance;                    // distance of flare at point of maximum influence

        float       Distance(float fInputAngle, float fInputProgress) const
        {
            float fDistance = 0.0f;
            // compute angle difference (accounting for 360/0 crossing)
            float fTestAngle = m_fAngle;
            float fAngleDifference = fabsf(fInputAngle - fTestAngle);
            if (fAngleDifference > c_fPi)
            {
                if (fInputAngle < fTestAngle)
                    fInputAngle += c_fTwoPi;
                else
                    fTestAngle += c_fTwoPi;
            }
            fAngleDifference = fabsf(fInputAngle - fTestAngle);

            if (fAngleDifference < m_fRadialInfluence)
            {
                float fProgressDifference = m_fLengthInfluence - fInputProgress;
                if (fProgressDifference > 0.0f)
                {
                    float fRadialContribution = 1.0f - (fAngleDifference / m_fRadialInfluence);
                    fRadialContribution = float(pow(double(fRadialContribution), double(m_fRadialExponent)));

                    float fVerticalContribution = fProgressDifference / m_fLengthInfluence;
                    fVerticalContribution = float(pow(double(fVerticalContribution), double(m_fLengthExponent)));
                    fDistance = m_fDistance * fRadialContribution * fVerticalContribution;
                }

            }

            return fDistance;
        }
};

DefineAllocator(st_allocator_sbranches);
typedef std::vector<SIdvBranch, st_allocator_sbranches<SIdvBranch> > st_vector_sbranches;

DefineAllocator(st_allocator_branchflare);
typedef std::vector<SIdvBranchFlare, st_allocator_branchflare<SIdvBranchFlare> > st_vector_branchflare;

