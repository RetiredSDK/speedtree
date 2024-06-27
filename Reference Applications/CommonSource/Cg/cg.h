/*
 *
 * Copyright (c) 2002-2006, NVIDIA Corporation.
 * 
 *  
 * 
 * NVIDIA Corporation("NVIDIA") supplies this software to you in consideration 
 * of your agreement to the following terms, and your use, installation, 
 * modification or redistribution of this NVIDIA software constitutes 
 * acceptance of these terms.  If you do not agree with these terms, please do 
 * not use, install, modify or redistribute this NVIDIA software.
 * 
 *  
 * 
 * In consideration of your agreement to abide by the following terms, and 
 * subject to these terms, NVIDIA grants you a personal, non-exclusive license,
 * under NVIDIA’s copyrights in this original NVIDIA software (the "NVIDIA 
 * Software"), to use, reproduce, modify and redistribute the NVIDIA 
 * Software, with or without modifications, in source and/or binary forms; 
 * provided that if you redistribute the NVIDIA Software, you must retain the 
 * copyright notice of NVIDIA, this notice and the following text and 
 * disclaimers in all such redistributions of the NVIDIA Software. Neither the 
 * name, trademarks, service marks nor logos of NVIDIA Corporation may be used 
 * to endorse or promote products derived from the NVIDIA Software without 
 * specific prior written permission from NVIDIA.  Except as expressly stated 
 * in this notice, no other rights or licenses express or implied, are granted 
 * by NVIDIA herein, including but not limited to any patent rights that may be 
 * infringed by your derivative works or by other works in which the NVIDIA 
 * Software may be incorporated. No hardware is licensed hereunder. 
 * 
 *  
 * 
 * THE NVIDIA SOFTWARE IS BEING PROVIDED ON AN "AS IS" BASIS, WITHOUT 
 * WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING 
 * WITHOUT LIMITATION, WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR ITS USE AND OPERATION 
 * EITHER ALONE OR IN COMBINATION WITH OTHER PRODUCTS.
 * 
 *  
 * 
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, 
 * EXEMPLARY, CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, LOST 
 * PROFITS; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) OR ARISING IN ANY WAY OUT OF THE USE, 
 * REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION OF THE NVIDIA SOFTWARE, 
 * HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING 
 * NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF NVIDIA HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */ 


#ifndef _cg_h
#define _cg_h


#define CG_VERSION_NUM                1502

#ifdef _WIN32
# ifndef APIENTRY /* From Win32's <windef.h> */
#  define CG_APIENTRY_DEFINED
#  if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) || defined(__LCC__)
#   define APIENTRY    __stdcall
#  else
#   define APIENTRY
#  endif
# endif
# ifndef WINGDIAPI /* From Win32's <wingdi.h> and <winnt.h> */
#  define CG_WINGDIAPI_DEFINED
#  define WINGDIAPI __declspec(dllimport)
# endif
#endif /* _WIN32 */

/* Set up for either Win32 import/export/lib. */
#ifndef CGDLL_API
# ifdef _WIN32
#  ifdef CGDLL_EXPORTS
#   define CGDLL_API __declspec(dllexport)
#  elif defined (CG_LIB)
#   define CGDLL_API
#  else
#   define CGDLL_API __declspec(dllimport)
#  endif
# else
#  define CGDLL_API
# endif
#endif

#ifndef CGENTRY
# ifdef _WIN32
#  define CGENTRY __cdecl
# else
#  define CGENTRY
# endif
#endif

/*************************************************************************/
/*** CG Run-Time Library API                                          ***/
/*************************************************************************/

/*************************************************************************/
/*** Data types and enumerants                                         ***/
/*************************************************************************/

typedef int CGbool;

#define CG_FALSE ((CGbool)0)
#define CG_TRUE ((CGbool)1)

typedef struct _CGcontext *CGcontext;
typedef struct _CGprogram *CGprogram;
typedef struct _CGparameter *CGparameter;
typedef struct _CGeffect *CGeffect;
typedef struct _CGtechnique *CGtechnique;
typedef struct _CGpass *CGpass;
typedef struct _CGstate *CGstate;
typedef struct _CGstateassignment *CGstateassignment;
typedef struct _CGannotation *CGannotation;
typedef void *CGhandle;

//!!! PREPROCESS BEGIN

typedef enum
 {
  CG_UNKNOWN_TYPE,
  CG_STRUCT,
  CG_ARRAY,

  CG_TYPE_START_ENUM = 1024,
# define CG_DATATYPE_MACRO(name, compiler_name, enum_name, base_name, ncols, nrows, pc) \
  enum_name ,

#include <Cg/cg_datatypes.h>

# undef CG_DATATYPE_MACRO

 } CGtype;

typedef enum
 {
# define CG_BINDLOCATION_MACRO(name,enum_name,compiler_name,\
                               enum_int,addressable,param_type) \
  enum_name = enum_int,

#include <Cg/cg_bindlocations.h>

  CG_UNDEFINED = 3256,

 } CGresource;

typedef enum
 {
  CG_PROFILE_START = 6144,
  CG_PROFILE_UNKNOWN,

# define CG_PROFILE_MACRO(name, compiler_id, compiler_id_caps, compiler_opt,int_id,vertex_profile) \
   CG_PROFILE_##compiler_id_caps = int_id,
  
#include <Cg/cg_profiles.h>

  CG_PROFILE_MAX = 7100,
 } CGprofile;

typedef enum
 {
# define CG_ERROR_MACRO(code, enum_name, message) \
   enum_name = code,
# include <Cg/cg_errors.h>
 } CGerror;

typedef enum
 {
  CG_PARAMETERCLASS_UNKNOWN = 0,
  CG_PARAMETERCLASS_SCALAR,
  CG_PARAMETERCLASS_VECTOR,
  CG_PARAMETERCLASS_MATRIX,
  CG_PARAMETERCLASS_STRUCT,
  CG_PARAMETERCLASS_ARRAY,
  CG_PARAMETERCLASS_SAMPLER,
  CG_PARAMETERCLASS_OBJECT
 } CGparameterclass;

//!!! PREPROCESS END

typedef enum
 {
# define CG_ENUM_MACRO(enum_name, enum_val) \
   enum_name = enum_val,
# include <Cg/cg_enums.h>
 } CGenum;

typedef enum
{
    CG_UNKNOWN_DOMAIN = 0,
    CG_FIRST_DOMAIN   = 1,
    CG_VERTEX_DOMAIN  = 1,
    CG_FRAGMENT_DOMAIN,
    CG_GEOMETRY_DOMAIN,
    CG_NUMBER_OF_DOMAINS
} CGdomain;

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef CGbool (CGENTRY * CGstatecallback)(CGstateassignment);
typedef void (CGENTRY * CGerrorCallbackFunc)(void);
typedef void (CGENTRY * CGerrorHandlerFunc)(CGcontext ctx, CGerror err, void *data);

/*************************************************************************/
/*** Functions                                                         ***/
/*************************************************************************/

#ifndef CG_EXPLICIT

/*** Context functions ***/

CGDLL_API CGcontext CGENTRY cgCreateContext(void); 
CGDLL_API void CGENTRY cgDestroyContext(CGcontext ctx); 
CGDLL_API CGbool CGENTRY cgIsContext(CGcontext ctx);
CGDLL_API const char * CGENTRY cgGetLastListing(CGcontext ctx);
CGDLL_API void CGENTRY cgSetLastListing(CGhandle handle, const char *listing);
CGDLL_API void CGENTRY cgSetAutoCompile(CGcontext ctx, CGenum flag);
CGDLL_API CGenum CGENTRY cgGetAutoCompile(CGcontext ctx);

/*** Program functions ***/

CGDLL_API CGprogram CGENTRY cgCreateProgram(CGcontext ctx, 
                                    CGenum program_type,
                                    const char *program,
                                    CGprofile profile,
                                    const char *entry,
                                    const char **args);
CGDLL_API CGprogram CGENTRY cgCreateProgramFromFile(CGcontext ctx, 
                                            CGenum program_type,
                                            const char *program_file,
                                            CGprofile profile,
                                            const char *entry,
                                            const char **args);
CGDLL_API CGprogram CGENTRY cgCopyProgram(CGprogram program); 
CGDLL_API void CGENTRY cgDestroyProgram(CGprogram program); 

CGDLL_API CGprogram CGENTRY cgGetFirstProgram(CGcontext ctx);
CGDLL_API CGprogram CGENTRY cgGetNextProgram(CGprogram current);
CGDLL_API CGcontext CGENTRY cgGetProgramContext(CGprogram prog);
CGDLL_API CGbool CGENTRY cgIsProgram(CGprogram program); 

CGDLL_API void CGENTRY cgCompileProgram(CGprogram program); 
CGDLL_API CGbool CGENTRY cgIsProgramCompiled(CGprogram program); 
CGDLL_API const char * CGENTRY cgGetProgramString(CGprogram prog, CGenum pname); 
CGDLL_API CGprofile CGENTRY cgGetProgramProfile(CGprogram prog); 
CGDLL_API char const * const * CGENTRY cgGetProgramOptions(CGprogram prog);
CGDLL_API void CGENTRY cgSetProgramProfile(CGprogram prog, CGprofile profile);

CGDLL_API void CGENTRY cgSetPassProgramParameters(CGprogram);

/*** Parameter functions ***/

CGDLL_API CGparameter CGENTRY cgCreateParameter(CGcontext ctx, CGtype type);
CGDLL_API CGparameter CGENTRY cgCreateParameterArray(CGcontext ctx,
                                             CGtype type, 
                                             int length);
CGDLL_API CGparameter CGENTRY cgCreateParameterMultiDimArray(CGcontext ctx,
                                                     CGtype type,
                                                     int dim, 
                                                     const int *lengths);
CGDLL_API void CGENTRY cgDestroyParameter(CGparameter param);
CGDLL_API void CGENTRY cgConnectParameter(CGparameter from, CGparameter to);
CGDLL_API void CGENTRY cgDisconnectParameter(CGparameter param);
CGDLL_API CGparameter CGENTRY cgGetConnectedParameter(CGparameter param);

CGDLL_API int CGENTRY cgGetNumConnectedToParameters(CGparameter param);
CGDLL_API CGparameter CGENTRY cgGetConnectedToParameter(CGparameter param, int index);

CGDLL_API CGparameter CGENTRY cgGetNamedParameter(CGprogram prog, const char *name);
CGDLL_API CGparameter CGENTRY cgGetNamedProgramParameter(CGprogram prog, 
                                                 CGenum name_space, 
                                                 const char *name);

CGDLL_API CGparameter CGENTRY cgGetFirstParameter(CGprogram prog, CGenum name_space);
CGDLL_API CGparameter CGENTRY cgGetNextParameter(CGparameter current);
CGDLL_API CGparameter CGENTRY cgGetFirstLeafParameter(CGprogram prog, CGenum name_space);
CGDLL_API CGparameter CGENTRY cgGetNextLeafParameter(CGparameter current);

CGDLL_API CGparameter CGENTRY cgGetFirstStructParameter(CGparameter param);
CGDLL_API CGparameter CGENTRY cgGetNamedStructParameter(CGparameter param, 
                                                const char *name);

CGDLL_API CGparameter CGENTRY cgGetFirstDependentParameter(CGparameter param);

CGDLL_API CGparameter CGENTRY cgGetArrayParameter(CGparameter aparam, int index);
CGDLL_API int CGENTRY cgGetArrayDimension(CGparameter param);
CGDLL_API CGtype CGENTRY cgGetArrayType(CGparameter param);
CGDLL_API int CGENTRY cgGetArraySize(CGparameter param, int dimension);
CGDLL_API int CGENTRY cgGetArrayTotalSize(CGparameter param);
CGDLL_API void CGENTRY cgSetArraySize(CGparameter param, int size);
CGDLL_API void CGENTRY cgSetMultiDimArraySize(CGparameter param, const int *sizes);

CGDLL_API CGprogram CGENTRY cgGetParameterProgram(CGparameter param);
CGDLL_API CGcontext CGENTRY cgGetParameterContext(CGparameter param);
CGDLL_API CGbool CGENTRY cgIsParameter(CGparameter param);
CGDLL_API const char * CGENTRY cgGetParameterName(CGparameter param);
CGDLL_API CGtype CGENTRY cgGetParameterType(CGparameter param);
CGDLL_API CGtype CGENTRY cgGetParameterBaseType(CGparameter param);
CGDLL_API CGparameterclass CGENTRY cgGetParameterClass(CGparameter param);
CGDLL_API int CGENTRY cgGetParameterRows(CGparameter param);
CGDLL_API int CGENTRY cgGetParameterColumns(CGparameter param);
CGDLL_API CGtype CGENTRY cgGetParameterNamedType(CGparameter param);
CGDLL_API const char * CGENTRY cgGetParameterSemantic(CGparameter param);
CGDLL_API CGresource CGENTRY cgGetParameterResource(CGparameter param);
CGDLL_API CGresource CGENTRY cgGetParameterBaseResource(CGparameter param);
CGDLL_API unsigned long CGENTRY cgGetParameterResourceIndex(CGparameter param);
CGDLL_API CGenum CGENTRY cgGetParameterVariability(CGparameter param);
CGDLL_API CGenum CGENTRY cgGetParameterDirection(CGparameter param);
CGDLL_API CGbool CGENTRY cgIsParameterReferenced(CGparameter param);
CGDLL_API CGbool CGENTRY cgIsParameterUsed(CGparameter param, CGhandle handle);
CGDLL_API const double * CGENTRY cgGetParameterValues(CGparameter param, 
                                             CGenum value_type,
                                             int *nvalues);
CGDLL_API void CGENTRY cgSetParameterValuedr(CGparameter param, int n, const double *vals);
CGDLL_API void CGENTRY cgSetParameterValuedc(CGparameter param, int n, const double *vals);
CGDLL_API void CGENTRY cgSetParameterValuefr(CGparameter param, int n, const float *vals);
CGDLL_API void CGENTRY cgSetParameterValuefc(CGparameter param, int n, const float *vals);
CGDLL_API void CGENTRY cgSetParameterValueir(CGparameter param, int n, const int *vals);
CGDLL_API void CGENTRY cgSetParameterValueic(CGparameter param, int n, const int *vals);
CGDLL_API int CGENTRY cgGetParameterValuedr(CGparameter param, int n, double *vals);
CGDLL_API int CGENTRY cgGetParameterValuedc(CGparameter param, int n, double *vals);
CGDLL_API int CGENTRY cgGetParameterValuefr(CGparameter param, int n, float *vals);
CGDLL_API int CGENTRY cgGetParameterValuefc(CGparameter param, int n, float *vals);
CGDLL_API int CGENTRY cgGetParameterValueir(CGparameter param, int n, int *vals);
CGDLL_API int CGENTRY cgGetParameterValueic(CGparameter param, int n, int *vals);
CGDLL_API const char * CGENTRY cgGetStringParameterValue(CGparameter param);
CGDLL_API void CGENTRY cgSetStringParameterValue(CGparameter param, const char *str);

CGDLL_API int CGENTRY cgGetParameterOrdinalNumber(CGparameter param);
CGDLL_API CGbool CGENTRY cgIsParameterGlobal(CGparameter param);
CGDLL_API int CGENTRY cgGetParameterIndex(CGparameter param);

CGDLL_API void CGENTRY cgSetParameterVariability(CGparameter param, CGenum vary);
CGDLL_API void CGENTRY cgSetParameterSemantic(CGparameter param, const char *semantic);

CGDLL_API void CGENTRY cgSetParameter1f(CGparameter param, float x);
CGDLL_API void CGENTRY cgSetParameter2f(CGparameter param, float x, float y);
CGDLL_API void CGENTRY cgSetParameter3f(CGparameter param, float x, float y, float z);
CGDLL_API void CGENTRY cgSetParameter4f(CGparameter param, 
                                float x, 
                                float y, 
                                float z,
                                float w);
CGDLL_API void CGENTRY cgSetParameter1d(CGparameter param, double x);
CGDLL_API void CGENTRY cgSetParameter2d(CGparameter param, double x, double y);
CGDLL_API void CGENTRY cgSetParameter3d(CGparameter param, 
                                double x, 
                                double y, 
                                double z);
CGDLL_API void CGENTRY cgSetParameter4d(CGparameter param, 
                                double x, 
                                double y, 
                                double z,
                                double w);
CGDLL_API void CGENTRY cgSetParameter1i(CGparameter param, int x);
CGDLL_API void CGENTRY cgSetParameter2i(CGparameter param, int x, int y);
CGDLL_API void CGENTRY cgSetParameter3i(CGparameter param, int x, int y, int z);
CGDLL_API void CGENTRY cgSetParameter4i(CGparameter param, 
                                int x, 
                                int y, 
                                int z,
                                int w);

CGDLL_API void CGENTRY cgSetParameter1iv(CGparameter param, const int *v);
CGDLL_API void CGENTRY cgSetParameter2iv(CGparameter param, const int *v);
CGDLL_API void CGENTRY cgSetParameter3iv(CGparameter param, const int *v);
CGDLL_API void CGENTRY cgSetParameter4iv(CGparameter param, const int *v);
CGDLL_API void CGENTRY cgSetParameter1fv(CGparameter param, const float *v);
CGDLL_API void CGENTRY cgSetParameter2fv(CGparameter param, const float *v);
CGDLL_API void CGENTRY cgSetParameter3fv(CGparameter param, const float *v);
CGDLL_API void CGENTRY cgSetParameter4fv(CGparameter param, const float *v);
CGDLL_API void CGENTRY cgSetParameter1dv(CGparameter param, const double *v);
CGDLL_API void CGENTRY cgSetParameter2dv(CGparameter param, const double *v);
CGDLL_API void CGENTRY cgSetParameter3dv(CGparameter param, const double *v);
CGDLL_API void CGENTRY cgSetParameter4dv(CGparameter param, const double *v);

CGDLL_API void CGENTRY cgSetMatrixParameterir(CGparameter param, const int *matrix);
CGDLL_API void CGENTRY cgSetMatrixParameterdr(CGparameter param, const double *matrix);
CGDLL_API void CGENTRY cgSetMatrixParameterfr(CGparameter param, const float *matrix);
CGDLL_API void CGENTRY cgSetMatrixParameteric(CGparameter param, const int *matrix);
CGDLL_API void CGENTRY cgSetMatrixParameterdc(CGparameter param, const double *matrix);
CGDLL_API void CGENTRY cgSetMatrixParameterfc(CGparameter param, const float *matrix);

CGDLL_API void CGENTRY cgGetMatrixParameterir(CGparameter param, int *matrix);
CGDLL_API void CGENTRY cgGetMatrixParameterdr(CGparameter param, double *matrix);
CGDLL_API void CGENTRY cgGetMatrixParameterfr(CGparameter param, float *matrix);
CGDLL_API void CGENTRY cgGetMatrixParameteric(CGparameter param, int *matrix);
CGDLL_API void CGENTRY cgGetMatrixParameterdc(CGparameter param, double *matrix);
CGDLL_API void CGENTRY cgGetMatrixParameterfc(CGparameter param, float *matrix);

/*** Type Functions ***/

CGDLL_API const char * CGENTRY cgGetTypeString(CGtype type);
CGDLL_API CGtype CGENTRY cgGetType(const char *type_string);

CGDLL_API CGtype CGENTRY cgGetNamedUserType(CGhandle handle, const char *name);

CGDLL_API int CGENTRY cgGetNumUserTypes(CGhandle handle);
CGDLL_API CGtype CGENTRY cgGetUserType(CGhandle handle, int index);

CGDLL_API int CGENTRY cgGetNumParentTypes(CGtype type);
CGDLL_API CGtype CGENTRY cgGetParentType(CGtype type, int index);

CGDLL_API CGbool CGENTRY cgIsParentType(CGtype parent, CGtype child);
CGDLL_API CGbool CGENTRY cgIsInterfaceType(CGtype type);

/*** Resource Functions ***/

CGDLL_API const char * CGENTRY cgGetResourceString(CGresource resource);
CGDLL_API CGresource CGENTRY cgGetResource(const char *resource_string);

/*** Enum Functions ***/

CGDLL_API const char * CGENTRY cgGetEnumString(CGenum en);
CGDLL_API CGenum CGENTRY cgGetEnum(const char *enum_string);

/*** Profile Functions ***/

CGDLL_API const char * CGENTRY cgGetProfileString(CGprofile profile);
CGDLL_API CGprofile CGENTRY cgGetProfile(const char *profile_string);

/*** Error Functions ***/

CGDLL_API CGerror CGENTRY cgGetError(void);
CGDLL_API CGerror CGENTRY cgGetFirstError(void);
CGDLL_API const char * CGENTRY cgGetErrorString(CGerror error);
CGDLL_API const char * CGENTRY cgGetLastErrorString(CGerror *error);
CGDLL_API void CGENTRY cgSetErrorCallback(CGerrorCallbackFunc func);
CGDLL_API CGerrorCallbackFunc CGENTRY cgGetErrorCallback(void);
CGDLL_API void CGENTRY cgSetErrorHandler(CGerrorHandlerFunc func, void *data);
CGDLL_API CGerrorHandlerFunc CGENTRY cgGetErrorHandler(void **data);

/*** Misc Functions ***/

CGDLL_API const char * CGENTRY cgGetString(CGenum sname);


/*** CgFX Functions ***/

CGDLL_API CGeffect CGENTRY cgCreateEffect(CGcontext, const char *code, const char **args);
CGDLL_API CGeffect CGENTRY cgCreateEffectFromFile(CGcontext, const char *filename,
                                          const char **args);
CGDLL_API void CGENTRY cgDestroyEffect(CGeffect);
CGDLL_API CGcontext CGENTRY cgGetEffectContext(CGeffect);
CGDLL_API CGbool CGENTRY cgIsEffect(CGeffect effect);

CGDLL_API CGeffect CGENTRY cgGetFirstEffect(CGcontext);
CGDLL_API CGeffect CGENTRY cgGetNextEffect(CGeffect);

CGDLL_API CGprogram CGENTRY cgCreateProgramFromEffect(CGeffect effect,
                                              CGprofile profile,
                                              const char *entry,
                                              const char **args);

CGDLL_API CGtechnique CGENTRY cgGetFirstTechnique(CGeffect);
CGDLL_API CGtechnique CGENTRY cgGetNextTechnique(CGtechnique);
CGDLL_API CGtechnique CGENTRY cgGetNamedTechnique(CGeffect, const char *name);
CGDLL_API const char * CGENTRY cgGetTechniqueName(CGtechnique);
CGDLL_API CGbool CGENTRY cgIsTechnique(CGtechnique);
CGDLL_API CGbool CGENTRY cgValidateTechnique(CGtechnique);
CGDLL_API CGbool CGENTRY cgIsTechniqueValidated(CGtechnique);
CGDLL_API CGeffect CGENTRY cgGetTechniqueEffect(CGtechnique);

CGDLL_API CGpass CGENTRY cgGetFirstPass(CGtechnique);
CGDLL_API CGpass CGENTRY cgGetNamedPass(CGtechnique, const char *name);
CGDLL_API CGpass CGENTRY cgGetNextPass(CGpass);
CGDLL_API CGbool CGENTRY cgIsPass(CGpass);
CGDLL_API const char * CGENTRY cgGetPassName(CGpass); 
CGDLL_API CGtechnique CGENTRY cgGetPassTechnique(CGpass);

CGDLL_API void CGENTRY cgSetPassState(CGpass);
CGDLL_API void CGENTRY cgResetPassState(CGpass);

CGDLL_API CGstateassignment CGENTRY cgGetFirstStateAssignment(CGpass);
CGDLL_API CGstateassignment CGENTRY cgGetNamedStateAssignment(CGpass, const char *name);
CGDLL_API CGstateassignment CGENTRY cgGetNextStateAssignment(CGstateassignment);
CGDLL_API CGbool CGENTRY cgIsStateAssignment(CGstateassignment);
CGDLL_API CGbool CGENTRY cgCallStateSetCallback(CGstateassignment);
CGDLL_API CGbool CGENTRY cgCallStateValidateCallback(CGstateassignment);
CGDLL_API CGbool CGENTRY cgCallStateResetCallback(CGstateassignment);
CGDLL_API CGpass CGENTRY cgGetStateAssignmentPass(CGstateassignment);
CGDLL_API CGparameter CGENTRY cgGetSamplerStateAssignmentParameter(CGstateassignment);

CGDLL_API const float * CGENTRY cgGetFloatStateAssignmentValues(CGstateassignment, int *nVals);
CGDLL_API const int * CGENTRY cgGetIntStateAssignmentValues(CGstateassignment, int *nVals);
CGDLL_API const CGbool * CGENTRY cgGetBoolStateAssignmentValues(CGstateassignment, int *nVals);
CGDLL_API const char * CGENTRY cgGetStringStateAssignmentValue(CGstateassignment);
CGDLL_API CGprogram CGENTRY cgGetProgramStateAssignmentValue(CGstateassignment);
CGDLL_API CGparameter CGENTRY cgGetTextureStateAssignmentValue(CGstateassignment);
CGDLL_API CGparameter CGENTRY cgGetSamplerStateAssignmentValue(CGstateassignment);
CGDLL_API int CGENTRY cgGetStateAssignmentIndex(CGstateassignment);

CGDLL_API int CGENTRY cgGetNumDependentStateAssignmentParameters(CGstateassignment);
CGDLL_API CGparameter CGENTRY cgGetDependentStateAssignmentParameter(CGstateassignment, int index);

CGDLL_API CGstate CGENTRY cgGetStateAssignmentState(CGstateassignment);
CGDLL_API CGstate CGENTRY cgGetSamplerStateAssignmentState(CGstateassignment);

CGDLL_API CGstate CGENTRY cgCreateState(CGcontext, const char *name, CGtype);
CGDLL_API CGstate CGENTRY cgCreateArrayState(CGcontext, const char *name, CGtype, int nelems);
CGDLL_API void CGENTRY cgSetStateCallbacks(CGstate, CGstatecallback set, CGstatecallback reset,
                                   CGstatecallback validate);
CGDLL_API CGstatecallback CGENTRY cgGetStateSetCallback(CGstate);
CGDLL_API CGstatecallback CGENTRY cgGetStateResetCallback(CGstate);
CGDLL_API CGstatecallback CGENTRY cgGetStateValidateCallback(CGstate);
CGDLL_API CGtype CGENTRY cgGetStateType(CGstate);
CGDLL_API const char * CGENTRY cgGetStateName(CGstate);
CGDLL_API CGstate CGENTRY cgGetNamedState(CGcontext, const char *name);
CGDLL_API CGstate CGENTRY cgGetFirstState(CGcontext);
CGDLL_API CGstate CGENTRY cgGetNextState(CGstate);
CGDLL_API CGbool CGENTRY cgIsState(CGstate);
CGDLL_API void CGENTRY cgAddStateEnumerant(CGstate, const char *name, int value);

CGDLL_API CGstate CGENTRY cgCreateSamplerState(CGcontext, const char *name, CGtype);
CGDLL_API CGstate CGENTRY cgCreateArraySamplerState(CGcontext, const char *name, CGtype, int nelems);
CGDLL_API CGstate CGENTRY cgGetNamedSamplerState(CGcontext, const char *name);
CGDLL_API CGstate CGENTRY cgGetFirstSamplerState(CGcontext);

CGDLL_API CGstateassignment CGENTRY cgGetFirstSamplerStateAssignment(CGparameter);
CGDLL_API CGstateassignment CGENTRY cgGetNamedSamplerStateAssignment(CGparameter, const char *);
CGDLL_API void CGENTRY cgSetSamplerState(CGparameter);

CGDLL_API CGparameter CGENTRY cgGetNamedEffectParameter(CGeffect, const char *);
CGDLL_API CGparameter CGENTRY cgGetFirstLeafEffectParameter(CGeffect);
CGDLL_API CGparameter CGENTRY cgGetFirstEffectParameter(CGeffect);
CGDLL_API CGparameter CGENTRY cgGetEffectParameterBySemantic(CGeffect, const char *);

CGDLL_API CGannotation CGENTRY cgGetFirstTechniqueAnnotation(CGtechnique);
CGDLL_API CGannotation CGENTRY cgGetFirstPassAnnotation(CGpass);
CGDLL_API CGannotation CGENTRY cgGetFirstParameterAnnotation(CGparameter);
CGDLL_API CGannotation CGENTRY cgGetFirstProgramAnnotation(CGprogram);
CGDLL_API CGannotation CGENTRY cgGetFirstEffectAnnotation(CGeffect);
CGDLL_API CGannotation CGENTRY cgGetNextAnnotation(CGannotation);

CGDLL_API CGannotation CGENTRY cgGetNamedTechniqueAnnotation(CGtechnique, const char *);
CGDLL_API CGannotation CGENTRY cgGetNamedPassAnnotation(CGpass, const char *);
CGDLL_API CGannotation CGENTRY cgGetNamedParameterAnnotation(CGparameter, const char *);
CGDLL_API CGannotation CGENTRY cgGetNamedProgramAnnotation(CGprogram, const char *);
CGDLL_API CGannotation CGENTRY cgGetNamedEffectAnnotation(CGeffect, const char *);

CGDLL_API CGbool CGENTRY cgIsAnnotation(CGannotation);

CGDLL_API const char * CGENTRY cgGetAnnotationName(CGannotation);
CGDLL_API CGtype CGENTRY cgGetAnnotationType(CGannotation);

CGDLL_API const float * CGENTRY cgGetFloatAnnotationValues(CGannotation, int *nvalues);
CGDLL_API const int * CGENTRY cgGetIntAnnotationValues(CGannotation, int *nvalues);
CGDLL_API const char * CGENTRY cgGetStringAnnotationValue(CGannotation);
CGDLL_API const CGbool * CGENTRY cgGetBoolAnnotationValues(CGannotation, int *nvalues);
CGDLL_API const int * CGENTRY cgGetBooleanAnnotationValues(CGannotation, int *nvalues);

CGDLL_API int CGENTRY cgGetNumDependentAnnotationParameters(CGannotation);
CGDLL_API CGparameter CGENTRY cgGetDependentAnnotationParameter(CGannotation, int index);

CGDLL_API void CGENTRY cgEvaluateProgram(CGprogram, float *, int ncomps, int nx, int ny, int nz);

/*** Cg 1.5 Additions ***/

CGDLL_API CGbool CGENTRY cgSetEffectName(CGeffect, const char *name);
CGDLL_API const char * CGENTRY cgGetEffectName(CGeffect);
CGDLL_API CGeffect CGENTRY cgGetNamedEffect(CGcontext, const char *name);
CGDLL_API CGparameter CGENTRY cgCreateEffectParameter(CGeffect, const char *name, CGtype);

CGDLL_API CGtechnique CGENTRY cgCreateTechnique(CGeffect, const char *name);

CGDLL_API CGparameter CGENTRY cgCreateEffectParameterArray(CGeffect, const char *name, CGtype type, int length); 
CGDLL_API CGparameter CGENTRY cgCreateEffectParameterMultiDimArray(CGeffect, const char *name, CGtype type, int dim, const int *lengths); 

CGDLL_API CGpass CGENTRY cgCreatePass(CGtechnique, const char *name);

CGDLL_API CGstateassignment CGENTRY cgCreateStateAssignment(CGpass, CGstate);
CGDLL_API CGstateassignment CGENTRY cgCreateStateAssignmentIndex(CGpass, CGstate, int index);
CGDLL_API CGstateassignment CGENTRY cgCreateSamplerStateAssignment(CGparameter, CGstate);

CGDLL_API CGbool CGENTRY cgSetFloatStateAssignment(CGstateassignment, float);
CGDLL_API CGbool CGENTRY cgSetIntStateAssignment(CGstateassignment, int);
CGDLL_API CGbool CGENTRY cgSetBoolStateAssignment(CGstateassignment, CGbool);
CGDLL_API CGbool CGENTRY cgSetStringStateAssignment(CGstateassignment, const char *);
CGDLL_API CGbool CGENTRY cgSetProgramStateAssignment(CGstateassignment, CGprogram);
CGDLL_API CGbool CGENTRY cgSetSamplerStateAssignment(CGstateassignment, CGparameter);
CGDLL_API CGbool CGENTRY cgSetTextureStateAssignment(CGstateassignment, CGparameter);

CGDLL_API CGbool CGENTRY cgSetFloatArrayStateAssignment(CGstateassignment, const float *vals);
CGDLL_API CGbool CGENTRY cgSetIntArrayStateAssignment(CGstateassignment, const int *vals);
CGDLL_API CGbool CGENTRY cgSetBoolArrayStateAssignment(CGstateassignment, const CGbool *vals);

CGDLL_API CGannotation CGENTRY cgCreateTechniqueAnnotation(CGtechnique, const char *name, CGtype);
CGDLL_API CGannotation CGENTRY cgCreatePassAnnotation(CGpass, const char *name, CGtype);
CGDLL_API CGannotation CGENTRY cgCreateParameterAnnotation(CGparameter, const char *name, CGtype);
CGDLL_API CGannotation CGENTRY cgCreateProgramAnnotation(CGprogram, const char *name, CGtype);
CGDLL_API CGannotation CGENTRY cgCreateEffectAnnotation(CGeffect, const char *name, CGtype);

CGDLL_API CGbool CGENTRY cgSetIntAnnotation(CGannotation, int value);
CGDLL_API CGbool CGENTRY cgSetFloatAnnotation(CGannotation, float value);
CGDLL_API CGbool CGENTRY cgSetBoolAnnotation(CGannotation, CGbool value);
CGDLL_API CGbool CGENTRY cgSetStringAnnotation(CGannotation, const char *value);

CGDLL_API const char * CGENTRY cgGetStateEnumerantName(CGstate, int value);
CGDLL_API int CGENTRY cgGetStateEnumerantValue(CGstate, const char *name);

CGDLL_API CGeffect CGENTRY cgGetParameterEffect(CGparameter param);

CGDLL_API CGparameterclass CGENTRY cgGetTypeClass(CGtype type);
CGDLL_API CGtype CGENTRY cgGetTypeBase(CGtype type);
CGDLL_API CGbool CGENTRY cgGetTypeSizes(CGtype type, int *nrows, int *ncols);
CGDLL_API void CGENTRY cgGetMatrixSize(CGtype type, int *nrows, int *ncols);

CGDLL_API int CGENTRY cgGetNumProgramDomains( CGprogram program );
CGDLL_API CGdomain CGENTRY cgGetProfileDomain( CGprofile profile );
CGDLL_API CGprogram CGENTRY cgCombinePrograms( int n, const CGprogram *exeList );
CGDLL_API CGprogram CGENTRY cgCombinePrograms2( const CGprogram exe1, const CGprogram exe2 );
CGDLL_API CGprogram CGENTRY cgCombinePrograms3( const CGprogram exe1, const CGprogram exe2, const CGprogram exe3 );
CGDLL_API CGprofile CGENTRY cgGetProgramDomainProfile(CGprogram program, int index);

#endif

#ifdef __cplusplus
}
#endif

#ifdef CG_APIENTRY_DEFINED
# undef CG_APIENTRY_DEFINED
# undef APIENTRY
#endif

#ifdef CG_WINGDIAPI_DEFINED
# undef CG_WINGDIAPI_DEFINED
# undef WINGDIAPI
#endif

#endif
