///////////////////////////////////////////////////////////////////////
//  SpeedTreeRT.cpp
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

#define SPEEDTREE_SOURCE_FILE // used to extern variable to other files in Memory.h
#include "Debug.h"
#include "LibGlobals_Source/IdvGlobals.h"
#include "SpeedTreeRT.h"
#include "IndexedGeometry.h"
#include "Instances.h"
#include "TreeEngine.h"
#include "WindEngine.h"
#include "SimpleBillboard.h"
#include "LightingEngine.h"
#include "LeafGeometry.h"
#include "UpVector.h"
#include "ProjectedShadow.h"
#include "EvalKey_Source/SpeedTreeKeyUtility.h"
#include "Endian.h"
#include "MapBank.h"
using namespace std;


// static/global variables
st_vector_speedtree_p* g_pUniqueTrees = NULL;
bool CSpeedTreeRT::m_bDropToBillboard = false;
float CSpeedTreeRT::m_fCameraAzimuth = 0.0f;
float CSpeedTreeRT::m_fCameraPitch = 0.0f;
bool CSpeedTreeRT::m_bTextureFlip = false;
unsigned int CSpeedTreeRT::m_uiAllRefCount = 0;
float CSpeedTreeRT::m_fHorizontalFadeStartAngle = 30.0f;
float CSpeedTreeRT::m_fHorizontalFadeEndAngle = 60.0f;
float CSpeedTreeRT::m_fHorizontalFadeValue = 84.0f;
CSpeedTreeRT::EMemoryPreference CSpeedTreeRT::m_eMemoryPreference = CSpeedTreeRT::MP_FAVOR_SMALLER_FOOTPRINT;

// shared by functions in this file only
static std::string g_strError;// = "No error";
static std::string g_strKey;

// constants
const float c_fClear = 255.0f;

// enumerations
enum ESpeedTreeInternalEvent
{
    STIE_CLIENT_CHANGED_WIND,
    STIE_CLIENT_CHANGED_CAMERA,
    STIE_UNKNOWN
};

enum ESpeedTreeSizes
{
    STS_MIN_BOX     = 0,    // (x, y, z) min of orthographic bounding box
    STS_MAX_BOX     = 3,    // (x, y, z) max of orthographic bounding box
    STS_BB_SIZE     = 6,    // width of simple billboard (last LOD)
    STS_COUNT       = 7
};


///////////////////////////////////////////////////////////////////////
//  SEmbeddedTexCoords definition
//
//  Used to store leaf map texture coordinates from a composite leaf map
//  as well as billboard texture coordinates.

struct SEmbeddedTexCoords
{
        SEmbeddedTexCoords( ) :
            m_nNumLeafMaps(0),
            m_pLeafTexCoords(NULL),
            m_nNumBillboards(0),
            m_pBillboardTexCoords(NULL),
            m_nNumFrondMaps(0),
            m_pFrondTexCoords(NULL)
        {
            m_afShadowTexCoords[0] = 1.0f;
            m_afShadowTexCoords[1] = 1.0f;

            m_afShadowTexCoords[2] = 0.0f;
            m_afShadowTexCoords[3] = 1.0f;

            m_afShadowTexCoords[4] = 0.0f;
            m_afShadowTexCoords[5] = 0.0f;

            m_afShadowTexCoords[6] = 1.0f;
            m_afShadowTexCoords[7] = 0.0f;
        }

        ~SEmbeddedTexCoords( )
        {
            st_delete_array<float>(m_pLeafTexCoords, "SEmbeddedTexCoords, m_pLeafTexCoords");
            st_delete_array<float>(m_pBillboardTexCoords, "SEmbeddedTexCoords, m_pBillboardTexCoords");
            st_delete_array<float>(m_pFrondTexCoords, "SEmbeddedTexCoords, m_pFrondTexCoords");
        }

        const SEmbeddedTexCoords& operator=(const SEmbeddedTexCoords& sRight)
        {
            if (&sRight != this)
            {
                // copy leaf texture coordinates
                if (sRight.m_pLeafTexCoords)
                {
                    m_nNumLeafMaps = sRight.m_nNumLeafMaps;
                    m_pLeafTexCoords = st_new_array<float>(m_nNumLeafMaps * 8, "SEmbeddedTexCoords operator=, m_pLeafTexCoords");
                    memcpy(m_pLeafTexCoords, sRight.m_pLeafTexCoords, m_nNumLeafMaps * 8 * sizeof(float));
                }
                // copy billboard texture coordinates
                if (sRight.m_pBillboardTexCoords)
                {
                    m_nNumBillboards = sRight.m_nNumBillboards;
                    m_pBillboardTexCoords = st_new_array<float>(m_nNumBillboards * 8, "SEmbeddedTexCoords operator=, m_pBillboardTexCoords");
                    memcpy(m_pBillboardTexCoords, sRight.m_pBillboardTexCoords, m_nNumBillboards * 8 * sizeof(float));
                }
                // copy frond texture coordinates
                if (sRight.m_pFrondTexCoords)
                {
                    m_nNumFrondMaps = sRight.m_nNumFrondMaps;
                    m_pFrondTexCoords = st_new_array<float>(m_nNumFrondMaps * 8, "SEmbeddedTexCoords operator=, m_pFrondTexCoords");
                    memcpy(m_pFrondTexCoords, sRight.m_pFrondTexCoords, m_nNumFrondMaps * 8 * sizeof(float));
                }

                m_strCompositeFilename = sRight.m_strCompositeFilename;
                memcpy(m_afShadowTexCoords, sRight.m_afShadowTexCoords, 8 * sizeof(float));
            }

            return *this;
        }

        int         m_nNumLeafMaps;             // number of leaf texture coordinate sets
        float*      m_pLeafTexCoords;           // array of leaf texture coordinates
        int         m_nNumBillboards;           // number of billboard texture coordinate sets
        float*      m_pBillboardTexCoords;      // array of billboard texture coordinates

        int         m_nNumFrondMaps;            // number of separate frond texture maps
        float*      m_pFrondTexCoords;          // array of frond texture coordinates

        st_string   m_strCompositeFilename;     // filename of composite map holding leaf, frond, and billboard textures
        float       m_afShadowTexCoords[8];     // self-shadow texcoords (may or may not index a composite map)
};


///////////////////////////////////////////////////////////////////////
//  SShape definition
//
//  Defines the shapes used for collision objects.

struct SShape
{
        SShape( ) :
            m_eType(CSpeedTreeRT::CO_SPHERE)
        {
            m_afPosition[0] = m_afPosition[1] = m_afPosition[2] = 0.0f;
            m_afDimensions[0] = m_afDimensions[1] = m_afDimensions[2] = 0.0f;
            m_afRotation[0] = m_afRotation[1] = m_afRotation[2] = 0.0f;
        }
        ~SShape( )
        {
            // do nothing
        }

        CSpeedTreeRT::ECollisionObjectType  m_eType;            // CO_SPHERE, CO_CYLINDER, or CO_BOX
        float                               m_afPosition[3];    // center of shape
        float                               m_afDimensions[3];  // dimensions of shape (each type interprets differently)
        float                               m_afRotation[3];    // Euler angles
};

DefineAllocator(st_allocator_shape);
typedef std::vector<SShape, st_allocator_shape<SShape> > st_vector_shape;


///////////////////////////////////////////////////////////////////////
//  SCollisionObjects definition
//
//  A wrapper used to hide the STL vector type from the application.

struct SCollisionObjects
{
        void            TransformAll(stTransform& cTransform)
        {
            for (st_vector_shape::iterator i = m_vObjects.begin( ); i != m_vObjects.end( ); ++i)
            {
                // transform base point
                stVec3 cPos(i->m_afPosition[0], i->m_afPosition[1], i->m_afPosition[2]);
                cPos = cPos * cTransform;

                i->m_afPosition[0] = cPos[0];
                i->m_afPosition[1] = cPos[1];
                i->m_afPosition[2] = cPos[2];

                // transform the dimensions
                if (i->m_eType == CSpeedTreeRT::CO_SPHERE)
                {
                    // the sphere is always meant to be perfectly round,
                    // so we've chosen to scale the radius arbitrarily by
                    // the x-axis scale factor
                    i->m_afDimensions[0] *= cTransform.m_afData[0][0];
                }
                else if (i->m_eType == CSpeedTreeRT::CO_CAPSULE)
                {
                    // the cylinder collision object was not meant to
                    // lie on anything but the up axis, so an arbitrary
                    // transform can wreak some havoc.
                    i->m_afDimensions[0] *= cTransform.m_afData[0][0];
                    i->m_afDimensions[1] *= cTransform.m_afData[0][0];
                }
                else if (i->m_eType == CSpeedTreeRT::CO_BOX)
                {
                    i->m_afDimensions[0] *= cTransform.m_afData[0][0];
                    i->m_afDimensions[1] *= cTransform.m_afData[1][1];
                    i->m_afDimensions[2] *= cTransform.m_afData[2][2];
                }
            }
        }

        st_vector_shape  m_vObjects;         // collection of all collision objects in tree
};


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry constructor definition

CSpeedTreeRT::SGeometry::SGeometry( ) :
    m_nNumLeafLods(0),
    m_pLeaves(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry destructor definition

CSpeedTreeRT::SGeometry::~SGeometry( )
{
    m_pLeaves = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SIndexed constructor definition

CSpeedTreeRT::SGeometry::SIndexed::SIndexed( ) :
    m_nNumLods(0),
    m_pNumStrips(NULL),
    m_pStripLengths(NULL),
    m_pStrips(NULL),
    m_nNumVertices(0),
    m_pColors(NULL),
    m_pNormals(NULL),
    m_pBinormals(NULL),
    m_pTangents(NULL),
    m_pCoords(NULL),
    m_pLodFadeHints(NULL),
    m_fLodFadeDistance(0.0f)
{
    m_pWindWeights[0] = m_pWindWeights[1] = NULL;
    m_pWindMatrixIndices[0] = m_pWindMatrixIndices[1] = NULL;
    for (int i = 0; i < int(TL_NUM_TEX_LAYERS); ++i)
        m_pTexCoords[i] = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SIndexed destructor definition

CSpeedTreeRT::SGeometry::SIndexed::~SIndexed( )
{
    // these tables are deleted in ~CIndexedGeometry( )
    m_pColors = NULL;
    m_pNormals = NULL;
    m_pBinormals = NULL;
    m_pTangents = NULL;
    m_pCoords = NULL;
    m_pWindWeights[0] = m_pWindWeights[1] = NULL;
    m_pWindMatrixIndices[0] = m_pWindMatrixIndices[1] = NULL;
    m_pStripLengths = NULL;
    m_pStrips = NULL;
    for (int i = 0; i < int(TL_NUM_TEX_LAYERS); ++i)
        m_pTexCoords[i] = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf constructor definition

CSpeedTreeRT::SGeometry::SLeaf::SLeaf( ) :
    m_nDiscreteLodLevel(-1),
    m_nNumLeaves(0),
    m_fLeafRockScalar(1.0f),
    m_fLeafRustleScalar(1.0f),
    m_pLeafMapIndices(NULL),
    m_pLeafCardIndices(NULL),
    m_pCenterCoords(NULL),
    m_pColors(NULL),
    m_pDimming(NULL),
    m_pNormals(NULL),
    m_pBinormals(NULL),
    m_pTangents(NULL),
    m_pCards(NULL)
{
    m_pWindWeights[0] = m_pWindWeights[1] = NULL;
    m_pWindMatrixIndices[0] = m_pWindMatrixIndices[1] = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf destructor definition

CSpeedTreeRT::SGeometry::SLeaf::~SLeaf( )
{
    // this data is deleted in CLeafGeometry::SLodGeometry::~SLodGeometry( )
    m_pLeafMapIndices = NULL;
    m_pLeafCardIndices = NULL;
    m_pCenterCoords = NULL;
    m_pColors = NULL;
    m_pDimming = NULL;
    m_pNormals = NULL;
    m_pBinormals = NULL;
    m_pTangents = NULL;
    m_pWindWeights[0] = m_pWindWeights[1] = NULL;
    m_pWindMatrixIndices[0] = m_pWindMatrixIndices[1] = NULL;
    m_pCards = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf::SMesh constructor definition

CSpeedTreeRT::SGeometry::SLeaf::SMesh::SMesh( ) :
    m_nNumVertices(0),
    m_pCoords(NULL),
    m_pTexCoords(NULL),
    m_pNormals(NULL),
    m_pBinormals(NULL), 
    m_pTangents(NULL),
    m_nNumIndices(NULL),
    m_pIndices(NULL)
{

}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf::SMesh destructor definition

CSpeedTreeRT::SGeometry::SLeaf::SMesh::~SMesh( )
{
    m_pCoords = NULL;
    m_pTexCoords = NULL;
    m_pNormals = NULL;
    m_pBinormals = NULL;
    m_pTangents = NULL;
    m_pIndices = NULL;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf::SCard constructor definition

CSpeedTreeRT::SGeometry::SLeaf::SCard::SCard( ) :
    m_fWidth(-1.0f),
    m_fHeight(-1.0f),
    m_pTexCoords(NULL),
    m_pCoords(NULL),
    m_pMesh(NULL)
{
    m_afPivotPoint[0] = m_afPivotPoint[1] = -1.0f;
    m_afAngleOffsets[0] = m_afAngleOffsets[1] = -1.0f;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SLeaf::~SCard constructor definition

CSpeedTreeRT::SGeometry::SLeaf::SCard::~SCard( )
{
    m_pTexCoords = NULL;
    m_pCoords = NULL;
    m_pMesh = NULL;
}

                    
///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::S360Billboard constructor definition

CSpeedTreeRT::SGeometry::S360Billboard::S360Billboard( ) :
    m_fWidth(0.0f),
    m_fHeight(0.0f),
    m_nNumImages(-1),
    m_pTexCoordTable(NULL),
    m_pCoords(NULL),
    m_pNormals(NULL),
    m_pBinormals(NULL),
    m_pTangents(NULL),
    m_fTransitionPercent(0.0f)
{
    m_pTexCoords[0] = m_pTexCoords[1] = NULL;
    m_afAlphaTestValues[0] = m_afAlphaTestValues[1] = c_fClear;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SGeometry::SHorzBillboard constructor definition

CSpeedTreeRT::SGeometry::SHorzBillboard::SHorzBillboard( ) :
    m_pCoords(NULL),
    m_pTexCoords(NULL),
    m_fAlphaTestValue(c_fClear)
{
    // normals, binormals, and tangents are the same for each vertex, but
    // may not be in the future
    const stVec3 c_cNormal(0.0f, 0.0f, 1.0f);
    const stVec3 c_cTangent(1.0f, 0.0f, 0.0f);
    const stVec3 c_cBinormal(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        Assign3d(m_afNormals[i], c_cNormal);
        Assign3d(m_afTangents[i], c_cTangent);
        Assign3d(m_afBinormals[i], c_cBinormal);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SLodValues constructor definition

CSpeedTreeRT::SLodValues::SLodValues( ) :
    m_nBranchActiveLod(-1),
    m_fBranchAlphaTestValue(-1.0f),
    m_nFrondActiveLod(-1),
    m_fFrondAlphaTestValue(-1.0f),
    m_fBillboardFadeOut(0.0f)
{
    m_anLeafActiveLods[0] = m_anLeafActiveLods[1] = -1;
    m_afLeafAlphaTestValues[0] = m_afLeafAlphaTestValues[1] = -1.0f;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SMapBank constructor definition

CSpeedTreeRT::SMapBank::SMapBank( ) :
    m_pBranchMaps(NULL),
    m_uiNumLeafMaps(0),
    m_pLeafMaps(NULL),
    m_uiNumFrondMaps(0),
    m_pFrondMaps(NULL),
    m_pCompositeMaps(NULL),
    m_pBillboardMaps(NULL),
    m_pSelfShadowMap(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SMapBank destructor definition

CSpeedTreeRT::SMapBank::~SMapBank( )
{
    st_delete_array<const char*>(m_pBranchMaps, "CSpeedTreeRT::SMapBank::~SMapBank, m_pBranchMaps");

    unsigned int i = 0;
    if (m_pLeafMaps)
    {
        for (i = 0; i < m_uiNumLeafMaps; ++i)
            st_delete_array<const char*>(m_pLeafMaps[i], "CSpeedTreeRT::SMapBank::~SMapBank, m_pLeafMaps[ ]");
        st_delete_array<const char**>(m_pLeafMaps, "CSpeedTreeRT::SMapBank::~SMapBank, m_pLeafMaps");
    }

    if (m_pFrondMaps)
    {
        for (i = 0; i < m_uiNumFrondMaps; ++i)
            st_delete_array<const char*>(m_pFrondMaps[i], "CSpeedTreeRT::SMapBank::~SMapBank, m_pFrondMaps[ ]");
        st_delete_array<const char**>(m_pFrondMaps, "CSpeedTreeRT::SMapBank::~SMapBank, m_pFrondMaps");
    }

    st_delete_array<const char*>(m_pCompositeMaps, "CSpeedTreeRT::SMapBank::~SMapBank, m_pCompositeMaps");
    st_delete_array<const char*>(m_pBillboardMaps, "CSpeedTreeRT::SMapBank::~SMapBank, m_pBillboardMaps");

    m_pBranchMaps = NULL;
    m_pLeafMaps = NULL;
    m_pFrondMaps = NULL;
    m_pCompositeMaps = NULL;
    m_pBillboardMaps = NULL;
    m_pSelfShadowMap = NULL;
}


///////////////////////////////////////////////////////////////////////
//  SLightShaderParams definition

CSpeedTreeRT::SLightShaderParams::SLightShaderParams( ) :
    m_fGlobalLightScalar(1.0f),
    m_fBranchLightScalar(1.0f),
    m_fFrondLightScalar(1.0f),
    m_fLeafLightScalar(1.0f),
    m_fAmbientScalar(0.5f),
    m_fBillboardDarkSideLightScalar(1.0f),
    m_fBillboardBrightSideLightScalar(1.0f),
    m_fBillboardAmbientScalar(0.5f)
{
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::CSpeedTreeRT definition
//
//  This is the default constructor exposed to the application.

CSpeedTreeRT::CSpeedTreeRT( ) :
    m_pEngine(NULL),
    m_pBranchGeometry(NULL),
    m_pLeafGeometry(NULL),
    m_pLeafLods(NULL),
    m_pLightingEngine(NULL),
    m_pWindEngine(NULL),
    m_pSimpleBillboard(NULL),
    m_eLeafLodMethod(LOD_SMOOTH),
    m_fLeafLodTransitionRadius(0.07f),
    m_fLeafLodCurveExponent(0.7f),
    m_fLeafSizeIncreaseFactor(0.5f),
    m_fLeafTransitionFactor(0.0f),
    m_pLeafLodSizeFactors(NULL),
    m_pInstanceRefCount(NULL),
    m_pInstanceData(NULL),
    m_nFrondLevel(-1),
    m_pTreeSizes(NULL),
    m_fTargetAlphaValue(84.0f),
    m_bTreeComputed(false),
    m_nBranchWindLevel(-1),
    m_pEmbeddedTexCoords(NULL),
    m_pProjectedShadow(NULL),
    m_pCollisionObjects(NULL),
    m_pFrondEngine(NULL),
    m_pFrondGeometry(NULL),
    m_pUserData(NULL),
    m_b360Billboard(false),
    m_bHorizontalBillboard(false),
    m_fBranchLightScalar(1.0f),
    m_fFrondLightScalar(1.0f),
    m_fLeafLightScalar(1.0f),
    m_fGlobalLightScalar(1.0f),
    m_fAmbientScalar(0.5f),
    m_fBillboardDarkSideLightScalar(1.0f),
    m_fBillboardBrightSideLightScalar(1.0f),
    m_fBillboardAmbientScalar(0.5f),
    m_nNumBranchLodLevels(0),
    m_nNumFrondLodLevels(0),
    m_nNumLeafLodLevels(0),
    m_fTransitionWidth(0.1f),
    m_fCrestWidth(0.1f),
    m_fCrestWidthBB(0.1f),
    m_fCycleLength(0.1f),
    m_fCycleLengthBB(0.1f),
    m_fBranchLodFadeDistance(0.0f),
    m_fFrondLodFadeDistance(0.0f),
    m_bApplyFadingToExtrusions(false),
    m_fBillboardTransition(0.0f),
    m_pMapBank(NULL)
{
    try
    {
        m_pWindEngine = new ("CSpeedTreeRT::CSpeedTreeRT, m_pWindEngine") CWindEngine;
        m_pBranchGeometry = new ("CSpeedTreeRT::CSpeedTreeRT, m_pBranchGeometry") CIndexedGeometry(m_pWindEngine);
        m_pEngine = new ("CSpeedTreeRT::CSpeedTreeRT, m_pEngine") CTreeEngine(m_pBranchGeometry);
        m_pLightingEngine = new ("CSpeedTreeRT::CSpeedTreeRT, m_pLightingEngine") CLightingEngine;
        m_pLeafGeometry = new ("CSpeedTreeRT::CSpeedTreeRT, m_pLeafGeometry") CLeafGeometry(m_pWindEngine);
        m_pSimpleBillboard = new ("CSpeedTreeRT::CSpeedTreeRT, m_pSimpleBillboard") CSimpleBillboard;
        m_pTreeSizes = st_new_array<float>(STS_COUNT, "CSpeedTreeRT::CSpeedTreeRT, m_pTreeSizes");
        m_pInstanceRefCount = new ("CSpeedTreeRT::CSpeedTreeRT, m_pInstancesRefCount") unsigned int;
        m_pInstanceList = new ("CSpeedTreeRT::CSpeedTreeRT, m_pInstanceList") SInstanceList;
        *m_pInstanceRefCount = 1;

        // fronds
        m_pFrondEngine = new ("CSpeedTreeRT::CSpeedTreeRT, m_pFrondEngine") CFrondEngine;
        m_pFrondGeometry = new ("CSpeedTreeRT::CSpeedTreeRT, m_pFrondGeometry") CIndexedGeometry(m_pWindEngine, true);

        // add to global tree list
        if (!g_pUniqueTrees)
            g_pUniqueTrees = new ("CSpeedTreeRT::CSpeedTreeRT, g_pUniqueTrees") st_vector_speedtree_p;
        g_pUniqueTrees->push_back(this);
        m_uiAllRefCount++;

        CBranch::SetLightingEngine(m_pLightingEngine);

        // setup initial tree extents
        m_pTreeSizes[STS_MIN_BOX + 0] = 0.0f;
        m_pTreeSizes[STS_MIN_BOX + 1] = 0.0f;
        m_pTreeSizes[STS_MIN_BOX + 2] = 0.0f;
        m_pTreeSizes[STS_MAX_BOX + 0] = 1.0f;
        m_pTreeSizes[STS_MAX_BOX + 1] = 1.0f;
        m_pTreeSizes[STS_MAX_BOX + 2] = 1.0f;
        m_pTreeSizes[STS_BB_SIZE] = 1.0f;

        // zero out horizontal coords
        for (int i = 0; i < 12; ++i)
            m_afHorizontalCoords[i] = 0.0f;

        // map bank
        m_pMapBank = new ("CSpeedTreeRT::CSpeedTreeRT, m_pMapBank") CMapBank;

        // tell leaf about tree engine
        m_pLeafGeometry->SetTreeEngine(m_pEngine);
    }

    SpeedTreeCatch("CSpeedTreeRT::CSpeedTreeRT( )")
    SpeedTreeCatchAll("CSpeedTreeRT::CSpeedTreeRT( )");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::CSpeedTreeRT definition
//
//  This is the constructor used for instancing that is hidden from
//  the application.

CSpeedTreeRT::CSpeedTreeRT(const CSpeedTreeRT* pOrig) :
    // general
    m_pEngine(pOrig->m_pEngine),
    m_pBranchGeometry(pOrig->m_pBranchGeometry),
    m_pLeafGeometry(pOrig->m_pLeafGeometry),
    m_pLeafLods(pOrig->m_pLeafLods),
    m_pLightingEngine(pOrig->m_pLightingEngine),
    m_pWindEngine(pOrig->m_pWindEngine),
    m_pSimpleBillboard(pOrig->m_pSimpleBillboard),
    // leaf lod
    m_eLeafLodMethod(pOrig->m_eLeafLodMethod),
    m_fLeafLodTransitionRadius(pOrig->m_fLeafLodTransitionRadius),
    m_fLeafLodCurveExponent(pOrig->m_fLeafLodCurveExponent),
    m_fLeafSizeIncreaseFactor(pOrig->m_fLeafSizeIncreaseFactor),
    m_fLeafTransitionFactor(pOrig->m_fLeafTransitionFactor),
    m_pLeafLodSizeFactors(pOrig->m_pLeafLodSizeFactors),
    // instancing
    m_pInstanceRefCount(pOrig->m_pInstanceRefCount),
    m_pInstanceList(pOrig->m_pInstanceList),
    // other
    m_nFrondLevel(pOrig->m_nFrondLevel),
    m_pTreeSizes(pOrig->m_pTreeSizes),
    m_fTargetAlphaValue(pOrig->m_fTargetAlphaValue),
    m_bTreeComputed(pOrig->m_bTreeComputed),
    m_nBranchWindLevel(pOrig->m_nBranchWindLevel),
    // texcoords
    m_pEmbeddedTexCoords(pOrig->m_pEmbeddedTexCoords),
    // shadow projection
    m_pProjectedShadow(pOrig->m_pProjectedShadow),
    // collision objects
    m_pCollisionObjects(pOrig->m_pCollisionObjects),
    // fronds
    m_pFrondEngine(pOrig->m_pFrondEngine),
    m_pFrondGeometry(pOrig->m_pFrondGeometry),
    // user data
    m_pUserData(pOrig->m_pUserData),
    // horizontal billboard
    m_b360Billboard(pOrig->m_b360Billboard),
    m_bHorizontalBillboard(pOrig->m_bHorizontalBillboard),
    // standard shader support
    m_fBranchLightScalar(pOrig->m_fBranchLightScalar),
    m_fFrondLightScalar(pOrig->m_fFrondLightScalar),
    m_fLeafLightScalar(pOrig->m_fLeafLightScalar),
    m_fGlobalLightScalar(pOrig->m_fGlobalLightScalar),
    m_fAmbientScalar(pOrig->m_fAmbientScalar),
    m_fBillboardDarkSideLightScalar(pOrig->m_fBillboardDarkSideLightScalar),
    m_fBillboardBrightSideLightScalar(pOrig->m_fBillboardBrightSideLightScalar),
    m_fBillboardAmbientScalar(pOrig->m_fBillboardAmbientScalar),
    // lod parameters
    m_nNumBranchLodLevels(pOrig->m_nNumBranchLodLevels),
    m_nNumFrondLodLevels(pOrig->m_nNumFrondLodLevels),
    m_nNumLeafLodLevels(pOrig->m_nNumLeafLodLevels),
    m_fTransitionWidth(pOrig->m_fTransitionWidth),
    m_fCrestWidth(pOrig->m_fCrestWidth),
    m_fCrestWidthBB(pOrig->m_fCrestWidthBB),
    m_fCycleLength(pOrig->m_fCycleLength),
    m_fCycleLengthBB(pOrig->m_fCycleLengthBB),
    // map bank
    m_pMapBank(pOrig->m_pMapBank)
{
    try
    {
        st_assert(pOrig);
        st_assert(pOrig->m_pEngine);
        st_assert(m_pInstanceRefCount);

        // add to parent's instance list
        m_pInstanceList->m_vInstances.push_back(this);

        // copy horizontal billboard coordinates
        memcpy(m_afHorizontalCoords, pOrig->m_afHorizontalCoords, 12 * sizeof(float));

        // reference counting for instances
        (*m_pInstanceRefCount)++;
        m_pInstanceData = new ("CSpeedTreeRT::CSpeedTreeRT, m_pInstanceData") STreeInstanceData;
        m_pInstanceData->m_pParent = pOrig;
        m_pInstanceData->m_cPosition = pOrig->m_pEngine->GetPosition( );
        m_pInstanceData->m_fLodLevel = pOrig->GetLodLevel( );

        m_uiAllRefCount++;
    }

    SpeedTreeCatch("CSpeedTreeRT::CSpeedTreeRT(CSpeedTreeRT*)")
    SpeedTreeCatchAll("CSpeedTreeRT::CSpeedTreeRT(CSpeedTreeRT*)");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::~CSpeedTreeRT definition

CSpeedTreeRT::~CSpeedTreeRT( )
{
    // delete tree from global tree list (if not an instance)
    if (!m_pInstanceData)
    {
        st_vector_speedtree_p::iterator iFind = find(g_pUniqueTrees->begin( ), g_pUniqueTrees->end( ), this);

        // there shouldn't be any reason why this won't be found, but we're being careful
        if (iFind != g_pUniqueTrees->end( ))
            (void) g_pUniqueTrees->erase(iFind);
    }

    // instance reference counting
    st_assert(m_pInstanceRefCount);
    (*m_pInstanceRefCount)--;

    // if this tree was an instance
    if (m_pInstanceData)
    {
        // find the instance in the parent's instance list and delete it
        st_assert(m_pInstanceList);
        st_vector_speedtree_p::iterator iFind = find(m_pInstanceList->m_vInstances.begin( ), m_pInstanceList->m_vInstances.end( ), this);
        st_assert(iFind != m_pInstanceList->m_vInstances.end( ));
        (void) m_pInstanceList->m_vInstances.erase(iFind);

        // delete the data
        st_delete<STreeInstanceData>(m_pInstanceData, "~CSpeedTreeRT, m_pInstanceData");
    }

    // if this is the last class to hold these pointers, they should be deleted
    if (*m_pInstanceRefCount == 0)
    {
        DeleteTransientData( );
        st_delete<CIndexedGeometry>(m_pBranchGeometry, "~CSpeedTreeRT, m_pBranchGeometry");
        st_delete<CLightingEngine>(m_pLightingEngine, "~CSpeedTreeRT, m_pLightingEngine");
        st_delete<CWindEngine>(m_pWindEngine, "~CSpeedTreeRT, m_pWindEngine");
        st_delete<CLeafGeometry>(m_pLeafGeometry, "~CSpeedTreeRT, m_pLeafGeometry");
        st_delete_array<SGeometry::SLeaf>(m_pLeafLods, "~CSpeedTreeRT, m_pLeafLods");
        st_delete<CSimpleBillboard>(m_pSimpleBillboard, "~CSpeedTreeRT, m_pSimpleBillboard");
        st_delete<CTreeEngine>(m_pEngine, "~CSpeedTreeRT, m_pEngine");
        st_delete<unsigned int>(m_pInstanceRefCount, "~CSpeedTreeRT, m_pInstanceRefCount");
        st_delete<SInstanceList>(m_pInstanceList, "~CSpeedTreeRT, m_pInstanceList");
        st_delete_array<float>(m_pTreeSizes, "~CSpeedTreeRT, m_pTreeSizes");
        st_delete<SCollisionObjects>(m_pCollisionObjects, "~CSpeedTreeRT, m_pCollisionObjects");
        st_delete<SEmbeddedTexCoords>(m_pEmbeddedTexCoords, "~CSpeedTreeRT, m_pEmbeddedTexCoords");
        st_delete<CFrondEngine>(m_pFrondEngine, "~CSpeedTreeRT, m_pFrondEngine");
        st_delete<CIndexedGeometry>(m_pFrondGeometry, "~CSpeedTreeRT, m_pFrondGeometry");
        st_delete_array<float>(m_pLeafLodSizeFactors, "~CSpeedTreeRT, m_pLeafLodSizeFactors");
        st_delete<CProjectedShadow>(m_pProjectedShadow, "~CSpeedTreeRT, m_pProjectedShadow");
        st_delete_array<char>(m_pUserData, "~CSpeedTreeRT, m_pUserData");
        st_delete<CMapBank>(m_pMapBank, "~CSpeedTreeRT, m_pMapBank");
    }

    // null the pointers because delete has been called on this
    // instance even though memory may not necessarily be freed.

    //lint -save -e672
    m_pBranchGeometry = NULL;
    m_pLightingEngine = NULL;
    m_pWindEngine = NULL;
    m_pLeafGeometry = NULL;
    m_pLeafLods = NULL;
    m_pSimpleBillboard = NULL;
    m_pEngine = NULL;
    m_pInstanceRefCount = NULL;
    m_pTreeSizes = NULL;
    m_pInstanceList = NULL;
    m_pCollisionObjects = NULL;
    m_pEmbeddedTexCoords = NULL;
    m_pFrondEngine = NULL;
    m_pFrondGeometry = NULL;
    m_pLeafLodSizeFactors = NULL;
    m_pProjectedShadow = NULL;
    m_pUserData = NULL;
    m_pMapBank = NULL;
    //lint -restore

    if (--m_uiAllRefCount == 0)
    {
        stBezierSpline::ClearCache( );
        st_delete<st_vector_speedtree_p>(g_pUniqueTrees, "~CSpeedTreeRT, g_pUniqueTrees");
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::Compute definition

bool CSpeedTreeRT::Compute(const float* pTransform, unsigned int nSeed, bool bCompositeStrips)
{
    try
    {
        if (!m_bTreeComputed)
        {
            // compute tree's geometry
            CBranch::SetFrondEngine(m_pFrondEngine);
            m_pEngine->SetSeed(nSeed);
            m_pEngine->Compute(m_fLeafSizeIncreaseFactor);
            m_pFrondEngine->Compute(m_pFrondGeometry, m_pLightingEngine);
            m_nNumFrondLodLevels = m_pFrondGeometry->GetNumLodLevels( );

            m_pEngine->InitTables( );

            // compute static lighting
            if (m_pLightingEngine->GetLeafLightingMethod( ) == CSpeedTreeRT::LIGHT_STATIC)
                ComputeLeafStaticLighting( );

            // prep wind and leaves
            const SIdvWindInfo& sWindInfo = m_pEngine->GetWindData( );
            m_pWindEngine->Init(sWindInfo);
            m_pLeafGeometry->Init(m_pEngine->GetNumLeafLodLevels( ), m_pEngine->GetAllLeaves( ), m_pEngine->GetLeafInfo( ), m_fLeafSizeIncreaseFactor);
            m_pLeafLods = st_new_array<SGeometry::SLeaf>(m_pLeafGeometry->GetNumLodLevels( ), "CSpeedTreeRT::Compute, m_pLeafLods");

            // move embedded texcoords (from file) into leaf geometry class and fronds
            if (m_pEmbeddedTexCoords)
            {
                int i = 0; // declared outside of the for-scope for portability
                for (i = 0; i < m_pEmbeddedTexCoords->m_nNumLeafMaps; ++i)
                    m_pLeafGeometry->SetTextureCoords(i, &(m_pEmbeddedTexCoords->m_pLeafTexCoords[i * 8]));

                bool bTextureFlip = GetTextureFlip( );
                for (i = 0; i < m_pEmbeddedTexCoords->m_nNumFrondMaps; ++i)
                    m_pFrondEngine->SetTextureCoords(m_pFrondGeometry, i, &(m_pEmbeddedTexCoords->m_pFrondTexCoords[i * 8]), bTextureFlip);
            }

            // for each branch LOD level, convert all strips to single strip using degenerate triangles
            if (bCompositeStrips)
            {
                m_pBranchGeometry->CombineStrips( );
                m_pFrondGeometry->CombineStrips(false);
            }
            m_pBranchGeometry->Finish( );
            m_pFrondGeometry->Finish( );

            // precompute a few parameters for later LOD optimizations
            m_nNumBranchLodLevels = m_pBranchGeometry->GetNumLodLevels( );
            m_nNumFrondLodLevels = m_pFrondGeometry->GetNumLodLevels( );
            m_nNumLeafLodLevels = m_pLeafGeometry->GetNumLodLevels( );

            m_fTransitionWidth = 2.0f * m_fLeafLodTransitionRadius;

            const int nNumTransitions = m_nNumLeafLodLevels - 1;
            m_fCrestWidth = nNumTransitions == 0 ? 1.0f : (1.0f - nNumTransitions * m_fTransitionWidth) / nNumTransitions;
            m_fCrestWidthBB = nNumTransitions == 0 ? 1.0f : (1.0f - (nNumTransitions + 1) * m_fTransitionWidth) / nNumTransitions;
            m_fCycleLength = m_fCrestWidth + m_fTransitionWidth;
            m_fCycleLengthBB = m_fCrestWidthBB + m_fTransitionWidth;

            // apply optional transformation
            if (pTransform)
            {
                stTransform cTransform;
                memcpy((void*) cTransform.m_afData, pTransform, 16 * sizeof(float));

                m_pBranchGeometry->Transform(cTransform);
                m_pLeafGeometry->Transform(cTransform);
                m_pFrondGeometry->Transform(cTransform);
                if (m_pCollisionObjects)
                    m_pCollisionObjects->TransformAll(cTransform);
            }

            // retrieve extents from branch and leaf geometry classes
            stRegion cExtents;
            m_pBranchGeometry->ComputeExtents(cExtents);
            m_pLeafGeometry->ComputeExtents(cExtents);
            m_pFrondGeometry->ComputeExtents(cExtents);

            memcpy(m_pTreeSizes + STS_MIN_BOX, cExtents.m_cMin.m_afData, 3 * sizeof(float));
            memcpy(m_pTreeSizes + STS_MAX_BOX, cExtents.m_cMax.m_afData, 3 * sizeof(float));

            // figure out how wide the billboard should be
            stVec3 cOrigin(0.0f, 0.0f, 0.0f);
            //lint -save -e666
#ifdef UPVECTOR_POS_Y
            float fDimension = cOrigin.Distance(stVec3(m_pTreeSizes[0], m_pTreeSizes[2], 0.0f));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[0], m_pTreeSizes[5], 0.0f)));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[3], m_pTreeSizes[2], 0.0f)));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[3], m_pTreeSizes[5], 0.0f)));
#else
            float fDimension = cOrigin.Distance(stVec3(m_pTreeSizes[0], m_pTreeSizes[1], 0.0f));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[0], m_pTreeSizes[4], 0.0f)));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[3], m_pTreeSizes[1], 0.0f)));
            fDimension = st_max(fDimension, cOrigin.Distance(stVec3(m_pTreeSizes[3], m_pTreeSizes[4], 0.0f)));
#endif
            //lint -restore
            m_pTreeSizes[STS_BB_SIZE] = fDimension * 2.0f;

            // compute the self-shadow coordinates
            if (m_pProjectedShadow)
                ComputeSelfShadowTexCoords( );

            // setup the horizontal billboard
            SetupHorizontalBillboard( );

            // some operations are not valid once the geometry has been computed
            m_bTreeComputed = true;
        }
        else
            SetError("Compute() called more than once for single tree model (ignored)");
    }

    SpeedTreeCatch("CSpeedTreeRT::Compute")
    SpeedTreeCatchAll("CSpeedTreeRT::Compute");

    return m_bTreeComputed;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::Clone definition

CSpeedTreeRT* CSpeedTreeRT::Clone(void) const
{
    CSpeedTreeRT* pClone = NULL;

    if (m_pEngine && m_pEngine->TransientDataIntact( ))
    {
        unsigned int uiNumBytes = 0;
        unsigned char* pSptChunk = SaveTree(uiNumBytes);
        if (pSptChunk)
        {
            pClone = new CSpeedTreeRT;
            if (!pClone->LoadTree(pSptChunk, uiNumBytes))
            {
                delete pClone;
                pClone = NULL;
            }
            st_delete_array<unsigned char>(pSptChunk, "CSpeedTreeRT::Clone, pSptChunk");
        }
    }

    return pClone;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::InstanceOf definition

const CSpeedTreeRT* CSpeedTreeRT::InstanceOf(void) const
{
    const CSpeedTreeRT* pParent = NULL;
    if (m_pInstanceData)
        pParent = m_pInstanceData->m_pParent;

    return pParent;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::MakeInstance definition

CSpeedTreeRT* CSpeedTreeRT::MakeInstance(void)
{
    CSpeedTreeRT* pInstance = NULL;
    try
    {
        pInstance = new CSpeedTreeRT(this);
    }

    SpeedTreeCatch("CSpeedTreeRT::MakeInstance")
    SpeedTreeCatchAll("CSpeedTreeRT::MakeInstance");

    return pInstance;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::DeleteTransientData definition

void CSpeedTreeRT::DeleteTransientData(void)
{
    if (m_pEngine->TransientDataIntact( ))
        m_pEngine->FreeTransientData( );
    else
        SetError("DeleteTransientData() called with no intact transient data");

    stBezierSpline::ClearCache( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::LoadTree definition

bool CSpeedTreeRT::LoadTree(const char* pFilename)
{
    #include "EvalTest.h"

    bool bSuccess = false;
    unsigned char* pMemory = NULL;

    try
    {
        if (pFilename)
        {
            FILE* pFile = fopen(pFilename, "rb");
            if (pFile)
            {
                fseek(pFile, 0L, SEEK_END);
                int nNumBytes = ftell(pFile);
                int nErrorCode = fseek(pFile, 0, SEEK_SET);

                if (nNumBytes > 0 &&
                    nErrorCode >= 0)
                {
                    // read all of the file into memory, then pass into other Parse() function
                    pMemory = st_new_array<unsigned char>(nNumBytes, "CSpeedTreeRT::LoadTree, pMemory");
                    int nBytesRead = int(fread(pMemory, 1, nNumBytes, pFile));
                    if (nBytesRead == nNumBytes)
                    {
                        bSuccess = LoadTree(pMemory, nNumBytes);
                        st_delete_array<unsigned char>(pMemory, "CSpeedTreeRT::LoadTree, pMemory");
                    }
                    else
                        throw(runtime_error(IdvFormatString("only read %d of %d from %s [%s]", nBytesRead, nNumBytes, pFilename, strerror(errno)).c_str( )));

                    // in our experience fclose() doesn't always give accurate return values
                    (void) fclose(pFile);
                }
                else
                    throw(runtime_error(IdvFormatString("file seek failed on '%s' [%s]", pFilename, strerror(errno)).c_str( )));
            }
            else
                throw(runtime_error(IdvFormatString("failed to load file '%s' [%s]", pFilename, strerror(errno)).c_str( )));
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::LoadTree(Filename)")
    SpeedTreeCatchAll("CSpeedTreeRT::LoadTree(Filename)");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::LoadTree definition

bool CSpeedTreeRT::LoadTree(const unsigned char* pBlock, unsigned int nNumBytes)
{
    //lint -e537  { it's OK that this file is included twice }
    #include "EvalTest.h"

    bool bSuccess = false;
    try
    {
        CTreeFileAccess cFile(pBlock, nNumBytes);
        if (m_pEngine->Parse(cFile))
        {
            bool bParsedMaps = false;
            bool bParsedNewTexCoords = false;
            if (!cFile.EndOfFile( ))
            {
                // new spt data is saved after all original format to maintain backward compatibility
                bool bEndOfFile = false;
                int nToken = cFile.ParseToken( );
                do
                {
                    switch (nToken)
                    {
                    case File_BeginLightingInfo:
                        m_pLightingEngine->Parse(cFile);
                        break;
                    case File_BeginLodInfo:
                        ParseLodInfo(&cFile);
                        break;
                    case File_BeginNewWindInfo:
                        ParseWindInfo(&cFile);
                        break;
                    case File_BeginTextureCoordInfo:
                        ParseTextureCoordInfo(&cFile);
                        break;
                    case File_BeginCollisionInfo:
                        ParseCollisionObjects(&cFile);
                        break;
                    case File_BeginFrondInfo:
                        m_pFrondEngine->Parse(cFile);
                        break;
                    case File_BeginTextureControls:
                        m_pEngine->ParseTextureControls(cFile);
                        break;
                    case File_BeginFlareInfo:
                        m_pEngine->ParseFlareInfo(cFile);
                        break;
                    case File_FlareSeed:
                        m_pEngine->ParseFlareSeed(cFile);
                        break;
                    case File_LeafTransitionFactor:
                        m_fLeafTransitionFactor = cFile.ParseFloat( );
                        break;
                    case File_BeginShadowProjectionInfo:
                        ParseShadowProjectionInfo(&cFile);
                        break;
                    case File_BeginUserData:
                        ParseUserData(&cFile);
                        break;
                    case File_BeginSupplementalTexCoordInfo:
                        ParseSupplementalTexCoordInfo(&cFile);
                        break;
                    case File_SpeedWindRockScalar:
                        m_pWindEngine->SetSpeedWindRockScalar(cFile.ParseFloat( ));
                        break;
                    case File_SpeedWindRustleScalar:
                        m_pWindEngine->SetSpeedWindRustleScalar(cFile.ParseFloat( ));
                        break;
                    case File_PropagateFlexibility:
                        m_pEngine->SetPropagateFlexibility(cFile.ParseBool( ));
                        break;
                    case File_BeginLightSeamReductionInfo:
                        m_pEngine->ParseLightSeamReductionInfo(cFile);
                        break;
                    case File_BeginSupplementalLeafPlacementInfo:
                        m_pEngine->ParseSupplementalLeafPlacementInfo(cFile);
                        break;
                    case File_BeginSupplementalFrondInfo:
                        m_pFrondEngine->ParseSupplementalInfo(cFile);
                        break;
                    case File_BeginSupplementalBranchInfo:
                        m_pEngine->ParseSupplementalBranchInfo(cFile);
                        break;
                    case File_BeginFloorInfo:
                        m_pEngine->ParseFloorInfo(cFile);
                        break;
                    case File_BeginLeafNormalSmoothing:
                        m_pEngine->ParseLeafNormalSmoothing(cFile);
                        break;
                    case File_BeginClusterInfo:
                        m_pEngine->ParseClusterInfo(cFile);
                        break;
                    case File_BeginStandardShaderInfo:
                        ParseStandardShaderInfo(cFile);
                        break;
                    case File_BeginRootSupportInfo:
                        m_pEngine->ParseRootSupportInfo(cFile);
                        break;
                    case File_BeginTexCoordControls:
                        m_pEngine->ParseTexCoordControls(cFile);
                        bParsedNewTexCoords = true;
                        break;
                    case File_BeginMapBank:
                        m_pMapBank->Parse(cFile);
                        bParsedMaps = true;
                        break;
                    case File_BeginMeshes:
                        m_pEngine->ParseMeshes(cFile);
                        break;
                    case File_BeginLeafMeshInfo:
                        m_pEngine->ParseLeafMeshInfo(cFile);
                        break;
                    case File_BeginSupplementalCollisionObjectInfo:
                        ParseSupplementalCollisionObjectsInfo(cFile);
                        break;
                    case File_BeginSupplementalGlobalInfo:
                        m_pEngine->ParseSupplementalGlobalInfo(cFile);
                        break;
                    case File_BeginSupplementalLodInfo:
                        ParseSupplementalLodInfo(cFile);
                        break;
                    default:
                        bEndOfFile = true;
                    }
                    if (cFile.EndOfFile( ))
                        bEndOfFile = true;
                    else
                        nToken = cFile.ParseToken( );
                } while (!bEndOfFile);
            }
            
            // some branch parameters may not have been parsed because they weren't part
            // of the older SPT file formats.  these aren't given default values to avoid
            // redundant computation because the stBezierSpline::Parse function is expensive
            // since it involves a lot of spline computation.
            m_pEngine->InitMissingBranchProfiles( );

            // update all lighting data
            SetBranchLightingMethod(m_pLightingEngine->GetBranchLightingMethod( ));
            SetLeafLightingMethod(m_pLightingEngine->GetLeafLightingMethod( ));
            SetFrondLightingMethod(m_pLightingEngine->GetFrondLightingMethod( ));

            // extract other data for later queries
            m_nBranchWindLevel = m_pEngine->GetBranchLevelForWeighting( );
            m_nFrondLevel = m_pFrondEngine->GetLevel( );

            // handle obsolete smooth_2 setting
            if (m_eLeafLodMethod == 2)
            {
                m_eLeafLodMethod = CSpeedTreeRT::LOD_SMOOTH;
                m_fLeafTransitionFactor = 0.5f;
            }
            // make matrix span match number of available matrices
            m_pWindEngine->SetLocalMatrices(0, m_pWindEngine->GetNumWindMatrices( ));

            if (!bParsedMaps)
                RecoverDeprecatedMaps( );

            if (!bParsedNewTexCoords)
                m_pEngine->SynchTexCoords( );

            bSuccess = true;
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::LoadTree(Memory block)")
    SpeedTreeCatchAll("CSpeedTreeRT::LoadTree(Memory block)");

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveTree definition

unsigned char* CSpeedTreeRT::SaveTree(unsigned int& nNumBytes, bool bSaveLeaves) const
{
    unsigned char* pData = NULL;
    try
    {
        if (m_pEngine->TransientDataIntact( ))
        {
            CTreeFileAccess cFile;
            // save original spt data
            m_pEngine->Save(cFile, bSaveLeaves);

            // save lighting info
            m_pLightingEngine->Save(cFile);

            // save lod info
            cFile.SaveToken(File_BeginLodInfo);
            cFile.SaveToken(File_LeafTransitionMethod);
            cFile.SaveInt(static_cast<int>(m_eLeafLodMethod));
            cFile.SaveToken(File_LeafTransitionRadius);
            cFile.SaveFloat(m_fLeafLodTransitionRadius);
            cFile.SaveToken(File_LeafCurveExponent);
            cFile.SaveFloat(m_fLeafLodCurveExponent);
            cFile.SaveToken(File_LeafSizeIncreaseFactor);
            cFile.SaveFloat(m_fLeafSizeIncreaseFactor);
            m_pEngine->SaveLodInfo(cFile);
            cFile.SaveToken(File_EndLodInfo);

            // save wind info
            cFile.SaveToken(File_BeginNewWindInfo);
            cFile.SaveToken(File_WindLevel);
            cFile.SaveInt(m_pEngine->GetBranchLevelForWeighting( ));
            cFile.SaveToken(File_EndNewWindInfo);

            // save texture coord info
            if (m_pEmbeddedTexCoords)
                SaveTextureCoords(&cFile);

            // save collision objects
            if (m_pCollisionObjects)
                SaveCollisionObjects(&cFile);

            // save frond info
            m_pFrondEngine->Save(cFile);

            // save extras
            m_pEngine->SaveTextureControls(cFile);
            m_pEngine->SaveFlareInfo(cFile);
            m_pEngine->SaveFlareSeed(cFile);
            cFile.SaveToken(File_LeafTransitionFactor);
            cFile.SaveFloat(m_fLeafTransitionFactor);

            // save projected shadows
            if (m_pProjectedShadow)
                m_pProjectedShadow->Save(cFile);

            // save user data
            SaveUserData(&cFile);

            // save supplemental texcoord info
            if (m_pEmbeddedTexCoords)
                SaveSupplementalTexCoordInfo(&cFile);

            // speedwind scalars
            cFile.SaveToken(File_SpeedWindRockScalar);
            cFile.SaveFloat(m_pWindEngine->GetSpeedWindRockScalar( ));
            cFile.SaveToken(File_SpeedWindRustleScalar);
            cFile.SaveFloat(m_pWindEngine->GetSpeedWindRustleScalar( ));

            // flexibility propagation
            cFile.SaveToken(File_PropagateFlexibility);
            cFile.SaveBool(m_pEngine->GetPropagateFlexibility( ));

            // frond supplemental info
            m_pFrondEngine->SaveSupplementalInfo(cFile);

            // branch supplemental info
            m_pEngine->SaveSupplementalBranchInfo(cFile);

            // floor info
            m_pEngine->SaveFloorInfo(cFile);

            // leaf normal smoothing
            m_pEngine->SaveLeafNormalSmoothing(cFile);

            // cluster info
            m_pEngine->SaveClusterInfo(cFile);

            // standard shader support
            SaveStandardShaderInfo(cFile);

            // roots
            m_pEngine->SaveRootSupportInfo(cFile);

            // tex coord controls
            m_pEngine->SaveTexCoordControls(cFile);

            // map names
            m_pMapBank->Save(cFile);

            // meshes
            m_pEngine->SaveMeshes(cFile);

            // leaf mesh info
            m_pEngine->SaveLeafMeshInfo(cFile);

            // supplemental collision info
            if (m_pCollisionObjects)
                SaveSupplementalCollisionObjectsInfo(cFile);

            // supplemental global info
            m_pEngine->SaveSupplementalGlobalInfo(cFile);

            // supplemental lod info
            SaveSupplementalLodInfo(cFile);

            // create spt formatted data
            pData = cFile.ConvertMemoryToArray(nNumBytes);
        }
        else
            SetError("cannot Save() after DeleteTransientData() is called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SaveTree")
    SpeedTreeCatchAll("CSpeedTreeRT::SaveTree");

    return pData;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetTreeSize definition

void CSpeedTreeRT::GetTreeSize(float& fSize, float& fVariance) const
{
    m_pEngine->GetSize(fSize, fVariance);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetTreeSize definition

void CSpeedTreeRT::SetTreeSize(float fNewSize, float fNewVariance)
{
    try
    {
        if (m_pEngine->TransientDataIntact( ))
        {
            if (fNewSize > 0.0f)
                m_pEngine->SetSize(fNewSize, fNewVariance);
            else
                SetError("SetTreeSize() is only valid for size values greater than 0.0");
        }
        else
            SetError("SetTreeSize() has no effect after DeleteTransientData() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetTreeSize")
    SpeedTreeCatchAll("CSpeedTreeRT::SetTreeSize");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetSeed definition

unsigned int CSpeedTreeRT::GetSeed(void) const
{
    unsigned int nSeed = 0;
    try
    {
        if (m_pEngine->TransientDataIntact( ))
            nSeed = m_pEngine->GetSeed( );
        else
            SetError("SetTreeSize() has no effect after DeleteTransientData() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetTreeSize")
    SpeedTreeCatchAll("CSpeedTreeRT::SetTreeSize");

    return nSeed;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetTreePosition definition

const float* CSpeedTreeRT::GetTreePosition(void) const
{
    const float* pPos = NULL;
    if (m_pInstanceData)
        pPos = m_pInstanceData->m_cPosition.m_afData;
    else
        pPos = m_pEngine->GetPosition( ).m_afData;

    return pPos;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetTreePosition definition

void CSpeedTreeRT::SetTreePosition(float x, float y, float z)
{
    if (m_pInstanceData)
        m_pInstanceData->m_cPosition.Set(x, y, z);
    else
        m_pEngine->SetPosition(stVec3(x, y, z));
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafTargetAlphaMask definition

void CSpeedTreeRT::SetLeafTargetAlphaMask(unsigned char ucMask)
{
    m_fTargetAlphaValue = float(ucMask);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBranchLightingMethod definition

CSpeedTreeRT::ELightingMethod CSpeedTreeRT::GetBranchLightingMethod(void) const
{
    return m_pLightingEngine->GetBranchLightingMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetBranchLightingMethod definition

void CSpeedTreeRT::SetBranchLightingMethod(ELightingMethod eMethod)
{
    try
    {
        if (!m_bTreeComputed)
        {
            m_pBranchGeometry->EnableManualLighting(eMethod == LIGHT_STATIC);
            m_pLightingEngine->SetBranchLightingMethod(eMethod);
        }
        else
            SetError("SetBranchLightingMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetBranchLightingMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetBranchLightingMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafLightingMethod definition

CSpeedTreeRT::ELightingMethod CSpeedTreeRT::GetLeafLightingMethod(void) const
{
    return m_pLightingEngine->GetLeafLightingMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafLightingMethod definition

void CSpeedTreeRT::SetLeafLightingMethod(ELightingMethod eMethod)
{
    try
    {
        if (!m_bTreeComputed)
        {
            m_pLightingEngine->SetLeafLightingMethod(eMethod);
            m_pLeafGeometry->EnableManualLighting(eMethod == LIGHT_STATIC);
        }
        else
            SetError("SetLeafLightingMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafLightingMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafLightingMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetStaticLightingStyle definition

CSpeedTreeRT::EStaticLightingStyle CSpeedTreeRT::GetStaticLightingStyle(void) const
{
    return m_pLightingEngine->GetStaticLightingStyle( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetStaticLightingStyle definition

void CSpeedTreeRT::SetStaticLightingStyle(EStaticLightingStyle eStyle)
{
    try
    {
        if (!m_bTreeComputed)
        {
            st_assert(m_pLightingEngine);
            m_pLightingEngine->SetStaticLightingStyle(eStyle);
        }
        else
            SetError("SetStaticLightingStyle() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetStaticLightingStyle")
    SpeedTreeCatchAll("CSpeedTreeRT::SetStaticLightingStyle");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafLightingAdjustment definition

float CSpeedTreeRT::GetLeafLightingAdjustment( ) const
{
    return m_pLightingEngine->GetLeafLightingAdjustment( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafLightingAdjustment definition

void CSpeedTreeRT::SetLeafLightingAdjustment(float fScalar)
{
    try
    {
        if (!m_bTreeComputed)
            m_pLightingEngine->SetLeafLightingAdjustment(fScalar);
        else
            SetError("SetLeafLightingAdjustment() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafLightingAdjustment")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafLightingAdjustment");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetFrondLightingMethod definition

CSpeedTreeRT::ELightingMethod CSpeedTreeRT::GetFrondLightingMethod(void) const
{
    return m_pLightingEngine->GetFrondLightingMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetFrondLightingMethod definition

void CSpeedTreeRT::SetFrondLightingMethod(ELightingMethod eMethod)
{
    try
    {
        if (!m_bTreeComputed)
        {
            m_pFrondGeometry->EnableManualLighting(eMethod == LIGHT_STATIC);
            m_pLightingEngine->SetFrondLightingMethod(eMethod);
        }
        else
            SetError("SetFrondLightingMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetFrondLightingMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetFrondLightingMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLightState definition

bool CSpeedTreeRT::GetLightState(unsigned int nLightIndex)
{
    return CLightingEngine::GetLightState(nLightIndex);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLightState definition

void CSpeedTreeRT::SetLightState(unsigned int nLightIndex, bool bLightState)
{
    CLightingEngine::SetLightState(nLightIndex, bLightState);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLightAttributes definition

const float* CSpeedTreeRT::GetLightAttributes(unsigned int nLightIndex)
{
    return CLightingEngine::GetLightAttributes(nLightIndex);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLightAttributes definition

void CSpeedTreeRT::SetLightAttributes(unsigned int nLightIndex, const float* pAttributes)
{
    try
    {
        CLightingEngine::SetLightAttributes(nLightIndex, pAttributes);
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLightAttributes")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLightAttributes");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBranchMaterial definition

const float* CSpeedTreeRT::GetBranchMaterial(void) const
{
    return m_pLightingEngine->GetBranchMaterial( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetBranchMaterial definition

void CSpeedTreeRT::SetBranchMaterial(const float* pBranchMaterial)
{
    try
    {
        if (!m_bTreeComputed)
            m_pLightingEngine->SetBranchMaterial(pBranchMaterial);
        else
            SetError("SetBranchMaterial() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetBranchMaterial")
    SpeedTreeCatchAll("CSpeedTreeRT::SetBranchMaterial");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafMaterial definition

const float* CSpeedTreeRT::GetLeafMaterial(void) const
{
    return m_pLightingEngine->GetLeafMaterial( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafMaterial definition

void CSpeedTreeRT::SetLeafMaterial(const float* pLeafMaterial)
{
    try
    {
        if (!m_bTreeComputed)
            m_pLightingEngine->SetLeafMaterial(pLeafMaterial);
        else
            SetError("SetLeafMaterial() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafMaterial")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafMaterial");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetFrondMaterial definition

const float* CSpeedTreeRT::GetFrondMaterial (void) const
{
    return m_pLightingEngine->GetFrondMaterial( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetFrondMaterial definition

void CSpeedTreeRT::SetFrondMaterial(const float* pLeafMaterial)
{
    try
    {
        if (!m_bTreeComputed)
            m_pLightingEngine->SetFrondMaterial(pLeafMaterial);
        else
            SetError("SetFrondMaterial() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetFrondMaterial")
    SpeedTreeCatchAll("CSpeedTreeRT::SetFrondMaterial");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetCamera definition

void CSpeedTreeRT::GetCamera(float* pPosition, float* pDirection)
{
    if (pPosition && pDirection)
    {
        stVec3 cCameraPos, cCameraDirection;
        CTreeEngine::GetCamera(cCameraPos, cCameraDirection);

        memcpy(pPosition, cCameraPos.m_afData, 3 * sizeof(float));
        memcpy(pDirection, cCameraDirection.m_afData, 3 * sizeof(float));
    }
    else
        SetError("GetCamera() requires non-NULL position and direction values");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetCamera definition

void CSpeedTreeRT::SetCamera(const float *pPosition, const float* pDirection)
{
    try
    {
        if (pPosition && pDirection)
        {
            stVec3 cOldPosition, cOldDirection;
            CTreeEngine::GetCamera(cOldPosition, cOldDirection);

            stVec3 cNewPosition(pPosition[0], pPosition[1], pPosition[2]);
            stVec3 cNewDirection(pDirection[0], pDirection[1], pDirection[2]);

            // only want to recompute billboards if necessary
            if (cOldPosition != cNewPosition ||
                cOldDirection != cNewDirection)
            {
                // compute camera azimuth and pitch using up axis adjusted camera direction
                stVec3 cAdjustedDirection;
                Assign3d(cAdjustedDirection, pDirection);
                m_fCameraAzimuth = VecRad2Deg(atan2f(cAdjustedDirection[1], cAdjustedDirection[0]));
                m_fCameraPitch = -VecRad2Deg(asinf(cAdjustedDirection[2]));

                CTreeEngine::SetCamera(cNewPosition, cNewDirection);
                NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_CAMERA);
                CSimpleBillboard::ComputeUnitBillboard(m_fCameraAzimuth);
                NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_CAMERA);

                // determine horizontal billboard fade value
                m_fHorizontalFadeValue = (fabs(m_fCameraPitch) - m_fHorizontalFadeStartAngle) / (m_fHorizontalFadeEndAngle - m_fHorizontalFadeStartAngle);
                m_fHorizontalFadeValue = st_max(0.0f, m_fHorizontalFadeValue);
                m_fHorizontalFadeValue = st_min(1.0f, m_fHorizontalFadeValue);
            }
        }
        else
            SetError("SetCamera() requires non-NULL position and direction values");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetCamera")
    SpeedTreeCatchAll("CSpeedTreeRT::SetCamera");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetCameraAngles definition

void CSpeedTreeRT::GetCameraAngles(float& fAzimuth, float& fPitch)
{
    fAzimuth = m_fCameraAzimuth;
    fPitch = m_fCameraPitch;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetTime definition

void CSpeedTreeRT::SetTime(float fTime)
{
    try
    {
        CWindEngine::SetTime(fTime);
        NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_WIND);
    }

    SpeedTreeCatch("CSpeedTreeRT::SetTime")
    SpeedTreeCatchAll("CSpeedTreeRT::SetTime");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeWindEffects definition

void CSpeedTreeRT::ComputeWindEffects(bool bBranches, bool bLeaves, bool bFronds)
{
    try
    {
        float fHighestLod = -1.0f;

        // branches
        if (bBranches &&
            m_pWindEngine->GetBranchWindMethod( ) == CSpeedTreeRT::WIND_CPU)
        {
            if (!m_pInstanceData && *m_pInstanceRefCount > 1)
            {
                // If this tree has instances of itself, then compute the highest LOD
                // among itself and its instances.  There's no need for more wind
                // computations than the highest LOD because all of the LODs share
                // the same vertexes.
                fHighestLod = m_pEngine->GetLod( );
                for (st_vector_speedtree_p::iterator i = m_pInstanceList->m_vInstances.begin( );
                     i != m_pInstanceList->m_vInstances.end( ); ++i)
                {
                    float fInstanceLodLevel = (*i)->GetLodLevel( );
                    if (fInstanceLodLevel > fHighestLod)
                        fHighestLod = fInstanceLodLevel;
                }

                (void) m_pBranchGeometry->ComputeWindEffect(GetDiscreteBranchLodLevel(fHighestLod));
            }
            else
                (void) m_pBranchGeometry->ComputeWindEffect(GetDiscreteBranchLodLevel(GetLodLevel( )));
        }

        // fronds
        if (bFronds &&
            m_pWindEngine->GetFrondWindMethod( ) == CSpeedTreeRT::WIND_CPU)
        {
            if (!m_pInstanceData && *m_pInstanceRefCount > 1)
            {
                // If this tree has instances of itself, then compute the highest LOD
                // among itself and its instances.  There's no need for more wind
                // computations than the highest LOD because all of the LODs share
                // the same vertexes.
                if (fHighestLod == -1.0f) // we don't need to make this calc if it was
                {                         // already done for the branches
                    fHighestLod = m_pEngine->GetLod( );
                    for (st_vector_speedtree_p::iterator i = m_pInstanceList->m_vInstances.begin( );
                         i != m_pInstanceList->m_vInstances.end( ); ++i)
                    {
                        float fInstanceLodLevel = (*i)->GetLodLevel( );
                        if (fInstanceLodLevel > fHighestLod)
                            fHighestLod = fInstanceLodLevel;
                    }
                }

                (void) m_pFrondGeometry->ComputeWindEffect(GetDiscreteFrondLodLevel(fHighestLod));
            }
            else
                (void) m_pFrondGeometry->ComputeWindEffect(GetDiscreteFrondLodLevel(GetLodLevel( )));
        }

        // leaves
        if (bLeaves &&
            m_pWindEngine->GetLeafWindMethod( ) == CSpeedTreeRT::WIND_CPU)
        {
            m_pLeafGeometry->Invalidate( );
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::ComputeWindEffects")
    SpeedTreeCatchAll("CSpeedTreeRT::ComputeWindEffects");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ResetLeafWindState definition

void CSpeedTreeRT::ResetLeafWindState( )
{
    m_pWindEngine->ResetLeafWindState( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafRockingState definition

bool CSpeedTreeRT::GetLeafRockingState(void) const
{
    return m_pWindEngine->GetLeafRockingState( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafRockingState definition

void CSpeedTreeRT::SetLeafRockingState(bool bFlag)
{
    m_pWindEngine->SetLeafRockingState(bFlag);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetNumLeafRockingGroups definition

void CSpeedTreeRT::SetNumLeafRockingGroups(unsigned int nRockingGroups)
{
    try
    {
        if (!m_bTreeComputed)
        {
            if (nRockingGroups == 0)
                nRockingGroups = 1;

            m_pEngine->SetNumLeafRockingGroups(nRockingGroups);
        }
        else
            SetError("SetNumLeafRockingGroups() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafRockingState")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafRockingState");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafWindMethod definition

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetLeafWindMethod(void) const
{
    return m_pWindEngine->GetLeafWindMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafWindMethod definition

void CSpeedTreeRT::SetLeafWindMethod(EWindMethod eMethod)
{
    try
    {
        if (!m_bTreeComputed)
        {
            m_pWindEngine->SetLeafWindMethod(eMethod);
            m_pLeafGeometry->EnableVertexWeighting(eMethod != WIND_NONE);
        }
        else
            SetError("SetLeafWindMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafWindMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafWindMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBranchWindMethod definition

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetBranchWindMethod(void) const
{
    return m_pWindEngine->GetBranchWindMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetBranchWindMethod definition

void CSpeedTreeRT::SetBranchWindMethod(EWindMethod eMethod)
{
    try
    {
        // Because the wind engine is optimized to only calculate the wind
        // effect for the highest visible LOD, the highest overall LOD may
        // not be calcuated.  When the wind is turned off, some of the tree
        // will be positioned according to the last wind calculation and
        // some of it may not be.  We make a call here to force it to
        // calculate the entire tree's geometry for the last wind value.

        if (!m_bTreeComputed)
        {
            if (eMethod == WIND_NONE &&
                m_pWindEngine->GetBranchWindMethod( ) == WIND_CPU)
            {
                m_pBranchGeometry->Invalidate( );
                (void) m_pBranchGeometry->ComputeWindEffect(0);
            }

            m_pWindEngine->SetBranchWindMethod(eMethod);
            m_pBranchGeometry->EnableVertexWeighting(eMethod != WIND_NONE);
            m_pBranchGeometry->SetWindMethod(eMethod);
        }
        else
            SetError("SetBranchWindMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetBranchWindMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetBranchWindMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetFrondWindMethod definition

CSpeedTreeRT::EWindMethod CSpeedTreeRT::GetFrondWindMethod(void) const
{
    return m_pWindEngine->GetFrondWindMethod( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetFrondWindMethod definition

void CSpeedTreeRT::SetFrondWindMethod(EWindMethod eMethod)
{
    try
    {
        // Because the wind engine is optimized to only calculate the wind
        // effect for the highest visible LOD, the highest overall LOD may
        // not be calcuated.  When the wind is turned off, some of the tree
        // will be positioned according to the last wind calculation and
        // some of it may not be.  We make a call here to force it to
        // calculate the entire tree's geometry for the last wind value.

        if (!m_bTreeComputed)
        {
            if (eMethod == WIND_NONE &&
                m_pWindEngine->GetFrondWindMethod( ) == WIND_CPU)
            {
                m_pFrondGeometry->Invalidate( );
                (void) m_pFrondGeometry->ComputeWindEffect(0);
            }

            m_pWindEngine->SetFrondWindMethod(eMethod);
            m_pFrondGeometry->EnableVertexWeighting(eMethod != WIND_NONE);
            m_pFrondGeometry->SetWindMethod(eMethod);
        }
        else
            SetError("SetFrondWindMethod() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetFrondWindMethod")
    SpeedTreeCatchAll("CSpeedTreeRT::SetFrondWindMethod");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetWindStrength definition

float CSpeedTreeRT::GetWindStrength(void) const
{
    return m_pWindEngine->GetWindStrength( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetWindStrength definition

float CSpeedTreeRT::SetWindStrength(float fNewStrength, float fOldStrength, float fFrequencyTimeOffset)
{
    float fNewFrequencyTimeShift = 0.0f;

    try
    {
        if (fNewStrength >= 0.0f)
        {
            if (fOldStrength == -1.0f)
                fOldStrength = m_pWindEngine->GetWindStrength( );
            if (fFrequencyTimeOffset == -1.0f)
                fFrequencyTimeOffset = m_pWindEngine->GetFrequencyTimeShift( );

            fNewFrequencyTimeShift = m_pWindEngine->SetWindStrength(fNewStrength, fOldStrength, fFrequencyTimeOffset);

            // invalidate geometry if CPU-based wind
            if (GetBranchWindMethod( ) == WIND_CPU &&
                m_pBranchGeometry)
                m_pBranchGeometry->Invalidate( );

            if (GetFrondWindMethod( ) == WIND_CPU &&
                m_pFrondGeometry)
                m_pFrondGeometry->Invalidate( );

            if (GetLeafWindMethod( ) == WIND_CPU ||
                m_pWindEngine->GetLeafRockingState( ))
                m_pLeafGeometry->Invalidate( );
        }
        else
            SetError("SetWindStrength() expects new wind strength >= 0.0");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetWindStrength")
    SpeedTreeCatchAll("CSpeedTreeRT::SetWindStrength");

    return fNewFrequencyTimeShift;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetWindStrengthAndLeafAngles definition

void CSpeedTreeRT::SetWindStrengthAndLeafAngles(float fNewStrength, const float* pRockingAngles, const float* pRustleAngles, unsigned int uiNumRockingAngles)
{
    try
    {
        if (fNewStrength >= 0.0f)
        {
            m_pWindEngine->SetWindStrengthAndLeafAngles(fNewStrength, pRockingAngles, pRustleAngles, uiNumRockingAngles);

            // invalidate geometry if CPU-based wind
            if (GetBranchWindMethod( ) == WIND_CPU &&
                m_pBranchGeometry)
                m_pBranchGeometry->Invalidate( );

            if (GetFrondWindMethod( ) == WIND_CPU &&
                m_pFrondGeometry)
                m_pFrondGeometry->Invalidate( );

            if (GetLeafWindMethod( ) == WIND_CPU ||
                m_pWindEngine->GetLeafRockingState( ))
                m_pLeafGeometry->Invalidate( );
        }
        else
            SetError("SetWindStrength() expects new wind strength >= 0.0");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetWindStrength")
    SpeedTreeCatchAll("CSpeedTreeRT::SetWindStrength");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetNumWindMatrices definition

void CSpeedTreeRT::SetNumWindMatrices(int nNumMatrices)
{
    try
    {
        // there must be at least one wind matrix
        nNumMatrices = st_max(nNumMatrices, 1);

        CWindEngine::SetNumWindMatrices(static_cast<unsigned short>(nNumMatrices));
    }

    SpeedTreeCatch("CSpeedTreeRT::SetNumWindMatrices")
    SpeedTreeCatchAll("CSpeedTreeRT::SetNumWindMatrices");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetWindMatrix definition

void CSpeedTreeRT::SetWindMatrix(unsigned int nMatrixIndex, const float* pMatrix)
{
    st_assert(pMatrix);
    st_assert(nMatrixIndex >= 0 && nMatrixIndex < CWindEngine::GetNumWindMatrices( ));

    try
    {
        if (!pMatrix)
            SetError("SetWindMatrix() requires a non-NULL matrix pointer");
        else if (nMatrixIndex >= CWindEngine::GetNumWindMatrices( ))
            SetError("SetWindMatrix() - matrix index out of range");
        else
        {
            CWindEngine::SetWindMatrix(static_cast<unsigned short>(nMatrixIndex), pMatrix);
            NotifyAllTreesOfEvent(STIE_CLIENT_CHANGED_WIND);
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::SetWindMatrix")
    SpeedTreeCatchAll("CSpeedTreeRT::SetWindMatrix");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLocalMatrices definition

void CSpeedTreeRT::GetLocalMatrices(unsigned int& nStartingMatrix, unsigned int& nMatrixSpan)
{
    m_pWindEngine->GetLocalMatrices(nStartingMatrix, nMatrixSpan);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLocalMatrices definition

void CSpeedTreeRT::SetLocalMatrices(unsigned int nStartingMatrix, unsigned int nMatrixSpan)
{
    try
    {
        if (!m_bTreeComputed)
        {
            if (nStartingMatrix + nMatrixSpan - 1 < m_pWindEngine->GetNumWindMatrices( ))
                m_pWindEngine->SetLocalMatrices(nStartingMatrix, nMatrixSpan);
            else
                SetError("SetLocalMatrices() parameters exceed available wind matrices");
        }
        else
            SetError("SetLocalMatrices() has no effect after Compute() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLocalMatrices")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLocalMatrices");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeLodLevel definition

void CSpeedTreeRT::ComputeLodLevel(void)
{
    try
    {
        if (m_pInstanceData)
        {
            // must activate instance's attributes, make the computation,
            // then restore the parent's attributes
            stVec3 cOldPosition = m_pEngine->GetPosition( );
            float fOldLodLevel = m_pEngine->GetLod( );

            // compute
            m_pEngine->SetPosition(m_pInstanceData->m_cPosition);
            m_pInstanceData->m_fLodLevel = m_pEngine->ComputeLod( );

            // restore
            m_pEngine->SetLod(fOldLodLevel);
            m_pEngine->SetPosition(cOldPosition);
        }
        else
            (void) m_pEngine->ComputeLod( );
    }

    SpeedTreeCatch("CSpeedTreeRT::ComputeLodLevel")
    SpeedTreeCatchAll("CSpeedTreeRT::ComputeLodLevel");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLodLevel definition

float CSpeedTreeRT::GetLodLevel(void) const
{
    float fLevel = -1.0f; // default to invalid value

    if (m_pInstanceData)
        fLevel = m_pInstanceData->m_fLodLevel;
    else
        fLevel = m_pEngine->GetLod( );

    return fLevel;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLodLevel definition

void CSpeedTreeRT::SetLodLevel(float fLevel)
{
    // clamp the incoming value to [0.0,1.0]
    fLevel = st_min(fLevel, 1.0f);
    fLevel = st_max(fLevel, 0.0f);

    if (m_pInstanceData)
        m_pInstanceData->m_fLodLevel = fLevel;
    else
        m_pEngine->SetLod(fLevel);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetDropToBillboard definition

void CSpeedTreeRT::SetDropToBillboard(bool bFlag)
{
    m_bDropToBillboard = bFlag;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLodLimits definition

void CSpeedTreeRT::GetLodLimits(float& fNear, float& fFar) const
{
    m_pEngine->GetLodLimits(fNear, fFar);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLodLimits definition

void CSpeedTreeRT::SetLodLimits(float fNear, float fFar)
{
    m_pEngine->SetLodLimits(fNear, fFar);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetDiscreteBranchLodLevel definition

int CSpeedTreeRT::GetDiscreteBranchLodLevel(float fLodLevel) const
{
    int nLevel = 0;

    // -1.0f passed in means to use the current interval value
    if (fLodLevel == -1.0f)
        fLodLevel = GetLodLevel( );

    int nNumLodLevels = GetNumBranchLodLevels( );
    nLevel = int((1.0f - fLodLevel) * nNumLodLevels);
    if (nLevel == nNumLodLevels)
        nLevel--;

    st_assert(nLevel >= 0 && nLevel < GetNumBranchLodLevels( ));
    return nLevel;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetDiscreteFrondLodLevel definition

int CSpeedTreeRT::GetDiscreteFrondLodLevel(float fLodLevel) const
{
    int nLevel = 0;

    // -1.0f passed in means to use the current interval value
    if (fLodLevel == -1.0f)
        fLodLevel = GetLodLevel( );

    int nNumLodLevels = GetNumFrondLodLevels( );
    nLevel = int((1.0f - fLodLevel) * nNumLodLevels);
    if (nLevel == nNumLodLevels)
        nLevel--;

    st_assert(nLevel >= 0 && nLevel < GetNumFrondLodLevels( ));
    return nLevel;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetDiscreteLeafLodLevel definition

int CSpeedTreeRT::GetDiscreteLeafLodLevel(float fLodLevel) const
{
    int nLevel = 0;
    // -1.0f passed in means to use the current interval value
    if (fLodLevel == -1.0f)
        fLodLevel = GetLodLevel( );

    if (m_bDropToBillboard && m_pEmbeddedTexCoords && m_pEmbeddedTexCoords->m_nNumBillboards > 0)
    {
        int nNumLodLevels = GetNumLeafLodLevels( ) + 1;
        nLevel = int((1.0f - fLodLevel) * nNumLodLevels);
        if (nLevel == nNumLodLevels)
            nLevel--;

        st_assert(nLevel >= 0 && nLevel < nNumLodLevels);
    }
    else
    {
        int nNumLodLevels = GetNumLeafLodLevels( );
        nLevel = int((1.0f - fLodLevel) * nNumLodLevels);
        if (nLevel == nNumLodLevels)
            nLevel--;

        st_assert(nLevel >= 0 && nLevel < nNumLodLevels);
    }

    return nLevel;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumBranchLodLevels definition

int CSpeedTreeRT::GetNumBranchLodLevels(void) const
{
    return m_pEngine->GetNumBranchLodLevels( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumLeafLodLevels definition

int CSpeedTreeRT::GetNumLeafLodLevels(void) const
{
    return m_pEngine->GetNumLeafLodLevels( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumFrondLodLevels definition

int CSpeedTreeRT::GetNumFrondLodLevels(void) const
{
    return m_nNumFrondLodLevels;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::DeleteBranchGeometry definition

void CSpeedTreeRT::DeleteBranchGeometry(void)
{
    try
    {
        if (m_bTreeComputed &&
            m_pBranchGeometry &&
            !m_pInstanceData &&
            *m_pInstanceRefCount == 1)
        {
            st_delete<CIndexedGeometry>(m_pBranchGeometry, "CSpeedTreeRT::DeleteBranchGeometry, m_pBranchGeometry");
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::DeleteBranchGeometry")
    SpeedTreeCatchAll("CSpeedTreeRT::DeleteBranchGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafBillboardTable definition

const float* CSpeedTreeRT::GetLeafBillboardTable(unsigned int& nEntryCount) const
{
    const float* pBillboardTable = NULL;
    try
    {
        pBillboardTable = m_pLeafGeometry->GetLeafBillboardTable(nEntryCount);
    }

    SpeedTreeCatch("CSpeedTreeRT::GetLeafBillboardTable")
    SpeedTreeCatchAll("CSpeedTreeRT::GetLeafBillboardTable");

    return pBillboardTable;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::Authorize definition

bool CSpeedTreeRT::Authorize(const char* pKey)
{
#ifdef FULL_SPEEDTREE_LIB

#ifndef PS3
    pKey; // prevent unreferenced parameter warning
#endif

    return true;
#else
    g_strKey = pKey;

    st_string strFailureCause;
    return SpeedTreeKeyUtility::KeyIsValid(g_strKey, strFailureCause);
#endif
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::IsAuthorized definition

bool CSpeedTreeRT::IsAuthorized(void)
{
#ifdef FULL_SPEEDTREE_LIB
    return true;
#else
    st_string strFailureCause;
    return SpeedTreeKeyUtility::KeyIsValid(g_strKey, strFailureCause);
#endif
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetCurrentError definition

const char* CSpeedTreeRT::GetCurrentError(void)
{
    return g_strError.c_str( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ResetError definition

void CSpeedTreeRT::ResetError(void)
{
    g_strError = "";
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::Version definition

const char* CSpeedTreeRT::Version(void)
{
    static std::string strVersion;
    strVersion = "";

    // report version and platform
#ifdef _XBOX
    strVersion += "SpeedTreeRT Version 4.1 (Xenon/Xbox 360)";
#elif WIN32
    strVersion +=  "SpeedTreeRT Version 4.1 (Windows PC)";
#elif PS3
    strVersion +=  "SpeedTreeRT Version 4.1 (Cell/PS3)";
#else
    #ifdef BIG_ENDIAN
        strVersion +=  "SpeedTreeRT Version 4.1 (Unsupported Platform / Big-Endian Architecture)";
    #else
        strVersion +=  "SpeedTreeRT Version 4.1 (Unsupported Platform / Little-Endian Architecture)";
    #endif
#endif

    // licensed or evaluation?
#ifdef FULL_SPEEDTREE_LIB
    strVersion += ", Fully Licensed Build";
#else
    strVersion += ", Evaluation Build";
#endif

    // which up vector?
#ifdef UPVECTOR_POS_Z
    strVersion += ", +Z Up Vector";
#elif defined(UPVECTOR_NEG_Z)
    strVersion += ", -Z Up Vector";
#elif defined(UPVECTOR_POS_Y)
    strVersion += ", +Y Up Vector";
#elif defined(UPVECTOR_NEG_Y)
    strVersion += ", -Y Up Vector";
#elif defined(UPVECTOR_POS_X)
    strVersion += ", +X Up Vector";
#elif defined(UPVECTOR_NEG_X)
    strVersion += ", -X Up Vector";
#elif defined(UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM)
    strVersion += ", DX RHCS Up Vector";
#endif

    return strVersion.c_str( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBoundingBox definition

void CSpeedTreeRT::GetBoundingBox(float* pBounds) const
{
    if (pBounds)
        memcpy(pBounds, m_pTreeSizes, 6 * sizeof(float));
    else
        SetError("GetBoundingBox() expects a non-NULL parameter");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumLeafTriangles definition

int CSpeedTreeRT::GetNumLeafTriangles(float fLodLevel)
{
    int nTriangleCount = 0;

    SLodValues sLod;
    GetLodValues(sLod, fLodLevel);

    if (m_pLeafGeometry)
    {
        if (sLod.m_anLeafActiveLods[0] > -1)
            nTriangleCount += m_pLeafGeometry->GetTriangleCount(sLod.m_anLeafActiveLods[0]);
        if (sLod.m_anLeafActiveLods[1] > -1)
            nTriangleCount += m_pLeafGeometry->GetTriangleCount(sLod.m_anLeafActiveLods[1]);
    }

    return nTriangleCount;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumBranchTriangles definition

int CSpeedTreeRT::GetNumBranchTriangles(float fLodLevel)
{
    int nTriangleCount = 0;

    SLodValues sLod;
    GetLodValues(sLod, fLodLevel);

    if (m_pBranchGeometry && sLod.m_nBranchActiveLod > -1)
        nTriangleCount = m_pBranchGeometry->GetTriangleCount(sLod.m_nBranchActiveLod);

    return nTriangleCount;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumFrondTriangles definition

int CSpeedTreeRT::GetNumFrondTriangles(float fLodLevel)
{
    int nTriangleCount = 0;

    SLodValues sLod;
    GetLodValues(sLod, fLodLevel);

    if (m_pFrondGeometry && sLod.m_nFrondActiveLod > -1)
        nTriangleCount = m_pFrondGeometry->GetTriangleCount(sLod.m_nFrondActiveLod);

    return nTriangleCount;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeLeafStaticLighting definition

void CSpeedTreeRT::ComputeLeafStaticLighting(void)
{
    try
    {
        float afBoundingBox[6];
        GetBoundingBox(afBoundingBox);
        stVec3 cTreeCenter((afBoundingBox[0] + afBoundingBox[3]) * 0.5f,
                           (afBoundingBox[1] + afBoundingBox[4]) * 0.5f,
                           (afBoundingBox[2] + afBoundingBox[5]) * 0.5f);

        st_vector_leaves* pAllLeaves = m_pEngine->GetAllLeaves( );
        m_pLightingEngine->ComputeLeafStaticLighting(cTreeCenter, pAllLeaves, GetNumLeafLodLevels( ));
    }

    SpeedTreeCatch("CSpeedTreeRT::ComputeLeafStaticLighting")
    SpeedTreeCatchAll("CSpeedTreeRT::ComputeLeafStaticLighting");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::NotifyAllTreesOfEvent definition

void CSpeedTreeRT::NotifyAllTreesOfEvent(int nMessage)
{
    try
    {
        if (!g_pUniqueTrees)
            g_pUniqueTrees = new ("CSpeedTreeRT::CSpeedTreeRT, g_pUniqueTrees") st_vector_speedtree_p;

        for (st_vector_speedtree_p::iterator i = g_pUniqueTrees->begin( ); i != g_pUniqueTrees->end( ); ++i)
        {
            if (nMessage == STIE_CLIENT_CHANGED_WIND)
            {
                if ((*i)->GetBranchWindMethod( ) == WIND_CPU)
                    (*i)->m_pBranchGeometry->Invalidate( );

                if ((*i)->GetFrondWindMethod( ) == WIND_CPU)
                    (*i)->m_pFrondGeometry->Invalidate( );

                if ((*i)->GetLeafWindMethod( ) == WIND_CPU ||
                    (*i)->m_pWindEngine->GetLeafRockingState( ))
                    (*i)->m_pLeafGeometry->Invalidate( );
            }
            else if (nMessage == STIE_CLIENT_CHANGED_CAMERA) // client changed camera
            {
                (*i)->m_pLeafGeometry->Invalidate( );
                (*i)->m_pSimpleBillboard->Invalidate( );
            }
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::NotifyAllTreesOfEvent")
    SpeedTreeCatchAll("CSpeedTreeRT::NotifyAllTreesOfEvent");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetError definition

void CSpeedTreeRT::SetError(const char* pError)
{
    g_strError = pError;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseLodInfo definition

void CSpeedTreeRT::ParseLodInfo(CTreeFileAccess* pFile)
{
    int nToken = pFile->ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_LeafTransitionMethod:
            m_eLeafLodMethod = static_cast<ELodMethod>(pFile->ParseInt( ));
            break;
        case File_LeafTransitionRadius:
            m_fLeafLodTransitionRadius = pFile->ParseFloat();
            break;
        case File_LeafCurveExponent:
            m_fLeafLodCurveExponent = pFile->ParseFloat( );
            break;
        case File_LeafSizeIncreaseFactor:
            m_fLeafSizeIncreaseFactor = pFile->ParseFloat( );
            break;
        case File_BeginEngineLodInfo:
            m_pEngine->ParseLodInfo(*pFile);
            break;
        default:
            throw(IdvFileError("malformed lod info"));
        }

        if (pFile->EndOfFile( ))
            throw(IdvFileError("premature end of file reached parsing new lod info"));
        else
            nToken = pFile->ParseToken( );

    } while (nToken != File_EndLodInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseWindInfo definition

void CSpeedTreeRT::ParseWindInfo(CTreeFileAccess* pFile)
{
    int nToken = pFile->ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_WindLevel:
            m_pEngine->SetBranchLevelForWeighting(pFile->ParseInt( ));
            break;
        default:
            throw(IdvFileError("malformed new wind info"));
        }

        if (pFile->EndOfFile( ))
            throw(IdvFileError("premature end of file reached parsing new wind info"));
        else
            nToken = pFile->ParseToken( );

    } while (nToken != File_EndNewWindInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseTextureCoordInfo definition

void CSpeedTreeRT::ParseTextureCoordInfo(CTreeFileAccess* pFile)
{
    m_pEmbeddedTexCoords = new ("CSpeedTreeRT::ParseTextureCoordInfo, m_pEmbeddedTexCoords") SEmbeddedTexCoords;

    int nToken = pFile->ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_LeafTextureCoords:
        {
            m_pEmbeddedTexCoords->m_nNumLeafMaps = pFile->ParseInt( );
            if (m_pEmbeddedTexCoords->m_nNumLeafMaps > 0)
            {
                m_pEmbeddedTexCoords->m_pLeafTexCoords = st_new_array<float>(m_pEmbeddedTexCoords->m_nNumLeafMaps * 8, "CSpeedTreeRT::ParseTextureCoordInfo, m_pEmbeddedTexCoords->m_pLeafTexCoords");
                for (int i = 0; i < m_pEmbeddedTexCoords->m_nNumLeafMaps; ++i)
                    for (int j = 0; j < 8; ++j)
                        m_pEmbeddedTexCoords->m_pLeafTexCoords[i * 8 + j] = pFile->ParseFloat( );
            }
        }
        break;
        case File_BillboardTextureCoords:
        {
            m_pEmbeddedTexCoords->m_nNumBillboards = pFile->ParseInt( );
            if (m_pEmbeddedTexCoords->m_nNumBillboards > 0)
            {
                bool bTextureFlip = GetTextureFlip( );
                m_pEmbeddedTexCoords->m_pBillboardTexCoords = st_new_array<float>(m_pEmbeddedTexCoords->m_nNumBillboards * 8, "CSpeedTreeRT::ParseTextureCoordInfo, m_pEmbeddedTexCoords->m_pBillboardTexCoords");
                for (int i = 0; i < m_pEmbeddedTexCoords->m_nNumBillboards; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        m_pEmbeddedTexCoords->m_pBillboardTexCoords[i * 8 + j] = pFile->ParseFloat( );
                        if (j % 2 && bTextureFlip) // flip T coordinates if necessary
                            m_pEmbeddedTexCoords->m_pBillboardTexCoords[i * 8 + j] = -m_pEmbeddedTexCoords->m_pBillboardTexCoords[i * 8 + j];
                    }
                }
            }
        }
        break;
        case File_FrondTextureCoords:
        {
            m_pEmbeddedTexCoords->m_nNumFrondMaps = pFile->ParseInt( );
            if (m_pEmbeddedTexCoords->m_nNumFrondMaps > 0)
            {
                m_pEmbeddedTexCoords->m_pFrondTexCoords = st_new_array<float>(m_pEmbeddedTexCoords->m_nNumFrondMaps * 8, "CSpeedTreeRT::ParseTextureCoordInfo, m_pEmbeddedTexCoords->m_pFrondTexCoords");
                for (int i = 0; i < m_pEmbeddedTexCoords->m_nNumFrondMaps; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                        m_pEmbeddedTexCoords->m_pFrondTexCoords[i * 8 + j] = pFile->ParseFloat( );
                }
            }
        }
        break;
        case File_CompositeFilename:
        {
            pFile->ParseString(m_pEmbeddedTexCoords->m_strCompositeFilename);
            m_pEmbeddedTexCoords->m_strCompositeFilename = IdvNoPath(m_pEmbeddedTexCoords->m_strCompositeFilename);
        }
        break;
        case File_HorizontalBillboard:
        {
            m_bHorizontalBillboard = pFile->ParseBool( );
        }
        break;
        case File_360Billboard:
        {
            m_b360Billboard = pFile->ParseBool( );
        }
        break;
        default:
            throw(IdvFileError("malformed texture coord info"));
        }

        if (pFile->EndOfFile( ))
            throw(IdvFileError("premature end of file reached parsing texture coordinate info"));
        else
            nToken = pFile->ParseToken( );

    } while (nToken != File_EndTextureCoordInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetNumCollisionObjects definition

int CSpeedTreeRT::GetNumCollisionObjects(void)
{
    int nCount = 0;

    if (m_pCollisionObjects)
        nCount = int(m_pCollisionObjects->m_vObjects.size( ));

    return nCount;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetCollisionObject definition

void CSpeedTreeRT::GetCollisionObject(unsigned int nIndex, ECollisionObjectType& eType, float* pPosition, float* pDimensions, float* pEulerAngles)
{
    if (m_pCollisionObjects)
    {
        if (nIndex >= m_pCollisionObjects->m_vObjects.size( ))
            SetError(IdvFormatString("collision object index (%d) exceeds maximum index (%d)", nIndex, m_pCollisionObjects->m_vObjects.size( )).c_str( ));
        else
        {
            eType = m_pCollisionObjects->m_vObjects[nIndex].m_eType;

            if (pPosition)
                memcpy(pPosition, m_pCollisionObjects->m_vObjects[nIndex].m_afPosition, 3 * sizeof(float));

            if (pDimensions) 
                memcpy(pDimensions, m_pCollisionObjects->m_vObjects[nIndex].m_afDimensions, 3 * sizeof(float));

            if (pEulerAngles)
                memcpy(pEulerAngles, m_pCollisionObjects->m_vObjects[nIndex].m_afRotation, 3 * sizeof(float));
        }
    }
    else
        SetError("no collision objects are stored with this tree");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseCollisionObjects definition

void CSpeedTreeRT::ParseCollisionObjects(CTreeFileAccess* pFile)
{
    m_pCollisionObjects = new ("CSpeedTreeRT::ParseCollisionObjects, m_pCollisionObjects") SCollisionObjects;

    int nToken = pFile->ParseToken( );
    do
    {
        SShape sShape;
        switch (nToken)
        {
        case File_CollisionSphere:
            sShape.m_eType = CSpeedTreeRT::CO_SPHERE;
            break;
        case File_CollisionCapsule:
            sShape.m_eType = CSpeedTreeRT::CO_CAPSULE;
            break;
        case File_CollisionBox:
            sShape.m_eType = CSpeedTreeRT::CO_BOX;
            break;
        default:
            throw(IdvFileError("malformed collision object info"));
        }

        float afPosition[3];
        afPosition[0] = pFile->ParseFloat( );
        afPosition[1] = pFile->ParseFloat( );
        afPosition[2] = pFile->ParseFloat( );
        Assign3d(sShape.m_afPosition, afPosition);

        switch (sShape.m_eType)
        {
        case CSpeedTreeRT::CO_SPHERE:
            sShape.m_afDimensions[0] = pFile->ParseFloat( );    // radius
            break;
        case CSpeedTreeRT::CO_CAPSULE:
            sShape.m_afDimensions[0] = pFile->ParseFloat( );    // radius
            sShape.m_afDimensions[1] = pFile->ParseFloat( );    // height
            break;
        case CSpeedTreeRT::CO_BOX:
            sShape.m_afDimensions[0] = pFile->ParseFloat( );    // x
            sShape.m_afDimensions[1] = pFile->ParseFloat( );    // y
            sShape.m_afDimensions[2] = pFile->ParseFloat( );    // z
            break;
        default:
            throw(IdvFileError("unknown collision object type"));
        }

        m_pCollisionObjects->m_vObjects.push_back(sShape);

        if (pFile->EndOfFile( ))
            throw(IdvFileError("premature end of file reached parsing collision object info"));
        else
            nToken = pFile->ParseToken( );

    } while (nToken != File_EndCollisionInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveTextureCoords definition

void CSpeedTreeRT::SaveTextureCoords(CTreeFileAccess* pFile) const
{
    if (m_pEmbeddedTexCoords)
    {
        pFile->SaveToken(File_BeginTextureCoordInfo);

        // setup for flip if necessary
        float fFlip = GetTextureFlip( ) ? -1.0f : 1.0f;

        // leaves
        pFile->SaveToken(File_LeafTextureCoords);
        pFile->SaveInt(m_pEmbeddedTexCoords->m_nNumLeafMaps);
        int i = 0; // declared outside of the for-scope for portability
        for (i = 0; i < m_pEmbeddedTexCoords->m_nNumLeafMaps; ++i)
            for (int j = 0; j < 8; ++j)
                pFile->SaveFloat(m_pEmbeddedTexCoords->m_pLeafTexCoords[i * 8 + j]);

        // billboards
        pFile->SaveToken(File_BillboardTextureCoords);
        pFile->SaveInt(m_pEmbeddedTexCoords->m_nNumBillboards);
        for (i = 0; i < m_pEmbeddedTexCoords->m_nNumBillboards; ++i)
            for (int k = 0; k < 8; ++k)
                if (k % 2)
                    pFile->SaveFloat(fFlip * m_pEmbeddedTexCoords->m_pBillboardTexCoords[i * 8 + k]);
                else
                    pFile->SaveFloat(m_pEmbeddedTexCoords->m_pBillboardTexCoords[i * 8 + k]);

        // fronds
        pFile->SaveToken(File_FrondTextureCoords);
        pFile->SaveInt(m_pEmbeddedTexCoords->m_nNumFrondMaps);
        for (i = 0; i < m_pEmbeddedTexCoords->m_nNumFrondMaps; ++i)
            for (int l = 0; l < 8; ++l)
                pFile->SaveFloat(m_pEmbeddedTexCoords->m_pFrondTexCoords[i * 8 + l]);

        pFile->SaveToken(File_EndTextureCoordInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveCollisionObjects definition

void CSpeedTreeRT::SaveCollisionObjects(CTreeFileAccess* pFile) const
{
    if (m_pCollisionObjects)
    {
        pFile->SaveToken(File_BeginCollisionInfo);
        for (unsigned int i = 0; i < m_pCollisionObjects->m_vObjects.size( ); ++i)
        {
            SShape& sShape = m_pCollisionObjects->m_vObjects[i];
            switch (sShape.m_eType)
            {
            case CSpeedTreeRT::CO_SPHERE:
                pFile->SaveToken(File_CollisionSphere);
                break;
            case CSpeedTreeRT::CO_CAPSULE:
                pFile->SaveToken(File_CollisionCapsule);
                break;
            case CSpeedTreeRT::CO_BOX:
                pFile->SaveToken(File_CollisionBox);
                break;
            }

            pFile->SaveFloat(sShape.m_afPosition[0]);
            pFile->SaveFloat(sShape.m_afPosition[1]);
            pFile->SaveFloat(sShape.m_afPosition[2]);

            switch (sShape.m_eType)
            {
            case CSpeedTreeRT::CO_SPHERE:
                pFile->SaveFloat(sShape.m_afDimensions[0]);  // radius
                break;
            case CSpeedTreeRT::CO_CAPSULE:
                pFile->SaveFloat(sShape.m_afDimensions[0]);  // radius
                pFile->SaveFloat(sShape.m_afDimensions[1]);  // height
                break;
            case CSpeedTreeRT::CO_BOX:
                pFile->SaveFloat(sShape.m_afDimensions[0]);  // x
                pFile->SaveFloat(sShape.m_afDimensions[1]);  // y
                pFile->SaveFloat(sShape.m_afDimensions[2]);  // z
                break;
            }
        }
        pFile->SaveToken(File_EndCollisionInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::DeleteFrondGeometry definition

void CSpeedTreeRT::DeleteFrondGeometry(void)
{
    try
    {
        if (m_bTreeComputed &&
            m_pFrondGeometry &&
            !m_pInstanceData &&
            m_pInstanceRefCount &&
            *m_pInstanceRefCount == 1)
        {
            st_delete<CIndexedGeometry>(m_pFrondGeometry, "CSpeedTreeRT::DeleteFrondGeometry, m_pFrondGeometry");

            if (GetFrondWindMethod( ) != WIND_CPU)
                st_delete<CFrondEngine>(m_pFrondEngine, "CSpeedTreeRT::DeleteFrondGeometry, m_pFrondEngine");
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::DeleteFrondGeometry")
    SpeedTreeCatchAll("CSpeedTreeRT::DeleteFrondGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::DeleteLeafGeometry definition

void CSpeedTreeRT::DeleteLeafGeometry(void)
{
    try
    {
        if (m_pLeafGeometry &&
m_bTreeComputed &&
            !m_pInstanceData &&
            m_pInstanceRefCount &&
            *m_pInstanceRefCount == 1)
        {
            if (GetLeafWindMethod( ) != WIND_CPU)
            {
                m_pLeafGeometry->DeleteGeometry( );
            }
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::DeleteLeafGeometry")
    SpeedTreeCatchAll("CSpeedTreeRT::DeleteLeafGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetFrondGeometryMapIndexes definition

unsigned char* CSpeedTreeRT::GetFrondGeometryMapIndexes(int nLodLevel) const
{
    unsigned char* pMapIndexes = NULL;
    try
    {
        if (m_pFrondGeometry)
        {
            int nNumFrondStrips = m_pFrondGeometry->GetNumStrips(nLodLevel);
            if (nNumFrondStrips > 0)
            {
                pMapIndexes = st_new_array<unsigned char>(nNumFrondStrips, "CSpeedTreeRT::GetFrondGeometryMapIndexes, pMapIndexes");

                int nNumStrips = 0;
                const int* pStripInfo = m_pFrondGeometry->GetStripInfo(nLodLevel, nNumStrips);
                st_assert(nNumFrondStrips == nNumFrondStrips);
                const int* pAllStrips = m_pFrondGeometry->GetStrips(nLodLevel);
                const unsigned char* pTexIndices = m_pFrondGeometry->GetVertexTexIndices( );

                for (int i = 0; i < nNumStrips; ++i)
                {
                    const int nStripStart = pStripInfo[i * 2];
                    const int nStripLength = pStripInfo[i * 2 + 1];
                    const int* pActiveStrip = pAllStrips + nStripStart;

                    if (nStripLength == 0)
                        pMapIndexes[i] = 0;
                    else
                        pMapIndexes[i] = pTexIndices[pActiveStrip[0]];
                }
            }
        }
    }

    SpeedTreeCatch("CSpeedTreeRT::GetFrondGeometryMapIndexes( )")
    SpeedTreeCatchAll("CSpeedTreeRT::GetFrondGeometryMapIndexes( )");

    return pMapIndexes;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetGeometry definition

void CSpeedTreeRT::GetGeometry(SGeometry& sGeometry, unsigned int uiBitVector)
{
    st_assert(m_pEngine);

    try
    {
        if (uiBitVector & SpeedTree_BranchGeometry)
            GetBranchGeometry(sGeometry);
        if (uiBitVector & SpeedTree_FrondGeometry)
            GetFrondGeometry(sGeometry);
        if (uiBitVector & SpeedTree_LeafGeometry)
            GetLeafGeometry(sGeometry);
        if (uiBitVector & SpeedTree_BillboardGeometry)
            GetBillboardGeometry(sGeometry);
    }

    SpeedTreeCatch("CSpeedTreeRT::GetGeometry")
    SpeedTreeCatchAll("CSpeedTreeRT::GetGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::UpdateLeafCards definition

void CSpeedTreeRT::UpdateLeafCards(SGeometry& sGeometry)
{
    if (m_pLeafGeometry)
    {
        sGeometry.m_pLeaves = m_pLeafLods;
        sGeometry.m_nNumLeafLods = GetNumLeafLodLevels( );

        float fAzimuth = 0.0f;
        float fPitch = 0.0f;
        GetCameraAngles(fAzimuth, fPitch);
        for (int nLod = 0; nLod < sGeometry.m_nNumLeafLods; ++nLod)
            m_pLeafGeometry->Update(sGeometry.m_pLeaves[nLod], nLod, fAzimuth, fPitch, m_fLeafSizeIncreaseFactor);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::UpdateBillboardGeometry definition

void CSpeedTreeRT::UpdateBillboardGeometry(SGeometry& sGeometry)
{
    SGeometry::S360Billboard& sBB = sGeometry.m_s360Billboard;

    if (m_pEmbeddedTexCoords && m_bDropToBillboard)
    {
        // find appropriate coordinates
        st_assert(m_pSimpleBillboard);
#ifdef UPVECTOR_POS_Z
        m_pSimpleBillboard->GetTransformedBillboard(m_pTreeSizes[STS_BB_SIZE], m_pTreeSizes[STS_MAX_BOX + 2], sBB);
#elif defined UPVECTOR_NEG_Z
        m_pSimpleBillboard->GetTransformedBillboard(m_pTreeSizes[STS_BB_SIZE], fabs(m_pTreeSizes[STS_MIN_BOX + 2]), sBB);
#else
        m_pSimpleBillboard->GetTransformedBillboard(m_pTreeSizes[STS_BB_SIZE], fabs(m_pTreeSizes[STS_MAX_BOX + 1]), sBB);
#endif

        // compute fade out value
        const float c_fOneMinusTransitionFactor = 1.0f - m_fLeafTransitionFactor;
        const float c_fTransitionWidth = 2.0f * m_fLeafLodTransitionRadius;
        const float c_fShortenedTransitionWidth = c_fTransitionWidth * c_fOneMinusTransitionFactor;
        float fFadeOut = GetLodLevel( ) / c_fShortenedTransitionWidth;

        if (fFadeOut < 1.0f)
        {
            // determine texcoords
            const int c_nNumImages = m_pEmbeddedTexCoords->m_nNumBillboards - (m_bHorizontalBillboard ? 1 : 0);
            if (c_nNumImages < 2)
            {
                sBB.m_pTexCoords[0] = m_pEmbeddedTexCoords->m_pBillboardTexCoords;
                sBB.m_pTexCoords[1] = NULL;
                sBB.m_afAlphaTestValues[0] = VecInterpolate(m_fTargetAlphaValue, c_fClear, fFadeOut);
                sBB.m_afAlphaTestValues[1] = c_fClear;
            }
            else
            {
                float fAzimuth = m_fCameraAzimuth + 180.0f; // adjust for 1.8 and earlier compatibility
                if (fAzimuth < 0.0f)
                    fAzimuth += 360.0f;
                if (fAzimuth > 360.0f)
                    fAzimuth -= 360.0f;
                const float c_fSliceDiameter = 360.0f / c_nNumImages;

                // billboard 0
                int nIndex0 = int(fAzimuth / c_fSliceDiameter) + 1;
                float fSliceAngle0 = nIndex0 * c_fSliceDiameter;
                if (nIndex0 > c_nNumImages - 1)
                    nIndex0 = 0;

                float fPercent0 = fabs((fAzimuth - fSliceAngle0) / c_fSliceDiameter);
                sBB.m_afAlphaTestValues[0] = ComputeLodCurveBB(m_fTargetAlphaValue, c_fClear, fPercent0);
                sBB.m_afAlphaTestValues[0] = VecInterpolate(sBB.m_afAlphaTestValues[0], c_fClear, fFadeOut);
                sBB.m_pTexCoords[0] = m_pEmbeddedTexCoords->m_pBillboardTexCoords + nIndex0 * 8;

                // billboard 1
                int nIndex1 = int(fAzimuth / c_fSliceDiameter);
                float fSliceAngle1 = nIndex1 * c_fSliceDiameter;
                if (nIndex1 > c_nNumImages - 1)
                    nIndex1 = 0;

                float fPercent1 = fabs((fAzimuth - fSliceAngle1) / c_fSliceDiameter);
                sBB.m_afAlphaTestValues[1] = ComputeLodCurveBB(m_fTargetAlphaValue, c_fClear, fPercent1);
                sBB.m_afAlphaTestValues[1] = VecInterpolate(sBB.m_afAlphaTestValues[1], c_fClear, fFadeOut);
                sBB.m_pTexCoords[1] = m_pEmbeddedTexCoords->m_pBillboardTexCoords + nIndex1 * 8;
            }

            SGeometry::SHorzBillboard& sHBB = sGeometry.m_sHorzBillboard;
            if (m_bHorizontalBillboard)
            {
                // always the plane that bisects the bounding box (computed once at initialization)
                sHBB.m_pCoords = m_afHorizontalCoords;

                // the horizontal billboard is always the last billboard in the billboard sequence
                sHBB.m_pTexCoords = m_pEmbeddedTexCoords->m_pBillboardTexCoords + (m_pEmbeddedTexCoords->m_nNumBillboards - 1) * 8;
                sHBB.m_fAlphaTestValue = VecInterpolate(c_fClear, m_fTargetAlphaValue, m_fHorizontalFadeValue);
                sHBB.m_fAlphaTestValue = VecInterpolate(sHBB.m_fAlphaTestValue, c_fClear, fFadeOut);
            }
            else
                sHBB.m_fAlphaTestValue = c_fClear;
        }
        else
        {
            sGeometry.m_s360Billboard.m_afAlphaTestValues[0] = sGeometry.m_s360Billboard.m_afAlphaTestValues[1] = c_fClear;
            sGeometry.m_sHorzBillboard.m_fAlphaTestValue = c_fClear;
        }
        sBB.m_fTransitionPercent = m_fBillboardTransition;
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::UpdateBillboardLighting definition

void CSpeedTreeRT::UpdateBillboardLighting(SGeometry::S360Billboard& sBillboard)
{
    CSimpleBillboard::GetLightingAttributes(sBillboard);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBranchGeometry definition

void CSpeedTreeRT::GetBranchGeometry(SGeometry& sGeometry)
{
    if (m_pBranchGeometry)
    {
        SGeometry::SIndexed& sBranches = sGeometry.m_sBranches;

        // assign vertex attributes
        sBranches.m_nNumVertices = m_pBranchGeometry->GetVertexCount( );
        sBranches.m_pColors = m_pBranchGeometry->GetVertexColors( );
        sBranches.m_pCoords = m_pBranchGeometry->GetVertexCoords( );
        sBranches.m_pNormals = m_pBranchGeometry->GetVertexNormals( );
        sBranches.m_pBinormals = m_pBranchGeometry->GetVertexBinormals( );
        sBranches.m_pTangents = m_pBranchGeometry->GetVertexTangents( );
        sBranches.m_pWindWeights[0] = m_pBranchGeometry->GetVertexWindWeights1( );
        sBranches.m_pWindMatrixIndices[0] = m_pBranchGeometry->GetVertexWindMatrixIndices1( );
        sBranches.m_pWindWeights[1] = m_pBranchGeometry->GetVertexWindWeights2( );
        sBranches.m_pWindMatrixIndices[1] = m_pBranchGeometry->GetVertexWindMatrixIndices2( );
        sBranches.m_pLodFadeHints = m_pBranchGeometry->GetLodFadeHints( );
        for (int i = 0; i < int(TL_NUM_TEX_LAYERS); ++i)
            sBranches.m_pTexCoords[i] = m_pBranchGeometry->GetVertexTexCoords(static_cast<ETextureLayers>(i));

        // assign the indexes
        sBranches.m_pNumStrips = m_pBranchGeometry->GetNumStripsPointer( );
        sBranches.m_pStripLengths = m_pBranchGeometry->GetStripLengthsPointer( );
        sBranches.m_pStrips = m_pBranchGeometry->GetStripsPointer( );

        sBranches.m_nNumLods = GetNumBranchLodLevels( );
        sBranches.m_fLodFadeDistance = m_fBranchLodFadeDistance;
    }
    else
        SetError("no branch geometry exists, possible prior call to DeleteBranchGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetFrondGeometry definition

void CSpeedTreeRT::GetFrondGeometry(SGeometry& sGeometry)
{
    if (m_pFrondGeometry)
    {
        SGeometry::SIndexed& sFronds = sGeometry.m_sFronds;

        // get vertex attributes
        sFronds.m_nNumVertices = m_pFrondGeometry->GetVertexCount( );
        sFronds.m_pColors = m_pFrondGeometry->GetVertexColors( );
        sFronds.m_pCoords = m_pFrondGeometry->GetVertexCoords( );
        sFronds.m_pNormals = m_pFrondGeometry->GetVertexNormals( );
        sFronds.m_pBinormals = m_pFrondGeometry->GetVertexBinormals( );
        sFronds.m_pTangents = m_pFrondGeometry->GetVertexTangents( );
        sFronds.m_pWindWeights[0] = m_pFrondGeometry->GetVertexWindWeights1( );
        sFronds.m_pWindMatrixIndices[0] = m_pFrondGeometry->GetVertexWindMatrixIndices1( );
        sFronds.m_pWindWeights[1] = m_pFrondGeometry->GetVertexWindWeights2( );
        sFronds.m_pWindMatrixIndices[1] = m_pFrondGeometry->GetVertexWindMatrixIndices2( );
        sFronds.m_pLodFadeHints = m_pFrondGeometry->GetLodFadeHints( );
        for (int i = 0; i < int(TL_NUM_TEX_LAYERS); ++i)
            sFronds.m_pTexCoords[i] = m_pFrondGeometry->GetVertexTexCoords(static_cast<ETextureLayers>(i));

        // assign the indexes
        sFronds.m_pNumStrips = m_pFrondGeometry->GetNumStripsPointer( );
        sFronds.m_pStripLengths = m_pFrondGeometry->GetStripLengthsPointer( );
        sFronds.m_pStrips = m_pFrondGeometry->GetStripsPointer( );

        sFronds.m_nNumLods = GetNumFrondLodLevels( );
        sFronds.m_fLodFadeDistance = m_fFrondLodFadeDistance;
    }
    else
        SetError("no frond geometry exists, possible prior call to DeleteFrondGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLeafGeometry definition

void CSpeedTreeRT::GetLeafGeometry(SGeometry& sGeometry)
{
    if (m_pLeafGeometry)
    {
        sGeometry.m_nNumLeafLods = GetNumLeafLodLevels( );

        // leaf cards
        sGeometry.m_pLeaves = m_pLeafLods;
        for (int nLod = 0; nLod < sGeometry.m_nNumLeafLods; ++nLod)
        {
            // azimuth and pitch parameters ignored (zeroes here)
            m_pLeafGeometry->Update(sGeometry.m_pLeaves[nLod], nLod, 0.0f, 0.0f, m_fLeafSizeIncreaseFactor);
        }
    }
    else
        SetError("no leaf geometry exists, possible prior call to DeleteLeafGeometry");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetBillboardGeometry definition

void CSpeedTreeRT::GetBillboardGeometry(SGeometry& sGeometry)
{
    SGeometry::S360Billboard& sBB = sGeometry.m_s360Billboard;

    // up-axis-adjusted width and height
#ifdef UPVECTOR_POS_Z
    sBB.m_fWidth = m_pTreeSizes[STS_BB_SIZE];
    sBB.m_fHeight = m_pTreeSizes[STS_MAX_BOX + 2];
#elif defined UPVECTOR_NEG_Z
    sBB.m_fWidth = m_pTreeSizes[STS_BB_SIZE];
    sBB.m_fHeight = fabs(m_pTreeSizes[STS_MAX_BOX + 2]);
#else
    sBB.m_fWidth = m_pTreeSizes[STS_BB_SIZE];
    sBB.m_fHeight = fabs(fabs(m_pTreeSizes[STS_MAX_BOX + 1]));
#endif

    if (m_pEmbeddedTexCoords)
    {
        // 360-degree images
        sBB.m_nNumImages = m_pEmbeddedTexCoords->m_nNumBillboards - (m_bHorizontalBillboard ? 1 : 0);
        sBB.m_pTexCoordTable = m_pEmbeddedTexCoords->m_pBillboardTexCoords;
    }

    // fill CPU-only stuff so it's not accidentally used
    sBB.m_pCoords = NULL;
    sBB.m_pTexCoords[0] = sBB.m_pTexCoords[1] = NULL;
    sBB.m_afAlphaTestValues[0] = sBB.m_afAlphaTestValues[1] = c_fClear;

    // set transition percentage to whatever value was assigned in CAD
    sBB.m_fTransitionPercent = m_fBillboardTransition;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseShadowProjectionInfo definition

void CSpeedTreeRT::ParseShadowProjectionInfo(CTreeFileAccess* pFile)
{
    m_pProjectedShadow = new ("CSpeedTreeRT::ParseShadowProjectionInfo, m_pProjectedShadow") CProjectedShadow;
    m_pProjectedShadow->Parse(*pFile);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeSelfShadowTexCoords definition

void CSpeedTreeRT::ComputeSelfShadowTexCoords(void)
{
    if (m_pBranchGeometry &&
        m_pFrondGeometry &&
        m_pProjectedShadow)
    {
        // compute the radius of the circumscribing sphere
        stVec3 cMin(m_pTreeSizes[0], m_pTreeSizes[1], m_pTreeSizes[2]), cMax(m_pTreeSizes[3], m_pTreeSizes[4], m_pTreeSizes[5]);
        float fRadius = cMin.Distance(cMax) * 0.5f;

        // compute the center
        stVec3 cCenter = (cMin + cMax) * 0.5f;

        if (m_pEmbeddedTexCoords)
        {
            // compute branch texcoords
            m_pProjectedShadow->ComputeTexCoords(m_pBranchGeometry, cCenter, fRadius, m_pEmbeddedTexCoords->m_afShadowTexCoords);

            // compute frond texcoords
            m_pProjectedShadow->ComputeTexCoords(m_pFrondGeometry, cCenter, fRadius, m_pEmbeddedTexCoords->m_afShadowTexCoords);
        }
        else
        {
            // compute branch texcoords
            m_pProjectedShadow->ComputeTexCoords(m_pBranchGeometry, cCenter, fRadius);

            // compute frond texcoords
            m_pProjectedShadow->ComputeTexCoords(m_pFrondGeometry, cCenter, fRadius);
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafTextureCoords definition

void CSpeedTreeRT::SetLeafTextureCoords(unsigned int nLeafMapIndex, const float* pTexCoords)
{
    st_assert(m_pEngine);
    st_assert(nLeafMapIndex >= 0);
    st_assert(pTexCoords);
    st_assert(m_pLeafGeometry);

    try
    {
        st_assert(m_pLeafGeometry);
        st_assert(nLeafMapIndex < m_pEngine->GetLeafTextures( ).size( ));
        m_pLeafGeometry->SetTextureCoords(nLeafMapIndex, pTexCoords);
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafTextureCoords")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafTextureCoords");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetFrondTextureCoords definition

void CSpeedTreeRT::SetFrondTextureCoords(unsigned int nFrondMapIndex, const float* pTexCoords)
{
    try
    {
        if (m_pFrondGeometry &&
            pTexCoords)
            m_pFrondEngine->SetTextureCoords(m_pFrondGeometry, nFrondMapIndex, pTexCoords, GetTextureFlip( ));
    }

    SpeedTreeCatch("CSpeedTreeRT::SetFrondTextureCoords")
    SpeedTreeCatchAll("CSpeedTreeRT::SetFrondTextureCoords");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveUserData definition

void CSpeedTreeRT::SaveUserData(CTreeFileAccess* pFile) const
{
    if (m_pUserData)
    {
        pFile->SaveToken(File_BeginUserData);

        pFile->SaveToken(File_UserData);
        pFile->SaveString(st_string(m_pUserData));

        pFile->SaveToken(File_EndUserData);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseUserData definition

void CSpeedTreeRT::ParseUserData(CTreeFileAccess* pFile)
{
    int nToken = pFile->ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_UserData:
            st_string strUserData;
            pFile->ParseString(strUserData);
            m_pUserData = CopyUserData(strUserData.c_str( ));
            break;
        }
        nToken = pFile->ParseToken( );

    } while (nToken != File_EndUserData);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetUserData definition

const char* CSpeedTreeRT::GetUserData(void) const
{
    return m_pUserData;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetupHorizontalBillboard definition

void CSpeedTreeRT::SetupHorizontalBillboard(void)
{
    if (m_pEmbeddedTexCoords && m_bHorizontalBillboard)
    {
        float afMin[3];
        Assign3d(afMin, m_pTreeSizes + 0);
        float afMax[3];
        Assign3d(afMax, m_pTreeSizes + 3);

        float fHalfHeight = (afMin[2] + afMax[2]) * 0.5f;

        // ccw from max x, max y to match the stored texcoords
        Assign3d(m_afHorizontalCoords + 0, stVec3(afMax[0], afMax[1], fHalfHeight));
        Assign3d(m_afHorizontalCoords + 3, stVec3(afMin[0], afMax[1], fHalfHeight));
        Assign3d(m_afHorizontalCoords + 6, stVec3(afMin[0], afMin[1], fHalfHeight));
        Assign3d(m_afHorizontalCoords + 9, stVec3(afMax[0], afMin[1], fHalfHeight));
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetTextureFlip definition

bool CSpeedTreeRT::GetTextureFlip(void)
{
    return m_bTextureFlip;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetTextureFlip definition

void CSpeedTreeRT::SetTextureFlip(bool bFlag)
{
    m_bTextureFlip = bFlag;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::CopyUserData definition
//
//  unsigned int uiSize does NOT include the null terminator

char* CSpeedTreeRT::CopyUserData(const char* pOrig)
{
    char* pCopy = NULL;

    if (pOrig)
    {
        int nLength = int(strlen(pOrig));
        if (nLength > 0)
        {
            pCopy = st_new_array<char>(nLength + 1, "CSpeedTreeRT::CopyUserData, pCopy");
            memcpy(pCopy, pOrig, nLength + 1);
        }
    }

    return pCopy;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetBranchTextureFilename definition

void CSpeedTreeRT::SetBranchTextureFilename(const char* pFilename)
{
    st_assert(m_pEngine);

    try
    {
        if (m_pEngine->TransientDataIntact( ))
            m_pEngine->SetBranchTextureFilename(pFilename);
        else
            SetError("SetBranchTextureFilename() has no effect after DeleteTransientData() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetBranchTextureFilename")
    SpeedTreeCatchAll("CSpeedTreeRT::SetBranchTextureFilename");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetLeafTextureFilename definition

void CSpeedTreeRT::SetLeafTextureFilename(unsigned int nLeafMapIndex, const char* pFilename)
{
    st_assert(m_pEngine);

    try
    {
        if (m_pEngine->TransientDataIntact( ))
            m_pEngine->SetLeafTextureFilename(nLeafMapIndex, pFilename);
        else
            SetError("SetLeafTextureFilename() has no effect after DeleteTransientData() has been called");
    }

    SpeedTreeCatch("CSpeedTreeRT::SetLeafTextureFilename")
    SpeedTreeCatchAll("CSpeedTreeRT::SetLeafTextureFilename");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetFrondTextureFilename definition

void CSpeedTreeRT::SetFrondTextureFilename(unsigned int nFrondMapIndex, const char* pFilename)
{
    st_assert(m_pFrondEngine);

    try
    {
        m_pFrondEngine->SetTextureFilename(nFrondMapIndex, pFilename);
    }

    SpeedTreeCatch("CSpeedTreeRT::SetFrondTextureFilename")
    SpeedTreeCatchAll("CSpeedTreeRT::SetFrondTextureFilename");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveSupplementalTexCoordInfo definition

void CSpeedTreeRT::SaveSupplementalTexCoordInfo(CTreeFileAccess* pFile) const
{
    if (m_pEmbeddedTexCoords)
    {
        pFile->SaveToken(File_BeginSupplementalTexCoordInfo);

        pFile->SaveToken(File_SupplementalCompositeFilename);
        pFile->SaveString(m_pEmbeddedTexCoords->m_strCompositeFilename);

        pFile->SaveToken(File_SupplementalHorizontalBillboard);
        pFile->SaveBool(m_bHorizontalBillboard);

        pFile->SaveToken(File_Supplemental360Billboard);
        pFile->SaveBool(m_b360Billboard);

        pFile->SaveToken(File_SupplementalShadowTexCoords);
        for (int i = 0; i < 8; ++i)
            pFile->SaveFloat(m_pEmbeddedTexCoords->m_afShadowTexCoords[i]);

        pFile->SaveToken(File_EndSupplementalTexCoordInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseSupplementalTexCoordInfo definition

void CSpeedTreeRT::ParseSupplementalTexCoordInfo(CTreeFileAccess* pFile)
{
    if (m_pEmbeddedTexCoords)
    {
        int nToken = pFile->ParseToken( );
        do
        {
            switch (nToken)
            {
            case File_SupplementalCompositeFilename:
            {
                pFile->ParseString(m_pEmbeddedTexCoords->m_strCompositeFilename);
                m_pEmbeddedTexCoords->m_strCompositeFilename = IdvNoPath(m_pEmbeddedTexCoords->m_strCompositeFilename);
            }
            break;
            case File_SupplementalHorizontalBillboard:
            {
                m_bHorizontalBillboard = pFile->ParseBool( );
            }
            break;
            case File_Supplemental360Billboard:
            {
                m_b360Billboard = pFile->ParseBool( );
            }
            break;
            case File_SupplementalShadowTexCoords:
            {
                for (int i = 0; i < 8; ++i)
                    m_pEmbeddedTexCoords->m_afShadowTexCoords[i] = pFile->ParseFloat( );
            }
            break;
            default:
                throw(IdvFileError("malformed texture coord info"));
            }

            if (pFile->EndOfFile( ))
                throw(IdvFileError("premature end of file reached parsing texture coordinate info"));
            else
                nToken = pFile->ParseToken( );

        } while (nToken != File_EndSupplementalTexCoordInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetHorzBillboardFadeAngles definition

void CSpeedTreeRT::SetHorzBillboardFadeAngles(float fStart, float fEnd)
{
    m_fHorizontalFadeStartAngle = fStart;
    m_fHorizontalFadeEndAngle = fEnd;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetHorzBillboardFadeAngles definition

void CSpeedTreeRT::GetHorzBillboardFadeAngles(float& fStart, float& fEnd)
{
    fStart = m_fHorizontalFadeStartAngle;
    fEnd = m_fHorizontalFadeEndAngle;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetHorzBillboardFadeValue definition

float CSpeedTreeRT::GetHorzBillboardFadeValue(void)
{
    return m_fHorizontalFadeValue;
}


///////////////////////////////////////////////////////////////////////
//  function st_fmod
//
//  Float-based faster version of fmod (4-5X fast on 3.2 GHz P4)
//
//  assumes x > 0, y > 0

inline float st_fmod(float x, float y)
{
    return x - int(x/y) * y;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeLodCurve definition

inline float CSpeedTreeRT::ComputeLodCurve(float fStart, float fEnd, float fPercent, bool bConcaveUp)
{
    if (bConcaveUp)
        return VecInterpolate(fEnd, fStart, powf(1.0f - fPercent, m_fLeafLodCurveExponent));
    else
        return VecInterpolate(fStart, fEnd, powf(fPercent, m_fLeafLodCurveExponent));
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ComputeLodCurveBB definition

inline float CSpeedTreeRT::ComputeLodCurveBB(float fStart, float fEnd, float fPercent)
{
    fPercent = st_max(fPercent, m_fLeafTransitionFactor);

    return VecInterpolate(fStart, fEnd, (powf((fPercent - m_fBillboardTransition) / (1.0f - m_fLeafTransitionFactor), m_fLeafLodCurveExponent)));
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLightShaderParams definition

void CSpeedTreeRT::GetLightShaderParams(SLightShaderParams& sParams) const
{
    sParams.m_fGlobalLightScalar = m_fGlobalLightScalar;
    sParams.m_fBranchLightScalar = m_fBranchLightScalar;
    sParams.m_fFrondLightScalar = m_fFrondLightScalar;
    sParams.m_fLeafLightScalar = m_fLeafLightScalar;
    sParams.m_fBillboardDarkSideLightScalar = m_fBillboardDarkSideLightScalar;
    sParams.m_fBillboardBrightSideLightScalar = m_fBillboardBrightSideLightScalar;
    sParams.m_fAmbientScalar = m_fAmbientScalar;
    sParams.m_fBillboardAmbientScalar = m_fBillboardAmbientScalar;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveStandardShaderInfo definition

void CSpeedTreeRT::SaveStandardShaderInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginStandardShaderInfo);

    cFile.SaveToken(File_BranchLightScalar);
    cFile.SaveFloat(m_fBranchLightScalar);

    cFile.SaveToken(File_FrondLightScalar);
    cFile.SaveFloat(m_fFrondLightScalar);

    cFile.SaveToken(File_LeafLightScalar);
    cFile.SaveFloat(m_fLeafLightScalar);

    cFile.SaveToken(File_GlobalLightScalar);
    cFile.SaveFloat(m_fGlobalLightScalar);

    cFile.SaveToken(File_AmbientScalar);
    cFile.SaveFloat(m_fAmbientScalar);

    cFile.SaveToken(File_BillboardDarkSideLightScalar);
    cFile.SaveFloat(m_fBillboardDarkSideLightScalar);

    cFile.SaveToken(File_BillboardAmbientScalar);
    cFile.SaveFloat(m_fBillboardAmbientScalar);

    cFile.SaveToken(File_BillboardBrightSideLightScalar);
    cFile.SaveFloat(m_fBillboardBrightSideLightScalar);

    cFile.SaveToken(File_EndStandardShaderInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseStandardShaderInfo definition

void CSpeedTreeRT::ParseStandardShaderInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_BranchLightScalar:
            m_fBranchLightScalar = cFile.ParseFloat( );
            break;
        case File_FrondLightScalar:
            m_fFrondLightScalar = cFile.ParseFloat( );
            break;
        case File_LeafLightScalar:
            m_fLeafLightScalar = cFile.ParseFloat( );
            break;
        case File_GlobalLightScalar:
            m_fGlobalLightScalar = cFile.ParseFloat( );
            break;
        case File_AmbientScalar:
            m_fAmbientScalar = cFile.ParseFloat( );
            break;
        case File_BillboardDarkSideLightScalar:
            m_fBillboardDarkSideLightScalar = cFile.ParseFloat( );
            break;
        case File_BillboardAmbientScalar:
            m_fBillboardAmbientScalar = cFile.ParseFloat( );
            break;
        case File_BillboardBrightSideLightScalar:
            m_fBillboardBrightSideLightScalar = cFile.ParseFloat( );
            break;
       default:
            throw(IdvFileError("malformed standard shader support info"));
        }

        if (cFile.EndOfFile( ))
            throw(IdvFileError("premature end of file reached parsing standard shader support info"));
        else
            nToken = cFile.ParseToken( );

    } while (nToken != File_EndStandardShaderInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetAllocator definition

void CSpeedTreeRT::SetAllocator(CSpeedTreeAllocator* pAllocator)
{
    g_pAllocator = pAllocator;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetMemoryUsage definition

size_t CSpeedTreeRT::GetMemoryUsage(void)
{
    return g_sMemoryUsed;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetLodValues definition

void CSpeedTreeRT::GetLodValues(SLodValues& sLodValues, float fLodLevel)
{
    const float c_fOneMinusTransitionFactor = 1.0f - m_fLeafTransitionFactor;
    const float c_fTransitionWidth = 2.0f * m_fLeafLodTransitionRadius;
    const float c_fShortenedTransitionWidth = c_fTransitionWidth * c_fOneMinusTransitionFactor;

    // query current LOD value
    if (fLodLevel == -1.0f)
        fLodLevel = GetLodLevel( );
    const float fOppositeLodValue = 1.0f - fLodLevel;

    // branches and fronds
    {
        // is billboard fully active?
        if (fLodLevel <= 0.0f)
        {
            if (m_bDropToBillboard)
            {
                sLodValues.m_nBranchActiveLod = sLodValues.m_nFrondActiveLod = -1;
                sLodValues.m_fBillboardFadeOut = 1.0f;
            }
            else
            {
                sLodValues.m_nBranchActiveLod = m_nNumBranchLodLevels - 1;
                sLodValues.m_nFrondActiveLod = m_nNumFrondLodLevels - 1;
                sLodValues.m_fBranchAlphaTestValue = sLodValues.m_fFrondAlphaTestValue = m_fTargetAlphaValue;
            }
        }
        // is full LOD on?
        else if (fLodLevel >= 1.0f)
        {
            sLodValues.m_nBranchActiveLod = sLodValues.m_nFrondActiveLod = 0;
            sLodValues.m_fBranchAlphaTestValue = sLodValues.m_fFrondAlphaTestValue = m_fTargetAlphaValue;
        }
        // is a middle LOD value active?
        else
        {
            // branches
            sLodValues.m_nBranchActiveLod = int(fOppositeLodValue * m_nNumBranchLodLevels);
            st_assert(sLodValues.m_nBranchActiveLod != m_nNumBranchLodLevels);

            // fronds
            sLodValues.m_nFrondActiveLod = int(fOppositeLodValue * m_nNumFrondLodLevels);
            st_assert(sLodValues.m_nFrondActiveLod != m_nNumFrondLodLevels);

            // determine branch and frond alpha test values
            if (m_bDropToBillboard && fLodLevel < c_fShortenedTransitionWidth)
            {
                sLodValues.m_fBranchAlphaTestValue = sLodValues.m_fFrondAlphaTestValue = ComputeLodCurve(c_fClear, m_fTargetAlphaValue, fLodLevel / c_fShortenedTransitionWidth, true);
                sLodValues.m_fBillboardFadeOut = 1.5f * (1.0f - fLodLevel / c_fShortenedTransitionWidth);
                if (sLodValues.m_fBillboardFadeOut > 1.0f)
                    sLodValues.m_fBillboardFadeOut = 1.0f;
            }
            else
                sLodValues.m_fBranchAlphaTestValue = sLodValues.m_fFrondAlphaTestValue = m_fTargetAlphaValue;
        }
    }

    // leaves
    {
        // is the lowest possible LOD active?
        if (fLodLevel <= 0.0f)
        {
            // only leaf layer 0 is active, last discrete LOD if no billboard, -1 if billboard
            sLodValues.m_anLeafActiveLods[0] = m_bDropToBillboard ? -1 : m_nNumLeafLodLevels - 1;
            sLodValues.m_afLeafAlphaTestValues[0] = m_bDropToBillboard ? c_fClear : m_fTargetAlphaValue;
            sLodValues.m_anLeafActiveLods[1] = -1;
        }
        // is the highest possible LOD active?
        else if (fLodLevel >= 1.0f)
        {
            // only leaf layer 0 is active and it's the first LOD value (0)
            sLodValues.m_anLeafActiveLods[0] = 0;
            sLodValues.m_afLeafAlphaTestValues[0] = m_fTargetAlphaValue;
            sLodValues.m_anLeafActiveLods[1] = -1;
        }
        // is a middle LOD value active?
        else
        {
            // LOD_SMOOTH is most common
            if (m_eLeafLodMethod == LOD_SMOOTH)
            {
                // compute a few handy values
                const float fCrestWidth = m_bDropToBillboard ? m_fCrestWidthBB : m_fCrestWidth;     // length of top of LOD curve where main LOD = 1.0
                const float fCycleLength = m_bDropToBillboard ? m_fCycleLengthBB : m_fCycleLength;  // length between whole LOD cycles

                // setup a few defaults
                sLodValues.m_anLeafActiveLods[1] = -1;
                sLodValues.m_afLeafAlphaTestValues[1] = c_fClear;

                // compute alpha values
                if (m_bDropToBillboard && fLodLevel < c_fShortenedTransitionWidth)
                {
                    sLodValues.m_afLeafAlphaTestValues[0] = ComputeLodCurve(c_fClear, m_fTargetAlphaValue, fLodLevel / c_fShortenedTransitionWidth, true);
                    sLodValues.m_anLeafActiveLods[0] = (sLodValues.m_afLeafAlphaTestValues[0] == c_fClear) ? -1 : m_nNumLeafLodLevels - 1;
                }
                else
                {
                    if (m_bDropToBillboard)
                        fLodLevel -= m_fTransitionWidth;

                    float fUnit = st_fmod(fLodLevel, fCycleLength);
                    int nRegion = m_nNumLeafLodLevels - int(fLodLevel / fCycleLength) - 1;
                    nRegion = st_max(nRegion, 0);

                    sLodValues.m_anLeafActiveLods[0] = nRegion;
                    if (fUnit < fCrestWidth)
                    {
                        sLodValues.m_afLeafAlphaTestValues[0] = m_fTargetAlphaValue;
                    }
                    else
                    {
                        const float fTransitionPoint = fUnit - fCrestWidth;
                        const float fPercent = fTransitionPoint / m_fTransitionWidth;
                        // leaf 0 curve
                        if (fPercent > m_fLeafTransitionFactor)
                            sLodValues.m_afLeafAlphaTestValues[1] = ComputeLodCurve(m_fTargetAlphaValue, c_fClear, (fPercent - m_fLeafTransitionFactor) / c_fOneMinusTransitionFactor, false);
                        else
                            sLodValues.m_afLeafAlphaTestValues[1] = m_fTargetAlphaValue;
                        // leaf 1 curve
                        if (fPercent < c_fOneMinusTransitionFactor)
                            sLodValues.m_afLeafAlphaTestValues[0] = ComputeLodCurve(c_fClear, m_fTargetAlphaValue, fPercent / c_fOneMinusTransitionFactor, true);
                        else
                            sLodValues.m_afLeafAlphaTestValues[0] = m_fTargetAlphaValue;
                        sLodValues.m_anLeafActiveLods[0] = nRegion - 1;
                        sLodValues.m_anLeafActiveLods[1] = nRegion;
                    }
                }
            }
            // no LOD is active, always use highest
            else if (m_eLeafLodMethod == LOD_NONE)
            {
                // set leaf layer 0
                sLodValues.m_anLeafActiveLods[0] = 0;
                sLodValues.m_afLeafAlphaTestValues[0] = m_fTargetAlphaValue;
            }
            else // assume LOD_POP (do hard transitions between LODs)
            {
                sLodValues.m_anLeafActiveLods[1] = -1;

                sLodValues.m_anLeafActiveLods[0] = int(fOppositeLodValue * m_nNumLeafLodLevels);
                st_assert(sLodValues.m_anLeafActiveLods[0] != m_nNumLeafLodLevels)
                    sLodValues.m_afLeafAlphaTestValues[0] = m_fTargetAlphaValue;
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::operator new

void* CSpeedTreeRT::operator new(size_t sSize)
{
    void* pBlock = g_pAllocator ? g_pAllocator->Alloc(sSize) : malloc(sSize);
    if (!pBlock)
        throw(std::bad_alloc( )); // ANSI/ISO compliant behavior

#ifdef TRACK_ALLOC_STATS
    CSpeedTreeAllocator::TrackAlloc("CSpeedTreeRT::operator new", pBlock, sSize);
#endif

    return pBlock;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::operator new[]

void* CSpeedTreeRT::operator new[](size_t sSize)
{
    void* pBlock = g_pAllocator ? g_pAllocator->Alloc(sSize) : malloc(sSize);
    if (!pBlock)
        throw(std::bad_alloc( )); // ANSI/ISO compliant behavior

#ifdef TRACK_ALLOC_STATS
    CSpeedTreeAllocator::TrackAlloc("CSpeedTreeRT::operator new[]", pBlock, sSize);
#endif

    return pBlock;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::operator delete

void CSpeedTreeRT::operator delete(void* pBlock)
{
    if (pBlock)
    {
#ifdef TRACK_ALLOC_STATS
        CSpeedTreeAllocator::TrackFree("CSpeedTreeRT::operator delete", pBlock, pBlock ? sizeof(CSpeedTreeRT) : 0);
#endif

        if (g_pAllocator)
            g_pAllocator->Free(pBlock);
        else
            free(pBlock);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::operator delete[]

void CSpeedTreeRT::operator delete[](void* pBlock)
{
    if (pBlock)
    {
#ifdef TRACK_ALLOC_STATS
        CSpeedTreeAllocator::TrackFree("CSpeedTreeRT::operator delete[]", pBlock, pBlock ? sizeof(CSpeedTreeRT) : 0);
#endif

        if (g_pAllocator)
            g_pAllocator->Free(pBlock);
        else
            free(pBlock);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetMapBank definition

void CSpeedTreeRT::GetMapBank(CSpeedTreeRT::SMapBank& sMapBank) const
{
    st_assert(m_pMapBank);

    try
    {
        // branches
        sMapBank.m_pBranchMaps = st_new_array<const char*>(CSpeedTreeRT::TL_SHADOW, "CSpeedTreeRT::GetMapBank, m_pBranchMaps)");
        int i = 0;
        for (i = 0; i < CSpeedTreeRT::TL_SHADOW; ++i)
            sMapBank.m_pBranchMaps[i] = m_pMapBank->m_sBranchMaps.m_astrMaps[i].c_str( );

        // leaves
        sMapBank.m_uiNumLeafMaps = (unsigned int) m_pMapBank->m_vLeafMaps.size( );
        if (sMapBank.m_uiNumLeafMaps > 0)
        {
            sMapBank.m_pLeafMaps = st_new_array<const char**>(sMapBank.m_uiNumLeafMaps, "CSpeedTreeRT::GetMapBank, m_pLeafMaps");
            for (i = 0; i < int(sMapBank.m_uiNumLeafMaps); ++i)
            {
                sMapBank.m_pLeafMaps[i] = st_new_array<const char*>(CSpeedTreeRT::TL_SHADOW, "CSpeedTreeRT::GetMapBank, m_pLeafMaps[ ])");
                for (int j = 0; j < CSpeedTreeRT::TL_SHADOW; ++j)
                    sMapBank.m_pLeafMaps[i][j] = m_pMapBank->m_vLeafMaps[i].m_astrMaps[j].c_str( );
            }
        }

        // fronds
        sMapBank.m_uiNumFrondMaps = (unsigned int) m_pMapBank->m_vFrondMaps.size( );
        if (sMapBank.m_uiNumFrondMaps > 0)
        {
            sMapBank.m_pFrondMaps = st_new_array<const char**>(sMapBank.m_uiNumFrondMaps, "CSpeedTreeRT::GetMapBank, m_pFrondMaps");
            for (i = 0; i < int(sMapBank.m_uiNumFrondMaps); ++i)
            {
                sMapBank.m_pFrondMaps[i] = st_new_array<const char*>(CSpeedTreeRT::TL_SHADOW, "CSpeedTreeRT::GetMapBank, m_pFrondMaps[ ])");
                for (int j = 0; j < CSpeedTreeRT::TL_SHADOW; ++j)
                    sMapBank.m_pFrondMaps[i][j] = m_pMapBank->m_vFrondMaps[i].m_astrMaps[j].c_str( );
            }
        }

        // composite
        sMapBank.m_pCompositeMaps = st_new_array<const char*>(CSpeedTreeRT::TL_SHADOW, "CSpeedTreeRT::GetMapBank, m_pCompositeMaps)");
        for (i = 0; i < CSpeedTreeRT::TL_SHADOW; ++i)
            sMapBank.m_pCompositeMaps[i] = m_pMapBank->m_sCompositeMaps.m_astrMaps[i].c_str( );

        // separate billboards
        sMapBank.m_pBillboardMaps = st_new_array<const char*>(CSpeedTreeRT::TL_SHADOW, "CSpeedTreeRT::GetMapBank, m_pBillboardMaps)");
        for (i = 0; i < CSpeedTreeRT::TL_SHADOW; ++i)
            sMapBank.m_pBillboardMaps[i] = m_pMapBank->m_sBillboardMaps.m_astrMaps[i].c_str( );

        // self shadow
        sMapBank.m_pSelfShadowMap = m_pMapBank->m_strSelfShadowMap.c_str( );
    }

    SpeedTreeCatch("CSpeedTreeRT::GetMapBank")
    SpeedTreeCatchAll("CSpeedTreeRT::GetMapBank");
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetTextureLayerName definition

const char* CSpeedTreeRT::GetTextureLayerName(CSpeedTreeRT::ETextureLayers eLayer)
{
    const char* pLayer = "Unknown";

    switch (eLayer)
    {
    case TL_DIFFUSE:
        pLayer = "Diffuse";
        break;
    case TL_DETAIL:
        pLayer = "Detail";
        break;
    case TL_NORMAL:
        pLayer = "Normal";
        break;
    case TL_HEIGHT:
        pLayer = "Height";
        break;
    case TL_SPECULAR:
        pLayer = "Specular";
        break;
    case TL_USER1:
        pLayer = "User 1";
        break;
    case TL_USER2:
        pLayer = "User 2";
        break;
    case TL_SHADOW:
        pLayer = "Shadow";
        break;
    default:
        pLayer = "Unknown";
        break;
    }

    return pLayer;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::RecoverDeprecatedMaps definition

void CSpeedTreeRT::RecoverDeprecatedMaps(void)
{
    st_assert(m_pMapBank);
    st_assert(m_pEngine);
    st_assert(m_pEmbeddedTexCoords);

    // branch
    m_pMapBank->m_sBranchMaps.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE] = m_pEngine->GetBranchTextureFilename( );

    // leaves
    const st_vector_leaftexture& vLeaves = m_pEngine->GetLeafTextures( );
    unsigned int i = 0;
    for (i = 0; i < vLeaves.size( ); ++i)
    {
        SMapCollection sCollection;
        sCollection.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE] = vLeaves[i].m_strFilename;
        m_pMapBank->m_vLeafMaps.push_back(sCollection);
    }

    // fronds
    for (i = 0; i < m_pFrondEngine->GetTextureCount( ); ++i)
    {
        SMapCollection sCollection;
        sCollection.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE] = m_pFrondEngine->GetTextureFilename(i);
        m_pMapBank->m_vFrondMaps.push_back(sCollection);
    }

    // composite
    m_pMapBank->m_sCompositeMaps.m_astrMaps[CSpeedTreeRT::TL_DIFFUSE] = m_pEmbeddedTexCoords->m_strCompositeFilename;

    // self shadow
    if (m_pProjectedShadow)
        m_pMapBank->m_strSelfShadowMap = m_pProjectedShadow->GetSelfShadowFilename( );
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveSupplementalCollisionObjectsInfo

void CSpeedTreeRT::SaveSupplementalCollisionObjectsInfo(CTreeFileAccess& cFile) const
{
    if (m_pCollisionObjects && m_pCollisionObjects->m_vObjects.size( ) > 0)
    {
        cFile.SaveToken(File_BeginSupplementalCollisionObjectInfo);

        for (int i = 0; i < int(m_pCollisionObjects->m_vObjects.size( )); ++i)
        {
            cFile.SaveToken(File_BeginCollisionObject);

            cFile.SaveToken(File_CollisionRotateX);
            cFile.SaveFloat(m_pCollisionObjects->m_vObjects[i].m_afRotation[0]);

            cFile.SaveToken(File_CollisionRotateY);
            cFile.SaveFloat(m_pCollisionObjects->m_vObjects[i].m_afRotation[1]);

            cFile.SaveToken(File_CollisionRotateZ);
            cFile.SaveFloat(m_pCollisionObjects->m_vObjects[i].m_afRotation[2]);

            cFile.SaveToken(File_EndCollisionObject);
        }

        cFile.SaveToken(File_EndSupplementalCollisionObjectInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseSupplementalCollisionObjectsInfo

void CSpeedTreeRT::ParseSupplementalCollisionObjectsInfo(CTreeFileAccess& cFile)
{
    if (m_pCollisionObjects)
    {
        int nIndex = 0;
        int nToken = cFile.ParseToken( );
        do
        {
            if (nToken == File_BeginCollisionObject && nIndex < int(m_pCollisionObjects->m_vObjects.size( )))
            {
                nToken = cFile.ParseToken( );
                do
                {
                    switch (nToken)
                    {
                    case File_CollisionRotateX:
                        m_pCollisionObjects->m_vObjects[nIndex].m_afRotation[0] = cFile.ParseFloat( );
                        break;
                    case File_CollisionRotateY:
                        m_pCollisionObjects->m_vObjects[nIndex].m_afRotation[1] = cFile.ParseFloat( );
                        break;
                    case File_CollisionRotateZ:
                        m_pCollisionObjects->m_vObjects[nIndex].m_afRotation[2] = cFile.ParseFloat( );
                        break;
                    case File_EndCollisionObject:
                        break;
                    }

                    nToken = cFile.ParseToken( );
                } while (nToken != File_EndCollisionObject);
            }
            else
            {
                if (nIndex >= int(m_pCollisionObjects->m_vObjects.size( )))
                    throw(IdvFileError("too many collision objects specified in supplemental info"));
                else
                    throw(IdvFileError("malformed supplemental collision object info"));
            }

            ++nIndex;

            nToken = cFile.ParseToken( );
        } while (nToken != File_EndSupplementalCollisionObjectInfo);
    }
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SetMemoryPreference

void CSpeedTreeRT::SetMemoryPreference(EMemoryPreference eMemPref)
{
    m_eMemoryPreference = eMemPref;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::GetMemoryPreference

CSpeedTreeRT::EMemoryPreference CSpeedTreeRT::GetMemoryPreference(void)
{
    return m_eMemoryPreference;
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::SaveSupplementalLodInfo

void CSpeedTreeRT::SaveSupplementalLodInfo(CTreeFileAccess& cFile) const
{
    cFile.SaveToken(File_BeginSupplementalLodInfo);

    cFile.SaveToken(File_BranchLodFadeDistance);
    cFile.SaveFloat(m_fBranchLodFadeDistance);

    cFile.SaveToken(File_FrondLodFadeDistance);
    cFile.SaveFloat(m_fFrondLodFadeDistance);

    cFile.SaveToken(File_ApplyFadingToExtrusions);
    cFile.SaveBool(m_bApplyFadingToExtrusions);

    cFile.SaveToken(File_BillboardTransitionFactor);
    cFile.SaveFloat(m_fBillboardTransition);

    cFile.SaveToken(File_EndSupplementalLodInfo);
}


///////////////////////////////////////////////////////////////////////
//  CSpeedTreeRT::ParseSupplementalLodInfo

void CSpeedTreeRT::ParseSupplementalLodInfo(CTreeFileAccess& cFile)
{
    int nToken = cFile.ParseToken( );
    do
    {
        switch (nToken)
        {
        case File_BranchLodFadeDistance:
            m_fBranchLodFadeDistance = cFile.ParseFloat( );
            break;
        case File_FrondLodFadeDistance:
            m_fFrondLodFadeDistance = cFile.ParseFloat( );
            break;
        case File_ApplyFadingToExtrusions:
            m_bApplyFadingToExtrusions = cFile.ParseBool( );
            break;
        case File_BillboardTransitionFactor:
            m_fBillboardTransition = cFile.ParseFloat( );
            break;
        default:
            break;
        }

        nToken = cFile.ParseToken( );
    } while (nToken != File_EndSupplementalLodInfo);

    if (m_bApplyFadingToExtrusions && m_pFrondEngine)
    {
        m_pFrondEngine->ProhibitSegmentReduction( );
    }
}


