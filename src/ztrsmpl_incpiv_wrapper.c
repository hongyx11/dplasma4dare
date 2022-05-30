/*
 * Copyright (c) 2010-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * @precisions normal z -> s d c
 *
 */


#include "dplasma.h"
#include "dplasmaaux.h"
#include "dplasma/types.h"
#include "cores/core_blas.h"

#include "ztrsmpl_incpiv.h"

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 * dplasma_ztrsmpl_incpiv_New - Generates the taskpool that solves U*x = b, when U has
 * been generated through LU factorization with incremental pivoting strategy
 * See dplasma_zgetrf_incpiv_New().
 *
 * WARNING: The computations are not done by this call.
 *
 *******************************************************************************
 *
 * @param[in] A
 *          Descriptor of the distributed matrix A to be factorized.
 *          On entry, The factorized matrix through dplasma_zgetrf_incpiv_New()
 *          routine.  Elements on and above the diagonal are the elements of
 *          U. Elements below the diagonal are NOT the classic L, but the L
 *          factors obtaines by succesive pivoting.
 *
 * @param[in] L
 *          Descriptor of the matrix L distributed exactly as the A matrix.
 *          L.mb defines the IB parameter of the tile LU
 *          algorithm. This matrix must be of size A.mt * L.mb - by - A.nt *
 *          L.nb, with L.nb == A.nb.
 *          On entry, contains auxiliary information required to solve the
 *          system and generated by dplasma_zgetrf_inciv_New().
 *
 * @param[in] IPIV
 *          Descriptor of the IPIV matrix. Should be distributed exactly as the
 *          A matrix. This matrix must be of size A.m - by - A.nt with IPIV.mb =
 *          A.mb and IPIV.nb = 1.
 *          On entry, contains the pivot indices of the successive row
 *          interchanged performed during the factorization.
 *
 * @param[in,out] B
 *          On entry, the N-by-NRHS right hand side matrix B.
 *          On exit, if return value = 0, B is overwritten by the solution matrix X.
 *
 *******************************************************************************
 *
 * @return
 *          \retval NULL if incorrect parameters are given.
 *          \retval The parsec taskpool describing the operation that can be
 *          enqueued in the runtime with parsec_context_add_taskpool(). It, then, needs to be
 *          destroy with dplasma_ztrsmpl_incpiv_Destruct();
 *
 *******************************************************************************
 *
 * @sa dplasma_ztrsmpl_incpiv
 * @sa dplasma_ztrsmpl_incpiv_Destruct
 * @sa dplasma_ctrsmpl_incpiv_New
 * @sa dplasma_dtrsmpl_incpiv_New
 * @sa dplasma_strsmpl_incpiv_New
 *
 ******************************************************************************/
parsec_taskpool_t *
dplasma_ztrsmpl_incpiv_New(const parsec_tiled_matrix_t *A,
                    const parsec_tiled_matrix_t *L,
                    const parsec_tiled_matrix_t *IPIV,
                    parsec_tiled_matrix_t *B)
{
    parsec_ztrsmpl_incpiv_taskpool_t *parsec_trsmpl_incpiv = NULL; 

    if ( (A->mt != L->mt) || (A->nt != L->nt) ) {
        dplasma_error("dplasma_ztrsmpl_incpiv_New", "L doesn't have the same number of tiles as A");
        return NULL;
    }
    if ( (A->mt != IPIV->mt) || (A->nt != IPIV->nt) ) {
        dplasma_error("dplasma_ztrsmpl_incpiv_New", "IPIV doesn't have the same number of tiles as A");
        return NULL;
    }

    parsec_trsmpl_incpiv = parsec_ztrsmpl_incpiv_new(A,
                                       L,
                                       IPIV,
                                       B );

    /* A */
    dplasma_add2arena_tile( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_DEFAULT_ADT_IDX],
                            A->mb*A->nb*sizeof(dplasma_complex64_t),
                            PARSEC_ARENA_ALIGNMENT_SSE,
                            parsec_datatype_double_complex_t, A->mb );

    /* IPIV */
    dplasma_add2arena_rectangle( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_PIVOT_ADT_IDX],
                                 A->mb*sizeof(int),
                                 PARSEC_ARENA_ALIGNMENT_SSE,
                                 parsec_datatype_int_t, A->mb, 1, -1 );

    /* L */
    dplasma_add2arena_rectangle( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_SMALL_L_ADT_IDX],
                                 L->mb*L->nb*sizeof(dplasma_complex64_t),
                                 PARSEC_ARENA_ALIGNMENT_SSE,
                                 parsec_datatype_double_complex_t, L->mb, L->nb, -1);


    return (parsec_taskpool_t*)parsec_trsmpl_incpiv;
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 *  dplasma_ztrsmpl_incpiv_Destruct - Free the data structure associated to an taskpool
 *  created with dplasma_ztrsmpl_incpiv_New().
 *
 *******************************************************************************
 *
 * @param[in,out] taskpool
 *          On entry, the taskpool to destroy.
 *          On exit, the taskpool cannot be used anymore.
 *
 *******************************************************************************
 *
 * @sa dplasma_ztrsmpl_incpiv_New
 * @sa dplasma_ztrsmpl_incpiv
 *
 ******************************************************************************/
void
dplasma_ztrsmpl_incpiv_Destruct( parsec_taskpool_t *tp )
{
    parsec_ztrsmpl_incpiv_taskpool_t *parsec_trsmpl_incpiv = (parsec_ztrsmpl_incpiv_taskpool_t *)tp;

    dplasma_matrix_del2arena( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_DEFAULT_ADT_IDX] );
    dplasma_matrix_del2arena( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_PIVOT_ADT_IDX  ] );
    dplasma_matrix_del2arena( &parsec_trsmpl_incpiv->arenas_datatypes[PARSEC_ztrsmpl_incpiv_SMALL_L_ADT_IDX] );

    parsec_taskpool_free(tp);
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 * dplasma_ztrsmpl_incpiv - Solves U*x = b, when U has been generated through LU
 * factorization with incremental pivoting strategy
 * See dplasma_zgetrf_incpiv().
 *
 *******************************************************************************
 *
 * @param[in,out] parsec
 *          The parsec context of the application that will run the operation.
 *
 * @param[in] A
 *          Descriptor of the distributed matrix A to be factorized.
 *          On entry, The factorized matrix through dplasma_zgetrf_incpiv_New()
 *          routine.  Elements on and above the diagonal are the elements of
 *          U. Elements below the diagonal are NOT the classic L, but the L
 *          factors obtaines by succesive pivoting.
*
  * @param[in] L
 *          Descriptor of the matrix L distributed exactly as the A matrix.
 *          L.mb defines the IB parameter of the tile LU
 *          algorithm. This matrix must be of size A.mt * L.mb - by - A.nt *
 *          L.nb, with L.nb == A.nb.
 *          On entry, contains auxiliary information required to solve the
 *          system and generated by dplasma_zgetrf_inciv_New().
 *
 * @param[in] IPIV
 *          Descriptor of the IPIV matrix. Should be distributed exactly as the
 *          A matrix. This matrix must be of size A.m - by - A.nt with IPIV.mb =
 *          A.mb and IPIV.nb = 1.
 *          On entry, contains the pivot indices of the successive row
 *          interchanged performed during the factorization.
 *
 * @param[in,out] B
 *          On entry, the N-by-NRHS right hand side matrix B.
 *          On exit, if return value = 0, B is overwritten by the solution matrix X.
 *
 *******************************************************************************
 *
 * @return
 *          \retval -i if the ith parameters is incorrect.
 *          \retval 0 on success.
 *          \retval i if ith value is singular. Result is incoherent.
 *
 *******************************************************************************
 *
 * @sa dplasma_ztrsmpl_incpiv
 * @sa dplasma_ztrsmpl_incpiv_Destruct
 * @sa dplasma_ctrsmpl_incpiv_New
 * @sa dplasma_dtrsmpl_incpiv_New
 * @sa dplasma_strsmpl_incpiv_New
 *
 ******************************************************************************/
int
dplasma_ztrsmpl_incpiv( parsec_context_t *parsec,
                 const parsec_tiled_matrix_t *A,
                 const parsec_tiled_matrix_t *L,
                 const parsec_tiled_matrix_t *IPIV,
                       parsec_tiled_matrix_t *B )
{
    parsec_taskpool_t *parsec_ztrsmpl_incpiv = NULL;

    if ( (A->mt != L->mt) || (A->nt != L->nt) ) {
        dplasma_error("dplasma_ztrsmpl_incpiv", "L doesn't have the same number of tiles as A");
        return -3;
    }
    if ( (A->mt != IPIV->mt) || (A->nt != IPIV->nt) ) {
        dplasma_error("dplasma_ztrsmpl_incpiv", "IPIV doesn't have the same number of tiles as A");
        return -4;
    }

    parsec_ztrsmpl_incpiv = dplasma_ztrsmpl_incpiv_New(A, L, IPIV, B);
    if ( parsec_ztrsmpl_incpiv != NULL )
    {
        parsec_context_add_taskpool( parsec, parsec_ztrsmpl_incpiv );
        dplasma_wait_until_completion( parsec );
        dplasma_ztrsmpl_incpiv_Destruct( parsec_ztrsmpl_incpiv );
        return 0;
    }
    else
        return -101;
}
