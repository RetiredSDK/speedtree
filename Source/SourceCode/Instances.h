///////////////////////////////////////////////////////////////////////  
//  Instances.h
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

#include "LibGlobals_Source/IdvGlobals.h"
#include "LibVector_Source/IdvVector.h"
#include <vector>


class CSpeedTreeRT;

///////////////////////////////////////////////////////////////////////  
//  struct STreeInstanceData definition
//
//  If a new CSpeedTreeRT is created that is an instance of another tree,
//  this structure stores the instance's overriding values.

struct STreeInstanceData
{
        STreeInstanceData( ) :
            m_pParent(NULL),
            m_fLodLevel(1.0f)
        {
        }

        const CSpeedTreeRT* m_pParent;      // which tree this is an instance of
        stVec3              m_cPosition;    // position for this instance, different from parent
        float               m_fLodLevel;    // this instance's LOD value
};



///////////////////////////////////////////////////////////////////////  
//  struct SInstanceList definition
//
//  A single tree may have multiple instances.  This structure tracks all
//  of the instances of a single (m_vInstances), as well as all of the
//  unique trees in an entire scene (m_vUniqueTrees).

struct SInstanceList
{
        st_vector_speedtree_p   m_vInstances;       // list of instances of a particular tree
//static  st_vector_speedtree_p   m_vUniqueTrees;     // list of all trees in the scene that aren't instances
};




