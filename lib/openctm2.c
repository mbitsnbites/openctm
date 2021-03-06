//-----------------------------------------------------------------------------
// Product:     OpenCTM
// File:        openctm2.c
// Description: API functions.
//-----------------------------------------------------------------------------
// Copyright (c) 2009-2013 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//     1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//     2. Altered source versions must be plainly marked as such, and must not
//     be misrepresented as being the original software.
//
//     3. This notice may not be removed or altered from any source
//     distribution.
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "openctm2.h"
#include "internal.h"


// The C99 macro isfinite() is not supported on all platforms (specifically,
// MS Visual Studio does not support C99)
#if !defined(isfinite) && defined(_MSC_VER)
  #include <float.h>
  #define isfinite(x) _finite(x)
#endif


//=============================================================================
// Private internal helper functions.
//=============================================================================

//-----------------------------------------------------------------------------
// _ctmAllocateFloatMaps()
//-----------------------------------------------------------------------------
static CTMuint _ctmAllocateFloatMaps(_CTMcontext * self,
  _CTMfloatmap ** aMapListPtr, CTMuint aCount)
{
  _CTMfloatmap ** mapListPtr;
  CTMuint i;

  mapListPtr = aMapListPtr;
  for(i = 0; i < aCount; ++ i)
  {
    // Allocate & clear memory for this map
    *mapListPtr = (_CTMfloatmap *) malloc(sizeof(_CTMfloatmap));
    if(!*mapListPtr)
    {
      self->mError = CTM_OUT_OF_MEMORY;
      return CTM_FALSE;
    }
    memset(*mapListPtr, 0, sizeof(_CTMfloatmap));

    // Clear the array
    _ctmClearArray(&(*mapListPtr)->mArray);

    // Next map...
    mapListPtr = &(*mapListPtr)->mNext;
  }

  return CTM_TRUE;
}

#ifdef _CTM_SUPPORT_SAVE
//-----------------------------------------------------------------------------
// _ctmAddFloatMap()
//-----------------------------------------------------------------------------
static _CTMfloatmap * _ctmAddFloatMap(_CTMcontext * self,
  const char * aName, const char * aFileName,
  _CTMfloatmap ** aList)
{
  _CTMfloatmap * map;
  CTMuint len;

  // Allocate memory for a new map list item and append it to the list
  if(!*aList)
  {
    *aList = (_CTMfloatmap *) malloc(sizeof(_CTMfloatmap));
    map = *aList;
  }
  else
  {
    map = *aList;
    while(map->mNext)
      map = map->mNext;
    map->mNext = (_CTMfloatmap *) malloc(sizeof(_CTMfloatmap));
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return (_CTMfloatmap *) 0;
  }

  // Init the map item
  memset(map, 0, sizeof(_CTMfloatmap));
  map->mPrecision = 1.0f / 1024.0f;

  // Set name of the map
  if(aName)
  {
    // Get length of string (if empty, do nothing)
    len = (CTMuint) strlen(aName);
    if(len)
    {
      // Copy the string
      map->mName = (char *) malloc(len + 1);
      if(!map->mName)
      {
        self->mError = CTM_OUT_OF_MEMORY;
        free(map);
        return (_CTMfloatmap *) 0;
      }
      strcpy(map->mName, aName);
    }
  }

  // Set file name reference for the map
  if(aFileName)
  {
    // Get length of string (if empty, do nothing)
    len = (CTMuint) strlen(aFileName);
    if(len)
    {
      // Copy the string
      map->mFileName = (char *) malloc(len + 1);
      if(!map->mFileName)
      {
        self->mError = CTM_OUT_OF_MEMORY;
        if(map->mName)
          free(map->mName);
        free(map);
        return (_CTMfloatmap *) 0;
      }
      strcpy(map->mFileName, aFileName);
    }
  }

  return map;
}
#endif // _CTM_SUPPORT_SAVE

//-----------------------------------------------------------------------------
// _ctmFreeMapList() - Free a float map list.
//-----------------------------------------------------------------------------
static void _ctmFreeMapList(_CTMfloatmap * aMapList)
{
  _CTMfloatmap * map, * nextMap;
  map = aMapList;
  while(map)
  {
    // Free map name
    if(map->mName)
      free(map->mName);

    // Free file name
    if(map->mFileName)
      free(map->mFileName);

    nextMap = map->mNext;
    free(map);
    map = nextMap;
  }
}

//-----------------------------------------------------------------------------
// _ctmFreeContextData() - Clear all the context data in a CTM context,
// and clear external mesh array assignments.
//-----------------------------------------------------------------------------
static void _ctmFreeContextData(_CTMcontext * self)
{
  // Clear external mesh array assignments
  _ctmClearArray(&self->mVertices);
  self->mVertexCount = 0;
  _ctmClearArray(&self->mIndices);
  self->mTriangleCount = 0;
  _ctmClearArray(&self->mNormals);

  // Free UV coordinate map list
  _ctmFreeMapList(self->mUVMaps);
  self->mUVMaps = (_CTMfloatmap *) 0;
  self->mUVMapCount = 0;

  // Free attribute map list
  _ctmFreeMapList(self->mAttribMaps);
  self->mAttribMaps = (_CTMfloatmap *) 0;
  self->mAttribMapCount = 0;

  // Free the file comment
  if(self->mFileComment)
    free(self->mFileComment);

#ifdef _CTM_SUPPORT_V5_FILES
  // Free v5 compatibility data
  _ctmCleanupV5Data(self);
#endif
}

//-----------------------------------------------------------------------------
// _ctmCheckMeshIntegrity() - Check if a mesh is valid (i.e. is non-empty, and
// contains valid data).
//-----------------------------------------------------------------------------
static CTMint _ctmCheckMeshIntegrity(_CTMcontext * self)
{
  CTMuint i, j;
  _CTMfloatmap * map;

  // Check that we have all the mandatory data
  if(!self->mVertices.mData || !self->mIndices.mData ||
     (self->mVertexCount < 1) || (self->mTriangleCount < 1))
  {
    return CTM_FALSE;
  }

  // Check that all indices are within range
  for(i = 0; i < self->mTriangleCount; ++ i)
  {
    for(j = 0; j < 3; ++ j)
    {
      if(self->mIndices.geti(&self->mIndices, i, j) >= self->mVertexCount)
        return CTM_FALSE;
    }
  }

  // Check that all vertices and normals are finite (non-NaN, non-inf)
  for(i = 0; i < self->mVertexCount; ++ i)
  {
    for(j = 0; j < 3; ++ j)
    {
      if(!isfinite(self->mVertices.getf(&self->mVertices, i, j)))
        return CTM_FALSE;
      if(self->mHasNormals && !isfinite(self->mNormals.getf(&self->mNormals, i, j)))
        return CTM_FALSE;
    }
  }

  // Check that all UV maps are finite (non-NaN, non-inf)
  map = self->mUVMaps;
  while(map)
  {
    for(i = 0; i < self->mVertexCount; ++ i)
    {
      for(j = 0; j < 2; ++ j)
      {
        if(!isfinite(map->mArray.getf(&map->mArray, i, j)))
          return CTM_FALSE;
      }
    }
    map = map->mNext;
  }

  // Check that all attribute maps are finite (non-NaN, non-inf)
  map = self->mAttribMaps;
  while(map)
  {
    for(i = 0; i < self->mVertexCount; ++ i)
    {
      for(j = 0; j < 4; ++ j)
      {
        if(!isfinite(map->mArray.getf(&map->mArray, i, j)))
          return CTM_FALSE;
      }
    }
    map = map->mNext;
  }

  return CTM_TRUE;
}

//-----------------------------------------------------------------------------
// _ctmDefaultRead()
//-----------------------------------------------------------------------------
static CTMuint CTMCALL _ctmDefaultRead(void * aBuf, CTMuint aCount,
  void * aUserData)
{
  return (CTMuint) fread(aBuf, 1, (size_t) aCount, (FILE *) aUserData);
}

#ifdef _CTM_SUPPORT_SAVE
//-----------------------------------------------------------------------------
// _ctmDefaultWrite()
//-----------------------------------------------------------------------------
static CTMuint CTMCALL _ctmDefaultWrite(const void * aBuf, CTMuint aCount,
  void * aUserData)
{
  return (CTMuint) fwrite(aBuf, 1, (size_t) aCount, (FILE *) aUserData);
}
#endif


//=============================================================================
// Public API functions.
//=============================================================================

//-----------------------------------------------------------------------------
// ctmNewContext()
//-----------------------------------------------------------------------------
CTMEXPORT CTMcontext CTMCALL ctmNewContext(CTMenum aMode)
{
  _CTMcontext * self;

#ifndef _CTM_SUPPORT_SAVE
  if(aMode == CTM_EXPORT)
    return (CTMcontext) 0;
#endif

  // Allocate memory for the new structure
  self = (_CTMcontext *) malloc(sizeof(_CTMcontext));
  if(!self)
    return (CTMcontext) 0;

  // Initialize structure (set null pointers and zero array lengths)
  memset(self, 0, sizeof(_CTMcontext));
  self->mMode = aMode;
  self->mFrameCount = 1;
  self->mCurrentFrame = -1;
  self->mError = CTM_NONE;
#if defined(_CTM_SUPPORT_MG1)
  self->mMethod = CTM_METHOD_MG1;
#elif defined(_CTM_SUPPORT_RAW)
  self->mMethod = CTM_METHOD_RAW;
#else
  self->mMethod = CTM_METHOD_MG2;
#endif
  self->mCompressionLevel = _CTM_DEFAULT_LZMA_LEVEL;
  self->mVertexPrecision = _CTM_DEFAULT_VERTEX_PRECISION;
  self->mNormalPrecision = _CTM_DEFAULT_NORMAL_PRECISION;

  return (CTMcontext) self;
}

//-----------------------------------------------------------------------------
// ctmFreeContext()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmFreeContext(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Close the file stream, if necessary
  if(self->mFileStream)
    fclose(self->mFileStream);

  // Free all mesh resources
  _ctmFreeContextData(self);

  // Free the context
  free(self);
}

//-----------------------------------------------------------------------------
// ctmGetError()
//-----------------------------------------------------------------------------
CTMEXPORT CTMenum CTMCALL ctmGetError(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMenum err;

  if(!self) return CTM_INVALID_CONTEXT;

  // Get error code and reset error state
  err = self->mError;
  self->mError = CTM_NONE;
  return err;
}

//-----------------------------------------------------------------------------
// ctmErrorString()
//-----------------------------------------------------------------------------
CTMEXPORT const char * CTMCALL ctmErrorString(CTMenum aError)
{
  switch(aError)
  {
    case CTM_INVALID_CONTEXT:
      return "Invalid context";
    case CTM_INVALID_ARGUMENT:
      return "Invalid argument";
    case CTM_INVALID_OPERATION:
      return "Invalid operation";
    case CTM_INVALID_MESH:
      return "Invalid mesh";
    case CTM_OUT_OF_MEMORY:
      return "Out of memory";
    case CTM_FILE_ERROR:
      return "File I/O error";
    case CTM_BAD_FORMAT:
      return "Bad format";
    case CTM_LZMA_ERROR:
      return "LZMA error";
    case CTM_INTERNAL_ERROR:
      return "Internal error";
    case CTM_UNSUPPORTED_FORMAT_VERSION:
      return "Unsupported format version";
    case CTM_UNSUPPORTED_OPERATION:
      return "Unsupported operation";
    default:
      return "Unknown error code";
  }
}

//-----------------------------------------------------------------------------
// ctmGetBoolean()
//-----------------------------------------------------------------------------
CTMEXPORT CTMbool CTMCALL ctmGetBoolean(CTMcontext aContext, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return CTM_FALSE;

  switch(aProperty)
  {
    case CTM_HAS_NORMALS:
      return self->mHasNormals ? CTM_TRUE : CTM_FALSE;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return CTM_FALSE;
}

//-----------------------------------------------------------------------------
// ctmGetInteger()
//-----------------------------------------------------------------------------
CTMEXPORT CTMuint CTMCALL ctmGetInteger(CTMcontext aContext, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return 0;

  switch(aProperty)
  {
    case CTM_VERTEX_COUNT:
      return self->mVertexCount;

    case CTM_TRIANGLE_COUNT:
      return self->mTriangleCount;

    case CTM_UV_MAP_COUNT:
      return self->mUVMapCount;

    case CTM_ATTRIB_MAP_COUNT:
      return self->mAttribMapCount;

    case CTM_COMPRESSION_METHOD:
      return (CTMuint) self->mMethod;

    case CTM_FRAME_COUNT:
      return self->mFrameCount;

    case CTM_FRAME_INDEX:
      return self->mCurrentFrame - 1;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return 0;
}

//-----------------------------------------------------------------------------
// ctmGetFloat()
//-----------------------------------------------------------------------------
CTMEXPORT CTMfloat CTMCALL ctmGetFloat(CTMcontext aContext, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return 0.0f;

  switch(aProperty)
  {
    case CTM_VERTEX_PRECISION:
      return self->mVertexPrecision;

    case CTM_NORMAL_PRECISION:
      return self->mNormalPrecision;

    case CTM_FRAME_TIME:
      return self->mFrameTime;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return 0.0f;
}

//-----------------------------------------------------------------------------
// ctmGetString()
//-----------------------------------------------------------------------------
CTMEXPORT const char * CTMCALL ctmGetString(CTMcontext aContext,
  CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return 0;

  switch(aProperty)
  {
    case CTM_FILE_COMMENT:
      return (const char *) self->mFileComment;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (const char *) 0;
}

//-----------------------------------------------------------------------------
// ctmGetNamedUVMap()
//-----------------------------------------------------------------------------
CTMEXPORT CTMenum CTMCALL ctmGetNamedUVMap(CTMcontext aContext,
  const char * aName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint result;
  if(!self) return CTM_NONE;

  map = self->mUVMaps;
  result = CTM_UV_MAP_1;
  while(map && (strcmp(aName, map->mName) != 0))
  {
    map = map->mNext;
    ++ result;
  }
  if(!map)
  {
    return CTM_NONE;
  }
  return (CTMenum)result;
}

//-----------------------------------------------------------------------------
// ctmGetNamedAttribMap()
//-----------------------------------------------------------------------------
CTMEXPORT CTMenum CTMCALL ctmGetNamedAttribMap(CTMcontext aContext,
  const char * aName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint result;
  if(!self) return CTM_NONE;

  map = self->mAttribMaps;
  result = CTM_ATTRIB_MAP_1;
  while(map && (strcmp(aName, map->mName) != 0))
  {
    map = map->mNext;
    ++ result;
  }
  if(!map)
  {
    return CTM_NONE;
  }
  return (CTMenum)result;
}

//-----------------------------------------------------------------------------
// ctmGetUVMapString()
//-----------------------------------------------------------------------------
CTMEXPORT const char * CTMCALL ctmGetUVMapString(CTMcontext aContext,
  CTMenum aUVMap, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint i;
  if(!self) return (const char *) 0;

  // Find the indicated map
  map = self->mUVMaps;
  i = CTM_UV_MAP_1;
  while(map && (i != aUVMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return (const char *) 0;
  }

  // Get the requested string
  switch(aProperty)
  {
    case CTM_NAME:
      return (const char *) map->mName;

    case CTM_FILE_NAME:
      return (const char *) map->mFileName;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (const char *) 0;
}

//-----------------------------------------------------------------------------
// ctmGetUVMapFloat()
//-----------------------------------------------------------------------------
CTMEXPORT CTMfloat CTMCALL ctmGetUVMapFloat(CTMcontext aContext,
  CTMenum aUVMap, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint i;
  if(!self) return 0.0f;

  // Find the indicated map
  map = self->mUVMaps;
  i = CTM_UV_MAP_1;
  while(map && (i != aUVMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return 0.0f;
  }

  // Get the requested string
  switch(aProperty)
  {
    case CTM_PRECISION:
      return map->mPrecision;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return 0.0f;
}

//-----------------------------------------------------------------------------
// ctmGetAttribMapString()
//-----------------------------------------------------------------------------
CTMEXPORT const char * CTMCALL ctmGetAttribMapString(CTMcontext aContext,
  CTMenum aAttribMap, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint i;
  if(!self) return (const char *) 0;

  // Find the indicated map
  map = self->mAttribMaps;
  i = CTM_ATTRIB_MAP_1;
  while(map && (i != aAttribMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return (const char *) 0;
  }

  // Get the requested string
  switch(aProperty)
  {
    case CTM_NAME:
      return (const char *) map->mName;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return (const char *) 0;
}

//-----------------------------------------------------------------------------
// ctmGetAttribMapFloat()
//-----------------------------------------------------------------------------
CTMEXPORT CTMfloat CTMCALL ctmGetAttribMapFloat(CTMcontext aContext,
  CTMenum aAttribMap, CTMenum aProperty)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  CTMuint i;
  if(!self) return 0.0f;

  // Find the indicated map
  map = self->mAttribMaps;
  i = CTM_ATTRIB_MAP_1;
  while(map && (i != aAttribMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return 0.0f;
  }

  // Get the requested string
  switch(aProperty)
  {
    case CTM_PRECISION:
      return map->mPrecision;

    default:
      self->mError = CTM_INVALID_ARGUMENT;
  }

  return 0.0f;
}

//-----------------------------------------------------------------------------
// ctmVertexCount()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmVertexCount(CTMcontext aContext, CTMuint aCount)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change the vertex count in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  self->mVertexCount = aCount;
#else
  DUMMYUSE(aCount);
  self->mError = CTM_INVALID_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmTriangleCount()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmTriangleCount(CTMcontext aContext, CTMuint aCount)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change the triangle count in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  self->mTriangleCount = aCount;
#else
  DUMMYUSE(aCount);
  self->mError = CTM_INVALID_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmAddUVMap()
//-----------------------------------------------------------------------------
CTMEXPORT CTMenum CTMCALL ctmAddUVMap(CTMcontext aContext, const char * aName,
  const char * aFileName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  _CTMfloatmap * map;
#endif
  if(!self) return CTM_NONE;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to add UV maps in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return CTM_NONE;
  }

  // Add a new UV map to the UV map list
  map = _ctmAddFloatMap(self, aName, aFileName, &self->mUVMaps);
  if(!map)
    return CTM_NONE;
  else
  {
    // Set the default UV coordinate precision
    map->mPrecision = _CTM_DEFAULT_UV_PRECISION;
    ++ self->mUVMapCount;
    return (CTMenum)(CTM_UV_MAP_1 + self->mUVMapCount - 1);
  }
#else
  DUMMYUSE(aName);
  DUMMYUSE(aFileName);
  self->mError = CTM_UNSUPPORTED_OPERATION;
  return CTM_NONE;
#endif
}

//-----------------------------------------------------------------------------
// ctmAddAttribMap()
//-----------------------------------------------------------------------------
CTMEXPORT CTMenum CTMCALL ctmAddAttribMap(CTMcontext aContext,
  const char * aName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  _CTMfloatmap * map;
#endif
  if(!self) return CTM_NONE;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to add attribute maps in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return CTM_NONE;
  }

  // Add a new attribute map to the attribute map list
  map = _ctmAddFloatMap(self, aName, (const char *) 0, &self->mAttribMaps);
  if(!map)
    return CTM_NONE;
  else
  {
    // Set the default vertex attribute precision
    map->mPrecision = _CTM_DEFAULT_ATTRIB_PRECISION;
    ++ self->mAttribMapCount;
    return (CTMenum)(CTM_ATTRIB_MAP_1 + self->mAttribMapCount - 1);
  }
#else
  DUMMYUSE(aName);
  self->mError = CTM_UNSUPPORTED_OPERATION;
  return CTM_NONE;
#endif
}

//-----------------------------------------------------------------------------
// ctmArrayPointer()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmArrayPointer(CTMcontext aContext, CTMenum aTarget,
  CTMuint aSize, CTMenum aType, CTMuint aStride, void * aData)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  _CTMfloatmap * map;
  _CTMarray * array = (_CTMarray *) 0;
  CTMuint i;
  if(!self) return;

  // Get the array handle for the selected target array, and check the aSize
  // argument
  if(aTarget == CTM_INDICES)
  {
    // You are only allowed to change the index pointer for the first frame
    if(self->mCurrentFrame >= 1)
    {
      self->mError = CTM_INVALID_OPERATION;
      return;
    }
    if(aSize != 3)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return;
    }
    array = &self->mIndices;
  }
  else if(aTarget == CTM_VERTICES)
  {
    if(aSize != 3)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return;
    }
    array = &self->mVertices;
  }
  else if(aTarget == CTM_NORMALS)
  {
    if(aSize != 3)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return;
    }
    array = &self->mNormals;
    if(self->mMode == CTM_EXPORT)
      self->mHasNormals = aData ? CTM_TRUE : CTM_FALSE;
  }
  else if((aTarget >= CTM_UV_MAP_1) && (aTarget <= CTM_UV_MAP_LAST))
  {
    if(aSize != 2)
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return;
    }
    map = self->mUVMaps;
    i = CTM_UV_MAP_1;
    while(map && (i != aTarget))
    {
      map = map->mNext;
      ++ i;
    }
    if(map)
      array = &map->mArray;
  }
  else if((aTarget >= CTM_ATTRIB_MAP_1) && (aTarget <= CTM_ATTRIB_MAP_LAST))
  {
    if((aSize < 1) || (aSize > 4))
    {
      self->mError = CTM_INVALID_ARGUMENT;
      return;
    }
    map = self->mAttribMaps;
    i = CTM_ATTRIB_MAP_1;
    while(map && (i != aTarget))
    {
      map = map->mNext;
      ++ i;
    }
    if(map)
      array = &map->mArray;
  }
  else
  {
    // Unsupported target
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set up array
  CTMenum err = _ctmInitArray(array, aSize, aType, aStride, aData);
  if (err != CTM_NONE) {
    self->mError = err;
  }
}

//-----------------------------------------------------------------------------
// ctmFileComment()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmFileComment(CTMcontext aContext,
  const char * aFileComment)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  int len;
#endif
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change file attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Free the old comment string, if necessary
  if(self->mFileComment)
  {
    free(self->mFileComment);
    self->mFileComment = (char *) 0;
  }

  // Get length of string (if empty, do nothing)
  if(!aFileComment)
    return;
  len = (CTMuint) strlen(aFileComment);
  if(!len)
    return;

  // Copy the string
  self->mFileComment = (char *) malloc(len + 1);
  if(!self->mFileComment)
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }
  strcpy(self->mFileComment, aFileComment);
#else
  DUMMYUSE(aFileComment);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmFrameCount()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmFrameCount(CTMcontext aContext, CTMuint aCount)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change the animation frame count in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Set frame count
  if(aCount < 1)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }
  self->mFrameCount = aCount;
#else
  DUMMYUSE(aCount);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmCompressionMethod()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmCompressionMethod(CTMcontext aContext,
  CTMenum aMethod)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check if this is a supported operation
#ifndef _CTM_SUPPORT_RAW
  if(aMethod == CTM_METHOD_RAW)
  {
    self->mError = CTM_UNSUPPORTED_OPERATION;
    return;
  }
#endif
#ifndef _CTM_SUPPORT_MG1
  if(aMethod == CTM_METHOD_MG1)
  {
    self->mError = CTM_UNSUPPORTED_OPERATION;
    return;
  }
#endif
#ifndef _CTM_SUPPORT_MG2
  if(aMethod == CTM_METHOD_MG2)
  {
    self->mError = CTM_UNSUPPORTED_OPERATION;
    return;
  }
#endif

  // Check arguments
  if((aMethod != CTM_METHOD_RAW) && (aMethod != CTM_METHOD_MG1) &&
     (aMethod != CTM_METHOD_MG2))
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set method
  self->mMethod = aMethod;
#else
  DUMMYUSE(aMethod);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmCompressionLevel()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmCompressionLevel(CTMcontext aContext,
  CTMuint aLevel)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aLevel > 9)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set the compression level
  self->mCompressionLevel = aLevel;
#else
  DUMMYUSE(aLevel);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmVertexPrecision()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmVertexPrecision(CTMcontext aContext,
  CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aPrecision <= 0.0f)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set precision
  self->mVertexPrecision = aPrecision;
#else
  DUMMYUSE(aPrecision);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmVertexPrecisionRel()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmVertexPrecisionRel(CTMcontext aContext,
  CTMfloat aRelPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  CTMfloat avgEdgeLength, p1[3], p2[3];
  CTMuint edgeCount, i, j, k, idx[3];
#endif
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aRelPrecision <= 0.0f)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Check that the index array and vertex array have been defined
  if((!self->mIndices.mData) || (!self->mVertices.mData))
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }

  // Calculate the average edge length (Note: we actually sum up all the half-
  // edges, so in a proper solid mesh all connected edges are counted twice)
  avgEdgeLength = 0.0f;
  edgeCount = 0;
  for(i = 0; i < self->mTriangleCount; ++ i)
  {
    for(j = 0; j < 3; ++ j)
      idx[j] = self->mIndices.geti(&self->mIndices, i, j);
    for(k = 0; k < 3; ++ k)
      p1[k] = self->mVertices.getf(&self->mVertices, idx[2], k);
    for(j = 0; j < 3; ++ j)
    {
      for(k = 0; k < 3; ++ k)
        p2[k] = self->mVertices.getf(&self->mVertices, idx[j], k);
      avgEdgeLength += sqrtf((p2[0] - p1[0]) * (p2[0] - p1[0]) +
                             (p2[1] - p1[1]) * (p2[1] - p1[1]) +
                             (p2[2] - p1[2]) * (p2[2] - p1[2]));
      for(k = 0; k < 3; ++ k)
        p1[k] = p2[k];
      ++ edgeCount;
    }
  }
  if(edgeCount == 0)
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }
  avgEdgeLength /= (CTMfloat) edgeCount;

  // Set precision
  self->mVertexPrecision = aRelPrecision * avgEdgeLength;
#else
  DUMMYUSE(aRelPrecision);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmNormalPrecision()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmNormalPrecision(CTMcontext aContext,
  CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aPrecision <= 0.0f)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Set precision
  self->mNormalPrecision = aPrecision;
#else
  DUMMYUSE(aPrecision);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmUVCoordPrecision()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmUVCoordPrecision(CTMcontext aContext,
  CTMenum aUVMap, CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  _CTMfloatmap * map;
  CTMuint i;
#endif
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aPrecision <= 0.0f)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Find the indicated map
  map = self->mUVMaps;
  i = CTM_UV_MAP_1;
  while(map && (i != aUVMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Update the precision
  map->mPrecision = aPrecision;
#else
  DUMMYUSE(aUVMap);
  DUMMYUSE(aPrecision);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmAttribPrecision()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmAttribPrecision(CTMcontext aContext,
  CTMenum aAttribMap, CTMfloat aPrecision)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  _CTMfloatmap * map;
  CTMuint i;
#endif
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // You are only allowed to change compression attributes in export mode
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check arguments
  if(aPrecision <= 0.0f)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Find the indicated map
  map = self->mAttribMaps;
  i = CTM_ATTRIB_MAP_1;
  while(map && (i != aAttribMap))
  {
    ++ i;
    map = map->mNext;
  }
  if(!map)
  {
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }

  // Update the precision
  map->mPrecision = aPrecision;
#else
  DUMMYUSE(aAttribMap);
  DUMMYUSE(aPrecision);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmOpenReadFile()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmOpenReadFile(CTMcontext aContext,
  const char * aFileName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Are we allowed to read the file?
  if((self->mMode != CTM_IMPORT) || (self->mCurrentFrame >= 0) ||
     self->mFileStream)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Open file stream
  self->mFileStream = fopen(aFileName, "rb");
  if(!self->mFileStream)
  {
    self->mError = CTM_FILE_ERROR;
    return;
  }

  // ...continue with the custom function
  ctmOpenReadCustom(aContext, _ctmDefaultRead, self->mFileStream);
}

//-----------------------------------------------------------------------------
// ctmOpenReadCustom()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmOpenReadCustom(CTMcontext aContext,
  CTMreadfn aReadFn, void * aUserData)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  CTMuint flags, method;
  _CTMfloatmap * map;
  if(!self) return;

  // Are we allowed to read the file?
  if((self->mMode != CTM_IMPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Initialize stream
  self->mReadFn = aReadFn;
  self->mUserData = aUserData;

  // Clear any old mesh data
  _ctmFreeContextData(self);

  // Read magic ID and file version number from the stream
  if(_ctmStreamReadUINT(self) != FOURCC("OCTM"))
  {
    self->mError = CTM_BAD_FORMAT;
    return;
  }
  self->mFormatVersion = _ctmStreamReadUINT(self);

  // Check if this is a supported version
#ifdef _CTM_SUPPORT_V5_FILES
  if(self->mFormatVersion == 5)
  {
    if(!_ctmLoadV5FileToMem(self))
      return;
  }
  else
#endif
  if(self->mFormatVersion != _CTM_FORMAT_VERSION)
  {
    self->mError = CTM_UNSUPPORTED_FORMAT_VERSION;
    return;
  }

  // Read the rest of the header
  method = _ctmStreamReadUINT(self);
  if(method == FOURCC("RAW\0"))
    self->mMethod = CTM_METHOD_RAW;
  else if(method == FOURCC("MG1\0"))
    self->mMethod = CTM_METHOD_MG1;
  else if(method == FOURCC("MG2\0"))
    self->mMethod = CTM_METHOD_MG2;
  else
  {
    self->mError = CTM_BAD_FORMAT;
    return;
  }
  self->mVertexCount = _ctmStreamReadUINT(self);
  if(self->mVertexCount == 0)
  {
    self->mError = CTM_BAD_FORMAT;
    return;
  }
  self->mTriangleCount = _ctmStreamReadUINT(self);
  if(self->mTriangleCount == 0)
  {
    self->mError = CTM_BAD_FORMAT;
    return;
  }
  self->mUVMapCount = _ctmStreamReadUINT(self);
  self->mAttribMapCount = _ctmStreamReadUINT(self);
  flags = _ctmStreamReadUINT(self);
  self->mFrameCount = _ctmStreamReadUINT(self);
  if(self->mFrameCount == 0)
  {
    self->mError = CTM_BAD_FORMAT;
    return;
  }
  _ctmStreamReadSTRING(self, &self->mFileComment);

  // Decode the flags field
  self->mHasNormals = (flags & _CTM_HAS_NORMALS_BIT) ? CTM_TRUE : CTM_FALSE;

  // Allocate memory for the UV and attribute maps (if any)
  if(!_ctmAllocateFloatMaps(self, &self->mUVMaps, self->mUVMapCount))
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }
  if(!_ctmAllocateFloatMaps(self, &self->mAttribMaps, self->mAttribMapCount))
  {
    self->mError = CTM_OUT_OF_MEMORY;
    return;
  }

  // Read UV map info
  if(self->mUVMaps)
  {
    if(_ctmStreamReadUINT(self) != FOURCC("UINF"))
    {
      self->mError = CTM_BAD_FORMAT;
      return;
    }
    map = self->mUVMaps;
    while(map)
    {
      _ctmStreamReadSTRING(self, &map->mName);
      _ctmStreamReadSTRING(self, &map->mFileName);
      map = map->mNext;
    }
  }

  // Read attribute map info
  if(self->mAttribMaps)
  {
    if(_ctmStreamReadUINT(self) != FOURCC("AINF"))
    {
      self->mError = CTM_BAD_FORMAT;
      return;
    }
    map = self->mAttribMaps;
    while(map)
    {
      _ctmStreamReadSTRING(self, &map->mName);
      map = map->mNext;
    }
  }

  // Reset the frame counter (no frames have been read yet)
  self->mCurrentFrame = 0;

  // Reset the frame time
  self->mFrameTime = 0.0f;
}

//-----------------------------------------------------------------------------
// ctmReadMesh()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmReadMesh(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Are we allowed to read the first frame?
  if((self->mMode != CTM_IMPORT) || (self->mCurrentFrame != 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Animation properties for the first frame
  self->mFrameTime = 0.0f;

  // Uncompress from stream
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
#ifdef _CTM_SUPPORT_RAW
      _ctmUncompressMesh_RAW(self);
      break;
#else
      _ctmFreeContextData(self);
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    case CTM_METHOD_MG1:
#ifdef _CTM_SUPPORT_MG1
      _ctmUncompressMesh_MG1(self);
      break;
#else
      _ctmFreeContextData(self);
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    case CTM_METHOD_MG2:
#ifdef _CTM_SUPPORT_MG2
      _ctmUncompressMesh_MG2(self);
      break;
#else
      _ctmFreeContextData(self);
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    default:
      self->mError = CTM_INTERNAL_ERROR;
  }

  // We are done with the frame, on to the next...
  ++ self->mCurrentFrame;

  // Check mesh integrity
  if(!_ctmCheckMeshIntegrity(self))
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }
}

//-----------------------------------------------------------------------------
// ctmReadNextFrame()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmReadNextFrame(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Are we allowed to read the next frame?
  if((self->mMode != CTM_IMPORT) || (self->mCurrentFrame < 1) ||
     (self->mCurrentFrame >= self->mFrameCount))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Read frame header
  self->mFrameTime = _ctmStreamReadFLOAT(self);

  // Uncompress from stream
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
#ifdef _CTM_SUPPORT_RAW
      _ctmUncompressFrame_RAW(self);
      break;
#else
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    case CTM_METHOD_MG1:
#ifdef _CTM_SUPPORT_MG1
      _ctmUncompressFrame_MG1(self);
      break;
#else
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    case CTM_METHOD_MG2:
#ifdef _CTM_SUPPORT_MG2
      _ctmUncompressFrame_MG2(self);
      break;
#else
      self->mError = CTM_UNSUPPORTED_OPERATION;
      return;
#endif

    default:
      self->mError = CTM_INTERNAL_ERROR;
  }

  // We are done with the frame, on to the next...
  ++ self->mCurrentFrame;
}

//-----------------------------------------------------------------------------
// ctmSaveFile()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmSaveFile(CTMcontext aContext,
  const char * aFileName)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // Are we allowed to write the mesh (=first frame)?
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0) ||
     self->mFileStream)
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Open file stream
  self->mFileStream = fopen(aFileName, "wb");
  if(!self->mFileStream)
  {
    self->mError = CTM_FILE_ERROR;
    return;
  }

  // Continue with the custom write function...
  ctmSaveCustom(aContext, _ctmDefaultWrite, self->mFileStream);
#else
  DUMMYUSE(aFileName);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmSaveCustom()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmSaveCustom(CTMcontext aContext,
  CTMwritefn aWriteFn, void * aUserData)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
#ifdef _CTM_SUPPORT_SAVE
  CTMuint flags;
  _CTMfloatmap * map;
#endif
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // Are we allowed to write the mesh (=first frame)?
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame >= 0))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Check mesh integrity
  if(!_ctmCheckMeshIntegrity(self))
  {
    self->mError = CTM_INVALID_MESH;
    return;
  }

  // Initialize stream
  self->mWriteFn = aWriteFn;
  self->mUserData = aUserData;

  // Determine flags
  flags = 0;
  if(self->mHasNormals)
    flags |= _CTM_HAS_NORMALS_BIT;

  // Write header to stream
  _ctmStreamWrite(self, (void *) "OCTM", 4);
  _ctmStreamWriteUINT(self, _CTM_FORMAT_VERSION);
  switch(self->mMethod)
  {
    case CTM_METHOD_RAW:
      _ctmStreamWrite(self, (void *) "RAW\0", 4);
      break;

    case CTM_METHOD_MG1:
      _ctmStreamWrite(self, (void *) "MG1\0", 4);
      break;

    case CTM_METHOD_MG2:
      _ctmStreamWrite(self, (void *) "MG2\0", 4);
      break;

    default:
      self->mError = CTM_INTERNAL_ERROR;
      return;
  }
  _ctmStreamWriteUINT(self, self->mVertexCount);
  _ctmStreamWriteUINT(self, self->mTriangleCount);
  _ctmStreamWriteUINT(self, self->mUVMapCount);
  _ctmStreamWriteUINT(self, self->mAttribMapCount);
  _ctmStreamWriteUINT(self, flags);
  _ctmStreamWriteUINT(self, self->mFrameCount);
  _ctmStreamWriteSTRING(self, self->mFileComment);

  // Write UV map info
  if(self->mUVMaps)
  {
    _ctmStreamWrite(self, (void *) "UINF", 4);
    map = self->mUVMaps;
    while(map)
    {
      _ctmStreamWriteSTRING(self, map->mName);
      _ctmStreamWriteSTRING(self, map->mFileName);
      map = map->mNext;
    }
  }

  // Write attribute map info
  if(self->mAttribMaps)
  {
    _ctmStreamWrite(self, (void *) "AINF", 4);
    map = self->mAttribMaps;
    while(map)
    {
      _ctmStreamWriteSTRING(self, map->mName);
      map = map->mNext;
    }
  }

  // Compress to stream
  switch(self->mMethod)
  {
#ifdef _CTM_SUPPORT_RAW
    case CTM_METHOD_RAW:
      _ctmCompressMesh_RAW(self);
      break;
#endif

#ifdef _CTM_SUPPORT_MG1
    case CTM_METHOD_MG1:
      _ctmCompressMesh_MG1(self);
      break;
#endif

#ifdef _CTM_SUPPORT_MG2
    case CTM_METHOD_MG2:
      _ctmCompressMesh_MG2(self);
      break;
#endif

    default:
      self->mError = CTM_INTERNAL_ERROR;
      return;
  }

  // We are done with the frame, on to the next...
  self->mCurrentFrame = 1;
#else
  DUMMYUSE(aWriteFn);
  DUMMYUSE(aUserData);
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmWriteNextFrame()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmWriteNextFrame(CTMcontext aContext,
  CTMfloat aFrameTime)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

#ifdef _CTM_SUPPORT_SAVE
  // Are we allowed to save this frame?
  if((self->mMode != CTM_EXPORT) || (self->mCurrentFrame < 1) ||
     (self->mCurrentFrame >= self->mFrameCount))
  {
    self->mError = CTM_INVALID_OPERATION;
    return;
  }

  // Write frame header
  if(aFrameTime <= self->mFrameTime)
  {
    // Frame times must be incremental. Rationale: applications would be very
    // confused if they had to re-sort the animation frames.
    self->mError = CTM_INVALID_ARGUMENT;
    return;
  }
  self->mFrameTime = aFrameTime;
  _ctmStreamWriteFLOAT(self, self->mFrameTime);

  // Compress to stream
  switch(self->mMethod)
  {
#ifdef _CTM_SUPPORT_RAW
    case CTM_METHOD_RAW:
      _ctmCompressFrame_RAW(self);
      break;
#endif

#ifdef _CTM_SUPPORT_MG1
    case CTM_METHOD_MG1:
      _ctmCompressFrame_MG1(self);
      break;
#endif

#ifdef _CTM_SUPPORT_MG2
    case CTM_METHOD_MG2:
      _ctmCompressFrame_MG2(self);
      break;
#endif

    default:
      self->mError = CTM_INTERNAL_ERROR;
      return;
  }

  // We are done with the frame, on to the next...
  ++ self->mCurrentFrame;
#else
  self->mError = CTM_UNSUPPORTED_OPERATION;
#endif
}

//-----------------------------------------------------------------------------
// ctmClose()
//-----------------------------------------------------------------------------
CTMEXPORT void CTMCALL ctmClose(CTMcontext aContext)
{
  _CTMcontext * self = (_CTMcontext *) aContext;
  if(!self) return;

  // Close the file stream (if any)
  if(self->mFileStream)
  {
    fclose(self->mFileStream);
    self->mFileStream = (FILE *) 0;
  }

  // Clear load/save handles
  self->mReadFn = (CTMreadfn) 0;
  self->mWriteFn = (CTMwritefn) 0;
  self->mUserData = (void *) 0;

  // Unset the internal frame counter (ready for writing/reading new files)
  self->mCurrentFrame = -1;
}
