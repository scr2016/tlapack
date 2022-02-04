/// @file example_mdspan.cpp Shows how to use mdspan to work with <T>LAPACK
/// @author Weslley S Pereira, University of Colorado Denver, USA
//
// Copyright (c) 2021, University of Colorado Denver. All rights reserved.
//
// This file is part of <T>LAPACK.
// <T>LAPACK is free software: you can redistribute it and/or modify it under
// the terms of the BSD 3-Clause license. See the accompanying LICENSE file.

// #include <tlapack.hpp>

#include <vector>
#include <iostream>

#include <plugins/tlapack_mdspan.hpp>
#include <slate_api/blas/mdspan.hpp>
#include <tlapack.hpp>

#include "tiledLayout.h"

//------------------------------------------------------------------------------
int main( int argc, char** argv )
{
    using T = float;

    using namespace blas;
    using blas::internal::colmajor_matrix;

    using std::experimental::layout_stride;
    using std::experimental::dextents;
    
    using idx_t = std::size_t;
    using pair = std::pair<idx_t,idx_t>;

    using TiledMapping  = typename TiledLayout::template mapping<dextents<2>>;
    using strideMapping = typename layout_stride::template mapping<dextents<2>>;

    
    // constants
    const idx_t n = 100, k = 40, row_tile = 2, col_tile = 5;
    const idx_t lda = 110, ldc = 120;
    T one( 1.0 );
    
    // raw data arrays
    T A_[ lda*n ] = {};
    T C_[ n*ldc ] = {};

    /// Using dynamic extents: 

    // Column Major Matrix A
    auto A  = colmajor_matrix( A_, n, n, lda );
    // Column Major Matrix Ak with the first k columns of A
    auto Ak = submatrix( A, pair(0,n), pair(0,k) );
    // Tiled Matrix B with the last k*n elements of A_
    auto B  = mdspan< T, dextents<2>, TiledLayout >(
        &A(n*(lda-k),0), TiledMapping( dextents<2>(k, n), row_tile, col_tile )
    );
    // Row Major Matrix C
    auto C  = mdspan< T, dextents<2>, layout_stride >(
        C_, strideMapping( dextents<2>(n, n), std::array<idx_t,2>{ldc,1} )
    );

    // Init random seed
    srand( 4539 );

    // Initialize A_ and C_ with junk
    for (idx_t j = 0; j < n; ++j) {
        for (idx_t i = 0; i < lda; ++i)
            A_[i+j*lda] = T( static_cast<float>( 0xDEADBEEF ) );
        for (idx_t i = 0; i < ldc; ++i)
            C_[i+j*ldc] = T( static_cast<float>( 0xDEFECA7E ) );
    }
    
    // Generate a random matrix in Ak
    for (idx_t j = 0; j < k; ++j)
        for (idx_t i = 0; i < n; ++i)
            Ak(i,j) = static_cast<float>( rand() )
                    / static_cast<float>( RAND_MAX );
                        
    // Initialize B with zeros
    for (idx_t j = 0; j < n; ++j)
        for (idx_t i = 0; i < k; ++i)
            B(i,j) = 0.0;
    // then put ones in the main diagonal of B
    for (idx_t i = 0; i < k; ++i)
        B(i,i) = 1.0;

    // Initialize the last n-k columns of C with zeros
    for (idx_t j = k; j < n; ++j)
        for (idx_t i = 0; i < n; ++i)
            C(i,j) = 0.0;
    // Set the first k columns of C with Ak
    for (idx_t j = 0; j < k; ++j)
        for (idx_t i = 0; i < n; ++i)
            C(i,j) = Ak(i,j);

    // gemm:

    // C = Ak*B + C
    gemm( Op::NoTrans, Op::NoTrans, T(-1.0), Ak, B, T(1.0), C );

    std::cout.precision(5);
    std::cout << std::scientific << std::showpos;
    std::cout << "|| C - Ak B ||_F = " 
              << lapack::lange( lapack::frob_norm, C )
              << std::endl;

    // potrf2:

    // Initialize A_ with junk one more time
    for (idx_t j = 0; j < n; ++j)
        for (idx_t i = 0; i < lda; ++i)
            A_[i+j*lda] = T( static_cast<float>( 0xDEADBEEF ) );

    // Column Major Matrices U and Asym as submatrices of A
    auto U    = submatrix( A, pair(0,k), pair(0,k) );
    auto Asym = submatrix( A, pair(0,k), pair(k,2*k) );
    
    // Fill Asym with random entries
    for (idx_t j = 0; j < k; ++j) {
        for (idx_t i = 0; i < k; ++i)
            Asym(i,j) = static_cast<float>( rand() )
                      / static_cast<float>( RAND_MAX );
    }
    
    // Turns Asym into a symmetric positive definite matrix
    for (idx_t j = 0; j < k; ++j) {
        for (idx_t i = 0; i < j; ++i) {
            Asym(i,j) = T(0.5) * ( Asym(i,j) + Asym(j,i) );
            Asym(j,i) = Asym(i,j);
        }
        Asym(j,j) += k * T( 1.0 );
    }
    
    // Copy the upper part of Asym to U
    for (idx_t j = 0; j < k; ++j) {
        for (idx_t i = 0; i <= j; ++i)
            U(i,j) = Asym(i,j);
    }

    // Compute the Cholesky decomposition of U
    int info = lapack::potrf2( lapack::upper_triangle, U );

    std::cout << "Cholesky ended with info " << info
              << std::endl;

    // Solve U^H U R = A
    auto R  = submatrix( A, pair(k,2*k), pair(0,k) );
    for (idx_t j = 0; j < k; ++j)
        for (idx_t i = 0; i < k; ++i)
            R(i,j) = Asym(i,j);
    trsm(
        Side::Left, Uplo::Upper,
        Op::ConjTrans, Diag::NonUnit,
        one, U, R );
    trsm(
        Side::Left, Uplo::Upper,
        Op::NoTrans, Diag::NonUnit,
        one, U, R );

    // error = ||R-Id||_F
    for (idx_t i = 0; i < k; ++i)
        R(i,i) -= one;
    T error = lapack::lange( lapack::frob_norm, R );

    std::cout.precision(5);
    std::cout << std::scientific << std::showpos;
    std::cout << "U^H U R = A   =>   ||R-Id||_F / ||Id||_F = " << error / sqrt(k)
              << std::endl;

    return 0;
}
