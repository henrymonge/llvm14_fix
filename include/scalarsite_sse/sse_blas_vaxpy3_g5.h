// $Id: sse_blas_vaxpy3_g5.h,v 1.2 2005-12-21 16:04:25 bjoo Exp $

/*! @file
 *  @brief Generic Scalar VAXPY routine
 *
 */

#ifndef QDP_SSE_BLAS_VAXPY3_G5
#define QDP_SSE_BLAS_VAXPY3_G5

#if defined(__GNUC__)

#include "qdp_config.h"
#include "qdp_sse_intrin.h"

QDP_BEGIN_NAMESPACE(QDP);

#if BASE_PRECISION==32



// (Vector) out = (Scalar) (*scalep) * (Vector) InScale + (Vector) P{+} Add
inline
void axpyz_g5ProjPlus(REAL32 *Out,REAL32 *scalep,REAL32 *InScale, REAL32 *Add,int n_4vec)
{
  // GNUC vector type
  

  // Load Vscalep
  v4sf vscalep = _mm_load_ss(scalep);
  asm("shufps\t$0,%0,%0" : "+x" (vscalep));

  for(int i=0; i < n_4vec; i++) {

    // Spin Component 0: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+ 0, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 0)), _mm_load_ps(Add+ 0)));
     
    // Spin Component 0: z2r, z2i, SpinComponent 1: z0r, z0i
    _mm_store_ps(Out+ 4, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 4)), _mm_load_ps(Add+ 4)));

    // Spin Component 1: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+ 8, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 8)), _mm_load_ps(Add+ 8)));

    // Spin Component 2: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+12, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 12)));

    // Spin Component 2: z2r, z2i, z0r, z0r
    _mm_store_ps(Out+16, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 16)));

    // Spin Component 3: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+20, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 20)));

    // Update offsets
    Out += 24; InScale += 24; Add += 24;
  }

}

// (Vector) out = (Scalar) (*scalep) * (Vector) InScale + (Vector) P{+} Add
inline
void axpyz_g5ProjMinus(REAL32 *Out,REAL32 *scalep,REAL32 *InScale, REAL32 *Add,int n_4vec)
{
  // GNUC vector type
  

  // Load Vscalep
  v4sf vscalep = _mm_load_ss(scalep);
  asm("shufps\t$0,%0,%0" : "+x" (vscalep));

  for(int i=0; i < n_4vec; i++) {
    // Spin Component 0: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+0, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 0)));

    // Spin Component 0: z2r, z2i, Spin Component 1: z0r, z0r
    _mm_store_ps(Out+4, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 4)));

    // Spin Component 1: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+8, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 8)));


    // Spin Component 2: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+12, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 12)), _mm_load_ps(Add+ 12)));
     
    // Spin Component 2: z2r, z2i, SpinComponent 3: z0r, z0i
    _mm_store_ps(Out+ 16, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 16)), _mm_load_ps(Add+ 16)));

    // Spin Component 3: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+ 20, _mm_add_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 20)), _mm_load_ps(Add+ 20)));
    // Update offsets
    Out += 24; InScale += 24; Add += 24;
  }

}



// AXMY  versions
// (Vector) out = (Scalar) (*scalep) * (Vector) InScale - (Vector) P{+} Add
inline
void axmyz_g5ProjPlus(REAL32 *Out,REAL32 *scalep,REAL32 *InScale, REAL32 *Add,int n_4vec)
{
  // GNUC vector type
  

  // Load Vscalep

  v4sf vscalep = _mm_load_ss(scalep);
  asm("shufps\t$0,%0,%0" : "+x" (vscalep));  

  for(int i=0; i < n_4vec; i++) {
    
    // Spin Component 0: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+ 0, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 0)), _mm_load_ps(Add+ 0)));
    
     
    // Spin Component 0: z2r, z2i, SpinComponent 1: z0r, z0i
    _mm_store_ps(Out+ 4, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 4)), _mm_load_ps(Add+ 4)));
    
     // Spin Component 1: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+ 8, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 8)), _mm_load_ps(Add+ 8)));
  
    // Spin Component 2: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+12, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 12)));
    
    
    
    // Spin Component 2: z2r, z2i, z0r, z0r
    _mm_store_ps(Out+16, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 16)));

    // Spin Component 3: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+20, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 20)));

    // Update offsets
    Out += 24; InScale += 24; Add += 24;
  }


}

// (Vector) out = (Scalar) (*scalep) * (Vector) InScale - (Vector) P{-} Add
inline
void axmyz_g5ProjMinus(REAL32 *Out,REAL32 *scalep,REAL32 *InScale, REAL32 *Add,int n_4vec)
{
  // GNUC vector type
  

  // Load Vscalep
  v4sf vscalep = _mm_load_ss(scalep);
  asm("shufps\t$0,%0,%0" : "+x" (vscalep));

  for(int i=0; i < n_4vec; i++) {
    // Spin Component 0: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+0, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 0)));

    // Spin Component 0: z2r, z2i, Spin Component 1: z0r, z0r
    _mm_store_ps(Out+4, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 4)));

    // Spin Component 1: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+8, _mm_mul_ps(vscalep, _mm_load_ps(InScale+ 8)));


    // Spin Component 2: z0r, z0i, z1r, z1i
    _mm_store_ps(Out+12, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 12)), _mm_load_ps(Add+ 12)));
     
    // Spin Component 2: z2r, z2i, SpinComponent 3: z0r, z0i
    _mm_store_ps(Out+ 16, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 16)), _mm_load_ps(Add+ 16)));

    // Spin Component 3: z1r, z1i, z2r, z2i
    _mm_store_ps(Out+ 20, _mm_sub_ps(_mm_mul_ps(vscalep, _mm_load_ps(InScale+ 20)), _mm_load_ps(Add+ 20)));
    // Update offsets
    Out += 24; InScale += 24; Add += 24;
  }

}


#endif

QDP_END_NAMESPACE(QDP);


#endif // GNUC

#endif // guard
