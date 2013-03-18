//-----------------------------------------------------------------------------
// Product:     OpenCTM
// File:        array.c
// Description: Typed array access functions.
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

#include "openctm2.h"
#include "internal.h"


//-----------------------------------------------------------------------------
// Generic getter functions.
//-----------------------------------------------------------------------------

static CTMuint _ctmGetArrayi(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    switch(aArray->mType)
    {
      case CTM_BYTE:
        return (CTMuint) ((CTMbyte *)elementPtr)[aComponent];
      case CTM_UBYTE:
        return (CTMuint) ((CTMubyte *)elementPtr)[aComponent];
      case CTM_SHORT:
        return (CTMuint) ((CTMshort *)elementPtr)[aComponent];
      case CTM_USHORT:
        return (CTMuint) ((CTMushort *)elementPtr)[aComponent];
      case CTM_INT:
        return (CTMuint) ((CTMint *)elementPtr)[aComponent];
      case CTM_UINT:
        return ((CTMuint *)elementPtr)[aComponent];
      case CTM_FLOAT:
        return (CTMuint) ((CTMfloat *)elementPtr)[aComponent];
      case CTM_DOUBLE:
        return (CTMuint) ((CTMdouble *)elementPtr)[aComponent];
      default:
        break;
    }
  }
  return 0;
}

static CTMfloat _ctmGetArrayf(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    switch(aArray->mType)
    {
      case CTM_BYTE:
        return (1.0f/127.0f) * (CTMfloat) ((CTMbyte *)elementPtr)[aComponent];
      case CTM_UBYTE:
        return (1.0f/255.0f) * (CTMfloat) ((CTMubyte *)elementPtr)[aComponent];
      case CTM_SHORT:
        return (CTMfloat) ((CTMshort *)elementPtr)[aComponent];
      case CTM_USHORT:
        return (CTMfloat) ((CTMushort *)elementPtr)[aComponent];
      case CTM_INT:
        return (CTMfloat) ((CTMint *)elementPtr)[aComponent];
      case CTM_UINT:
        return (CTMfloat) ((CTMuint *)elementPtr)[aComponent];
      case CTM_FLOAT:
        return ((CTMfloat *)elementPtr)[aComponent];
      case CTM_DOUBLE:
        return (CTMfloat) ((CTMdouble *)elementPtr)[aComponent];
      default:
        break;
    }
  }
  return 0.0f;
}

//-----------------------------------------------------------------------------
// Generic setter functions.
//-----------------------------------------------------------------------------

static void _ctmSetArrayi(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMuint aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    switch(aArray->mType)
    {
      case CTM_BYTE:
        ((CTMbyte *)elementPtr)[aComponent] = (CTMbyte) aValue;
        break;
      case CTM_UBYTE:
        ((CTMubyte *)elementPtr)[aComponent] = (CTMubyte) aValue;
        break;
      case CTM_SHORT:
        ((CTMshort *)elementPtr)[aComponent] = (CTMshort) aValue;
        break;
      case CTM_USHORT:
        ((CTMushort *)elementPtr)[aComponent] = (CTMushort) aValue;
        break;
      case CTM_INT:
        ((CTMint *)elementPtr)[aComponent] = (CTMint) aValue;
        break;
      case CTM_UINT:
        ((CTMuint *)elementPtr)[aComponent] = aValue;
        break;
      case CTM_FLOAT:
        ((CTMfloat *)elementPtr)[aComponent] = (CTMfloat) aValue;
        break;
      case CTM_DOUBLE:
        ((CTMdouble *)elementPtr)[aComponent] = (CTMdouble) aValue;
        break;
      default:
        break;
    }
  }
}

static void _ctmSetArrayf(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    switch(aArray->mType)
    {
      case CTM_BYTE:
        ((CTMbyte *)elementPtr)[aComponent] = (CTMbyte) (127.0f * aValue);
        break;
      case CTM_UBYTE:
        ((CTMubyte *)elementPtr)[aComponent] = (CTMubyte) (255.0f * aValue);
        break;
      case CTM_SHORT:
        ((CTMshort *)elementPtr)[aComponent] = (CTMshort) aValue;
        break;
      case CTM_USHORT:
        ((CTMushort *)elementPtr)[aComponent] = (CTMushort) aValue;
        break;
      case CTM_INT:
        ((CTMint *)elementPtr)[aComponent] = (CTMint) aValue;
        break;
      case CTM_UINT:
        ((CTMuint *)elementPtr)[aComponent] = (CTMuint) aValue;
        break;
      case CTM_FLOAT:
        ((CTMfloat *)elementPtr)[aComponent] = aValue;
        break;
      case CTM_DOUBLE:
        ((CTMdouble *)elementPtr)[aComponent] = (CTMdouble) aValue;
        break;
      default:
        break;
    }
  }
}

//-----------------------------------------------------------------------------
// Specialized getter functions (boilerplate alert!).
//-----------------------------------------------------------------------------

static CTMuint _ctmGetArrayi_UINT(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    return ((CTMuint *)elementPtr)[aComponent];
  }
  return 0;
}

static CTMuint _ctmGetArrayi_UINT_3(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    return ((CTMuint *)aArray->mData)[aElement * 3 + aComponent];
  }
  return 0;
}

static CTMfloat _ctmGetArrayf_FLOAT(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    return ((CTMfloat *)elementPtr)[aComponent];
  }
  return 0.0f;
}

static CTMfloat _ctmGetArrayf_FLOAT_2(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    return ((CTMfloat *)aArray->mData)[aElement * 2 + aComponent];
  }
  return 0.0f;
}

static CTMfloat _ctmGetArrayf_FLOAT_3(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    return ((CTMfloat *)aArray->mData)[aElement * 3 + aComponent];
  }
  return 0.0f;
}

static CTMfloat _ctmGetArrayf_FLOAT_8(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    return ((CTMfloat *)aArray->mData)[aElement * 8 + aComponent];
  }
  return 0.0f;
}

static CTMfloat _ctmGetArrayf_DOUBLE(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    return (CTMfloat) ((CTMdouble *)elementPtr)[aComponent];
  }
  return 0.0f;
}

//-----------------------------------------------------------------------------
// Specialized setter functions (boilerplate alert!).
//-----------------------------------------------------------------------------

static void _ctmSetArrayi_UINT(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMuint aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    ((CTMuint *)elementPtr)[aComponent] = aValue;
  }
}

static void _ctmSetArrayi_UINT_3(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMuint aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    ((CTMuint *)aArray->mData)[aElement * 3 + aComponent] = aValue;
  }
}

static void _ctmSetArrayf_FLOAT(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    ((CTMfloat *)elementPtr)[aComponent] = aValue;
  }
}

static void _ctmSetArrayf_FLOAT_2(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    ((CTMfloat *)aArray->mData)[aElement * 2 + aComponent] = aValue;
  }
}

static void _ctmSetArrayf_FLOAT_3(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    ((CTMfloat *)aArray->mData)[aElement * 3 + aComponent] = aValue;
  }
}

static void _ctmSetArrayf_FLOAT_8(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    ((CTMfloat *)aArray->mData)[aElement * 8 + aComponent] = aValue;
  }
}

static void _ctmSetArrayf_DOUBLE(_CTMarray * aArray, CTMuint aElement,
  CTMuint aComponent, CTMfloat aValue)
{
  if(LIKELY((aComponent < aArray->mSize) && aArray->mData))
  {
    void * elementPtr = (void *) &((CTMbyte *)aArray->mData)[aElement * aArray->mStride];
    ((CTMdouble *)elementPtr)[aComponent] = (CTMdouble) aValue;
  }
}


//-----------------------------------------------------------------------------
// _ctmClearArray() - Clear a typed array (set default values).
//-----------------------------------------------------------------------------
void _ctmClearArray(_CTMarray * aArray)
{
  aArray->mData = (void *) 0;
  aArray->mType = CTM_FLOAT;
  aArray->mSize = 0;
  aArray->mStride = 0;
  aArray->geti = _ctmGetArrayi;
  aArray->getf = _ctmGetArrayf;
  aArray->seti = _ctmSetArrayi;
  aArray->setf = _ctmSetArrayf;
}

//-----------------------------------------------------------------------------
// _ctmInitArray() - Initialize a typed array.
//-----------------------------------------------------------------------------
CTMenum _ctmInitArray(_CTMarray * aArray, CTMuint aSize, CTMenum aType,
  CTMuint aStride, void * aData)
{
  CTMuint typeSize;

  // Check type and get type size
  switch(aType)
  {
    case CTM_BYTE:
    case CTM_UBYTE:
      typeSize = sizeof(CTMbyte);
      break;
    case CTM_SHORT:
    case CTM_USHORT:
      typeSize = sizeof(CTMshort);
      break;
    case CTM_INT:
    case CTM_UINT:
      typeSize = sizeof(CTMint);
      break;
    case CTM_FLOAT:
      typeSize = sizeof(CTMfloat);
      break;
    case CTM_DOUBLE:
      typeSize = sizeof(CTMdouble);
      break;
    default:
      return CTM_INVALID_ARGUMENT;
  }

  // If stride is zero, the user wants a fully packed array
  if (!aStride) {
    aStride = aSize * typeSize;
  }

  // Empty array? (it's generally valid for an array to be empty)
  if(!aData || !aSize) {
    _ctmClearArray(aArray);
    return CTM_NONE;
  }

  // Define array
  aArray->mData = aData;
  aArray->mType = aType;
  aArray->mSize = aSize;
  aArray->mStride = aStride;

  // Set up getter & setter functions
  aArray->geti = _ctmGetArrayi;
  aArray->getf = _ctmGetArrayf;
  aArray->seti = _ctmSetArrayi;
  aArray->setf = _ctmSetArrayf;
  switch(aType)
  {
    case CTM_INT:
    case CTM_UINT:
      if (aStride == 3 * sizeof(CTMint))
      {
        // Typical packed 3-index / triangle array
        aArray->geti = _ctmGetArrayi_UINT_3;
        aArray->seti = _ctmSetArrayi_UINT_3;
        break;
      }
      aArray->geti = _ctmGetArrayi_UINT;
      aArray->seti = _ctmSetArrayi_UINT;
      break;
    case CTM_FLOAT:
      if (aStride == 2 * sizeof(CTMfloat))
      {
        // Typical packed 2-element array (x,y,z)
        aArray->getf = _ctmGetArrayf_FLOAT_2;
        aArray->setf = _ctmSetArrayf_FLOAT_2;
        break;
      }
      if (aStride == 3 * sizeof(CTMfloat))
      {
        // Typical packed 3-element array (x,y,z)
        aArray->getf = _ctmGetArrayf_FLOAT_3;
        aArray->setf = _ctmSetArrayf_FLOAT_3;
        break;
      }
      if (aStride == 8 * sizeof(CTMfloat))
      {
        // Typical interleaved array:
        // position (3xfloat) + normal (3xfloat) + uv (2xfloat)
        aArray->getf = _ctmGetArrayf_FLOAT_8;
        aArray->setf = _ctmSetArrayf_FLOAT_8;
        break;
      }
      aArray->getf = _ctmGetArrayf_FLOAT;
      aArray->setf = _ctmSetArrayf_FLOAT;
      break;
    case CTM_DOUBLE:
      aArray->getf = _ctmGetArrayf_DOUBLE;
      aArray->setf = _ctmSetArrayf_DOUBLE;
      break;
    default:
      break;
  }

  return CTM_NONE;
}
