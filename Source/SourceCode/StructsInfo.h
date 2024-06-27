///////////////////////////////////////////////////////////////////////  
//  StructsInfo.h
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

#if defined(STRIP_EXCEPTION_HANDLING) && defined(WIN32)
#pragma warning (disable : 4530)
#endif

#include "LibSpline_Source/IdvSpline.h"
#include "LibFilename_Source/IdvFilename.h"
#include "StructsSupport.h"
#include "SpeedTreeRT.h"

//
//  forward references
//

class CTreeFileAccess;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvBranchInfo declaration
//
//  There is one instance of SIdvBranchInfo for each branch level used
//  to create the tree.

struct SIdvBranchInfo
{
        struct STexCoordControls
        {
            enum EClampValues
            {
                LEFT, RIGHT, BOTTOM, TOP, NUM_CLAMP_VALUES
            };

            float   m_fSTile;                           // number of texture repeats around the branch
            float   m_fTTile;                           // number of texture repeats along the length of the branch
            bool    m_bSTileAbsolute;                   // determines if the tiling based on the radius of the branch or absolute
            bool    m_bTTileAbsolute;                   // determines if the tiling based on the length of the branch or absolute
            bool    m_bRandomTCoordOffset;              // start map tiling at random T texture coordinate
            float   m_fTwist;                           // number of times the texture is twisted at the end
            float   m_fSCoordOffset;
            float   m_fTCoordOffset;
            bool    m_bClampSCoord;
            bool    m_bClampTCoord;
            float   m_afClampValues[NUM_CLAMP_VALUES];
            bool    m_bSynch;

            STexCoordControls(void) :
                m_fSTile(1.0f),
                m_fTTile(1.0f),
                m_bSTileAbsolute(true),
                m_bTTileAbsolute(false),
                m_bRandomTCoordOffset(false),
                m_fTwist(0.0f),
                m_fSCoordOffset(0.0f),
                m_fTCoordOffset(0.0f),
                m_bClampSCoord(false),
                m_bClampTCoord(false),
                m_bSynch(true)
            {
                m_afClampValues[LEFT] = m_afClampValues[BOTTOM] = 0.0f;
                m_afClampValues[RIGHT] = m_afClampValues[TOP] = 1.0f;
            }
        };

        SIdvBranchInfo( );
        SIdvBranchInfo(const SIdvBranchInfo& sRight);

static  SIdvBranchInfo*         Parse(CTreeFileAccess& cFile);
        void                    Save(CTreeFileAccess& cFile) const;
        void                    ParseThis(CTreeFileAccess& cFile);
        void                    ParseSupplemental(CTreeFileAccess& cFile);
        void                    SaveSupplemental(CTreeFileAccess& cFile) const;

        // spline class get/sets
        //lint -save -e1536
        stBezierSpline*         GetDisturbance(void)                            { return &m_cDisturbance; }
        stBezierSpline*         GetGravity(void)                                { return &m_cGravity; }
        stBezierSpline*         GetFlexibility(void)                            { return &m_cFlexibility; }
        stBezierSpline*         GetFlexibilityScale(void)                       { return &m_cFlexibilityScale; }
        stBezierSpline*         GetLength(void)                                 { return &m_cLength; }
        stBezierSpline*         GetRadius(void)                                 { return &m_cRadius; }
        stBezierSpline*         GetRadiusScale(void)                            { return &m_cRadiusScale; }
        stBezierSpline*         GetStartAngle(void)                             { return &m_cStartAngle; }
        stBezierSpline*         GetAngleProfile(void)                           { return &m_cAngleProfile; }
        stBezierSpline*         GetCrossSectionProfile(void)                    { return &m_cCrossSectionProfile; }
        stBezierSpline*         GetLightSeamProfile(void)                       { return &m_cLightSeamProfile; }
        stBezierSpline*         GetRoughnessProfile(void)                       { return &m_cRoughnessProfile; }
        stBezierSpline*         GetFrequencyProfile(void)                       { return &m_cFrequencyProfile; }
        stBezierSpline*         GetLightSeamBias(void)                          { return &m_cLightSeamBias; }
        stBezierSpline*         GetGnarlProfile(void)                           { return &m_cGnarlProfile; }
        //lint -restore

        // resolution-specifiers
        int                     m_nCrossSectionSegments;            // number of circular/radial points used in each branch
        int                     m_nSegments;                        // number of points used along the length of the branch

        // branch generation
        float                   m_fFirstBranch;                     // percentage along parent's length where children branch start being created
        float                   m_fLastBranch;                      // percentage along parent's length where children branch strop being created
        float                   m_fFrequency;                       // affects quantity of children branches created

        // tiling/texture controls
        STexCoordControls       m_asTexCoordControls[CSpeedTreeRT::TL_SHADOW]; // don't need an array of size NUM_TEX_LAYERS because shadow tex coords are computed differently

        // base flaring controls
        float                   m_fSegmentPackingExponent;          // controls how length segments are distributed along a branch
        int                     m_nNumFlares;                       // number of flares
        float                   m_fFlareBalance;                    // controls spacing between flares
        float                   m_fRadialInfluence;                 // angular measure of each flare's influence
        float                   m_fRadialInfluenceVariance;         // variance of radial influence
        float                   m_fRadialExponent;                  // controls sharpness of flare points
        float                   m_fRadialDistance;                  // distance flare protrudes (percentage of branch radius)
        float                   m_fRadialVariance;                  // variance of radial distance
        float                   m_fLengthDistance;                  // distance up the branch the flare is visible (percentage of branch length)
        float                   m_fLengthVariance;                  // variance of length distance
        float                   m_fLengthExponent;                  // controls lengthwise profile of flare segments

        // light seam reduction
        float                   m_fLightSeamStartBias;              // controls how much the normals are biased to the standard value near the start of a branch
        float                   m_fLightSeamEndBias;                // controls how much the normals are biased to the standard value near the end of a branch

        // roughness
        float                   m_fRoughness;                       // controls the amplitude of the roughness computation
        float                   m_fRoughnessVerticalFrequency;      // controls the frequency of the roughness pattern
        float                   m_fRoughnessHorizontalFrequency;    // controls the frequency of the roughness pattern
        float                   m_fRandomRoughness;                 // controls the frequency of the roughness pattern

        // segment variance
        float                   m_fMinLengthPercent;                // specifies what percentage of the length segment number is applied to small branches
        float                   m_fMinCrossSectionPercent;          // specifies what percentage of the cross section segment number is applied to small branches

        // depth based pruning
        float                   m_fPruningPercent;                  // set the cutoff distance for branch pruning
        unsigned int            m_uiPruningDepth;                   // specifies how many levels back to check for pruning

        // forking
        bool                    m_bForkingEnabled;                  // enables forking
        float                   m_fForkBias;                        // high values = favor segment breaks, low values = random
        float                   m_fForkAngle;                       // minimum angle at which forking can occur
        int                     m_nForkLimit;                       // number of fork branches that can occur at a vertex

        // gnarl
        float                   m_fGnarl;                           // amount of uniform twisting of the branches
        bool                    m_bGnarlUnison;                     // true = all branches gnarl in the same direction

private:
        stBezierSpline          m_cDisturbance;                     // used to apply a random angle variation at each segment along the length of the branch
        stBezierSpline          m_cGravity;                         // controls the max gravitational influence possible along the length of the branch
        stBezierSpline          m_cFlexibility;                     // controls the max flexibility (wind influence) of the branch's children along the length of the branch
        stBezierSpline          m_cFlexibilityScale;                // controls how the single flexibility value of a branch is scaled along the branch length
        stBezierSpline          m_cLength;                          // the length varies according to its place along its parent
        stBezierSpline          m_cRadius;                          // the radius varies according to its place along its parent
        stBezierSpline          m_cRadiusScale;                     // controls how the single radius value of a branch is scaled along the branch length
        stBezierSpline          m_cStartAngle;                      // controls the angle at which new branches grow relative to the parent is growing
        stBezierSpline          m_cAngleProfile;                    // controls (along w/ m_pGravity) how the branch grows from base to tip
        stBezierSpline          m_cCrossSectionProfile;             // controls cross sectional resolution from base to tip
        stBezierSpline          m_cLightSeamProfile;                // controls light seam reduction along the length of the branch
        stBezierSpline          m_cRoughnessProfile;                // controls roughness along the length of the branch
        stBezierSpline          m_cFrequencyProfile;                // controls distribution of child branches
        stBezierSpline          m_cLightSeamBias;                   // controls distribution of child branches
        stBezierSpline          m_cGnarlProfile;                    // controls amount of gnarl along the branch
};

DefineAllocator(st_allocator_const_branchinfo);
typedef std::vector<const SIdvBranchInfo*, st_allocator_const_branchinfo<const SIdvBranchInfo*> > st_vector_const_branchinfo;

DefineAllocator(st_allocator_branchinfo);
typedef std::vector<SIdvBranchInfo*, st_allocator_branchinfo<SIdvBranchInfo*> > st_vector_branchinfo;


///////////////////////////////////////////////////////////////////////  
//  struct SIdvLeafInfo declaration
//
//  There is one instance of SIdvLeafInfo per tree that encapsulates most of
//  of the leaf information.

struct SIdvLeafInfo
{
        enum ECollisionType
        {
            NONE, BRANCH, TREE
        };

        SIdvLeafInfo( );
        ~SIdvLeafInfo( );

        void                    InitTables(int nNumTextures);
        void                    Parse(CTreeFileAccess& cFile);
        void                    Save(CTreeFileAccess& cFile) const;


        //  color
        bool                    m_bDimming;                 // determines if the leaves get darker as the approach the tree center
        float                   m_fDimmingScalar;           // if the leaves are to be dimmed, this determines how much darker

        //  general leaves
        ECollisionType          m_eCollisionType;           // when leaves are generated, the collision type determines which algorithm is used
        st_vector_leaftexture   m_vLeafTextures;            // a list of the different leaf textures used
        float                   m_fSpacingTolerance;        // determines how close together leaves can be
        float                   m_fPlacementDistance;       // how far along a branch's length leaves can be created
        int                     m_nPlacementLevel;          // controls the branch level to which the placement distance parameter applies

        //  blossom
        float                   m_fBlossomDistance;         // how far along a branch's length the blossoms start to occur
        float                   m_fBlossomWeighting;        // determines the percentage of leaves that become blossoms that are eligible
        int                     m_nBlossomLevel;            // how far back up the tree to look for blossom determination

        //  table-related
        int                     m_nNumRockingGroups;        // billboard leaves are in groups that "rock" together - this controls how many groups
        int                     m_nNumLeafLodLevels;        // controls how many discrete number of leaf LOD levels
        int                     m_nNumTextures;             // the number of unique leaf textures used to create the tree
        float**                 m_pLeafVertexes;            // [lod][tex_i * (m_nNumRockingGroups * 20) + angle_i * 20 + corner_i]
        float*                  m_pLeafTexCoords;           // [texture * 8 points to beginning of 8-entry floats]
        float*                  m_pTimeOffsets;             // each rocking group has a different time offset so they don't appear synchronized

        //  leaf smoothing
        bool                    m_bSmoothingEnabled;        // enables leaf normal smoothing
        float                   m_fSmoothingAngle;          // angle (degrees) that is used when spreading out the normals

        //  dimming depth
        int                     m_nDimmingDepth;            // how far back up the tree to look for dimming calculations
};


///////////////////////////////////////////////////////////////////////  
//  struct SIdvWindInfo declaration
//
//  There is one instance of SIdvWindInfo per tree.

struct SIdvWindInfo
{
        enum EOscillation
        {
            MIN, MAX, FREQUENCY
        };

        SIdvWindInfo( );

        void    Parse(CTreeFileAccess& cFile);
        void    Save(CTreeFileAccess& cFile) const;

        stVec3  m_cLeafFactors;                             // [0] = amount of leaf oscillation, [1] = frequency of leaf oscillation
        stVec3  m_cLeafOscillation;                         // [0] = -leaf throw, [1] = leaf throw, [2] = leaf oscillation
        float   m_fStrength;                                // strength of wind from 0.0 to 1.0
};


///////////////////////////////////////////////////////////////////////  
//  struct SIdvTreeInfo declaration
//
//  There is one instance of SIdvTreeInfo per tree.

struct SIdvTreeInfo
{
        SIdvTreeInfo( );

        st_string   m_strBranchTextureFilename; // filename of the branch texture map

        float       m_fFar;                     // far LOD distance
        float       m_fNear;                    // near LOD distance

        int         m_nSeed;                    // random seed used to compute tree
        float       m_fSize;                    // global size scalar for tree
        float       m_fSizeVariance;            // amount of desired size variance
        int         m_nFlareSeed;               // seed used to compute flares
};


///////////////////////////////////////////////////////////////////////  
//  SRootSupportInfo

struct SRootSupportInfo
{
        SRootSupportInfo( );

        void            Parse(CTreeFileAccess& cFile);
        void            Save(CTreeFileAccess& cFile) const;

        SIdvBranchInfo  m_sRoots;
        int             m_nLevel;
        float           m_fFirst;
        float           m_fLast;
        float           m_fFrequency;
};

