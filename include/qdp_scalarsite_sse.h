// -*- C++ -*-
// $Id: qdp_scalarsite_sse.h,v 1.21 2005-03-17 18:52:32 bjoo Exp $

/*! @file
 * @brief Intel SSE optimizations
 *
 * SSE optimizations of basic operations
 */

#ifndef QDP_SCALARSITE_SSE_H
#define QDP_SCALARSITE_SSE_H


#if BASE_PRECISION == 32
// DO THE SSE STUFF

// These SSE asm instructions are only supported under GCC/G++
#if defined(__GNUC__)

// Use SSE specific Linalg stuff (inline assembler etc)
#include "scalarsite_sse/qdp_scalarsite_sse_linalg.h"

#if __GNUC_MINOR__ >= 2
// Use SSE specific blas stuff (inline assembler etc)
// Only supported on gcc >= 3.2
#include "scalarsite_sse/qdp_scalarsite_sse_blas.h"
#else
#warning "This version of gcc does not support vector types - not using SSE blas code"
#endif

// Use Complex BLAS from Generics. It is better than nothing
#include "scalarsite_generic/qdp_scalarsite_generic_cblas.h"

// Use chiralProject BLAS from Generics. There seems little difference
// between it and the SSE... Memory Bandwidth limits?

#else

#error "This is not a GNUC compiler, and therefore does not support the GNU specific asm directives."

#endif  // gnuc

#else // BASE PRECISION 

// Double precision. We don't actually have any double precision BLAS
// coded in SSE so use the Generics instead. THey are better than nowt.
#include "scalarsite_generic/qdp_scalarsite_generic_linalg.h"
#include "scalarsite_generic/qdp_scalarsite_generic_blas.h"
#include "scalarsite_generic/qdp_scalarsite_generic_cblas.h"

#endif  // BASE PRECISION

// This dude takes care of the GNUC and PRECISION and stuff internally
#include "scalarsite_sse/qdp_scalarsite_sse_blas_g5.h"

#endif  // guard

