/// @file ung2r.hpp
/// @author Weslley S Pereira, University of Colorado Denver, USA
/// Adapted from @see https://github.com/langou/latl/blob/master/include/ung2r.h
//
// Copyright (c) 2013-2022, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

#ifndef __TLAPACK_LEGACY_UNG2R_HH__
#define __TLAPACK_LEGACY_UNG2R_HH__

#include "lapack/ung2r.hpp"

#include "tblas.hpp"

namespace tlapack {

/** Generates a m-by-n matrix Q with orthogonal columns.
 * \[
 *     Q  =  H_1 H_2 ... H_k
 * \]
 * 
 * @return  0 if success
 * @return -i if the ith argument is invalid
 * 
 * @param[in] m The number of rows of the matrix A. m>=0
 * @param[in] n The number of columns of the matrix A. n>=0
 * @param[in] k The number of elementary reflectors whose product defines the matrix Q. n>=k>=0
 * @param[in,out] A m-by-n matrix.
 *      On entry, the i-th column must contains the vector which defines the
 *      elementary reflector $H_i$, for $i=0,1,...,k-1$, as returned by GEQRF in the
 *      first k columns of its array argument A.
 *      On exit, the m-by-n matrix $Q  =  H_1 H_2 ... H_k$.
 * @param[in] lda The leading dimension of A. lda >= max(1,m).
 * @param[in] tau Real vector of length min(m,n).      
 *      The scalar factors of the elementary reflectors.
 * 
 * @ingroup geqrf
 */
template< typename TA, typename Ttau >
inline int ung2r(
    idx_t m, idx_t n, idx_t k,
    TA* A, idx_t lda,
    const Ttau* tau )
{
    using internal::colmajor_matrix;
    using internal::vector;

    // check arguments
    tlapack_check_false( m < 0, -1 );
    tlapack_check_false( n < 0 || n > m, -2 );
    tlapack_check_false( k < 0 || k > n, -3 );
    tlapack_check_false( lda < m, -5 );

    // quick return
    if (n <= 0) return 0;

    // Local parameters
    int info = 0;
    TA* work = new TA[ (n > 0) ? n-1 : 0 ];

    // Matrix views
    auto A_    = colmajor_matrix<TA>( A, m, n, lda );
    auto _tau  = vector( (Ttau*)tau, std::min<idx_t>( m, n ) );
    auto _work = vector( work, n-1 );
    
    info = ung2r( k, A_, _tau, _work );

    delete[] work;
    return info;
}

}

#endif // __TLAPACK_LEGACY_UNG2R_HH__
