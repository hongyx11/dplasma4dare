/*
 * Copyright (c) 2011-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2013      Inria. All rights reserved.
 *
 * @precisions normal z -> s d c
 *
 */

#include "dplasma.h"
#include "dplasma/types.h"
#include "dplasmaaux.h"
#include "parsec/data_dist/matrix/two_dim_rectangle_cyclic.h"

#include "zlange_frb_cyclic.h"
#include "zlange_one_cyclic.h"

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 *  dplasma_zlantr_New - Generates the taskpool that computes the value
 *
 *     zlantr = ( max(abs(A(i,j))), NORM = dplasmaMaxNorm
 *              (
 *              ( norm1(A),         NORM = dplasmaOneNorm
 *              (
 *              ( normI(A),         NORM = dplasmaInfNorm
 *              (
 *              ( normF(A),         NORM = dplasmaFrobeniusNorm
 *
 *  where norm1 denotes the one norm of a matrix (maximum column sum),
 *  normI denotes the infinity norm of a matrix (maximum row sum) and
 *  normF denotes the Frobenius norm of a matrix (square root of sum
 *  of squares). Note that max(abs(A(i,j))) is not a consistent matrix
 *  norm.
 *
 *  WARNING: The computations are not done by this call
 *
 *******************************************************************************
 *
 * @param[in] norm
 *          = dplasmaMaxNorm: Max norm
 *          = dplasmaOneNorm: One norm
 *          = dplasmaInfNorm: Infinity norm
 *          = dplasmaFrobeniusNorm: Frobenius norm
 *
 * @param[in] uplo
 *          = dplasmaUpper: Upper triangle of A is stored;
 *          = dplasmaLower: Lower triangle of A is stored.
 *
 * @param[in] diag
 *          = dplasmaNonUnit: Non-unit diagonal
 *          = dplasmaUnit: Unit diagonal
 *
 * @param[in] A
 *          The descriptor of the matrix A.
 *          Must be a two_dim_rectangle_cyclic matrix.
 *
 * @param[out] result
 *          The norm described above. Might not be set when the function returns.
 *
 *******************************************************************************
 *
 * @return
 *          \retval NULL if incorrect parameters are given.
 *          \retval The parsec taskpool describing the operation that can be
 *          enqueued in the runtime with parsec_context_add_taskpool(). It, then, needs to be
 *          destroy with dplasma_zlantr_Destruct();
 *
 *******************************************************************************
 *
 * @sa dplasma_zlantr
 * @sa dplasma_zlantr_Destruct
 * @sa dplasma_clantr_New
 * @sa dplasma_dlantr_New
 * @sa dplasma_slantr_New
 *
 ******************************************************************************/
parsec_taskpool_t*
dplasma_zlantr_New( dplasma_enum_t norm, dplasma_enum_t uplo, dplasma_enum_t diag,
                    const parsec_tiled_matrix_t *A,
                    double *result )
{
    int P, Q, IP, JQ, m, n, mb, nb, elt;
    parsec_matrix_block_cyclic_t *Tdist;
    parsec_taskpool_t *parsec_zlantr = NULL;

    if ( (norm != dplasmaMaxNorm) && (norm != dplasmaOneNorm)
        && (norm != dplasmaInfNorm) && (norm != dplasmaFrobeniusNorm) ) {
        dplasma_error("dplasma_zlantr", "illegal value of norm");
        return NULL;
    }
    if ( !(A->dtype & parsec_matrix_block_cyclic_type) ) {
        dplasma_error("dplasma_zlantr", "illegal type of descriptor for A");
        return NULL;
    }

    P = ((parsec_matrix_block_cyclic_t*)A)->grid.rows;
    Q = ((parsec_matrix_block_cyclic_t*)A)->grid.cols;
    IP = ((parsec_matrix_block_cyclic_t*)A)->grid.ip;
    JQ = ((parsec_matrix_block_cyclic_t*)A)->grid.jq;

    /* Warning: Pb with smb/snb when mt/nt lower than P/Q */
    switch( norm ) {
    case dplasmaFrobeniusNorm:
        mb = 2;
        nb = 1;
        m  = dplasma_imax(A->mt, P);
        n  = Q;
        elt = 2;
        break;
    case dplasmaInfNorm:
        mb = A->mb;
        nb = 1;
        m  = dplasma_imax(A->mt, P);
        n  = Q;
        elt = 1;
        break;
    case dplasmaOneNorm:
        mb = 1;
        nb = A->nb;
        m  = P;
        n  = dplasma_imax(A->nt, Q);
        elt = 1;
        break;
    case dplasmaMaxNorm:
    default:
        mb = 1;
        nb = 1;
        m  = dplasma_imax(A->mt, P);
        n  = Q;
        elt = 1;
    }

    /* Create a copy of the A matrix to be used as a data distribution metric.
     * As it is used as a NULL value we must have a data_copy and a data associated
     * with it, so we can create them here.
     * Create the task distribution */
    Tdist = (parsec_matrix_block_cyclic_t*)malloc(sizeof(parsec_matrix_block_cyclic_t));

    parsec_matrix_block_cyclic_init(
        Tdist, PARSEC_MATRIX_DOUBLE, PARSEC_MATRIX_TILE,
        A->super.myrank,
        1, 1, /* Dimensions of the tiles              */
        m, n, /* Dimensions of the matrix             */
        0, 0, /* Starting points (not important here) */
        m, n, /* Dimensions of the submatrix          */
        P, Q, 1, 1, IP, JQ);
    Tdist->super.super.data_of = NULL;
    Tdist->super.super.data_of_key = NULL;

    /* Create the DAG */
    switch( norm ) {
    case dplasmaOneNorm:
        parsec_zlantr = (parsec_taskpool_t*)parsec_zlange_one_cyclic_new(
            P, Q, norm, uplo, diag, A, (parsec_data_collection_t*)Tdist, result);
        break;

    case dplasmaMaxNorm:
    case dplasmaInfNorm:
    case dplasmaFrobeniusNorm:
    default:
        parsec_zlantr = (parsec_taskpool_t*)parsec_zlange_frb_cyclic_new(
            P, Q, norm, uplo, diag, A, (parsec_data_collection_t*)Tdist, result);
    }

    /* Set the datatypes */
    dplasma_add2arena_tile( &((parsec_zlange_frb_cyclic_taskpool_t*)parsec_zlantr)->arenas_datatypes[PARSEC_zlange_frb_cyclic_DEFAULT_ADT_IDX],
                            A->mb*A->nb*sizeof(dplasma_complex64_t),
                            PARSEC_ARENA_ALIGNMENT_SSE,
                            parsec_datatype_double_complex_t, A->mb );
    dplasma_add2arena_rectangle( &((parsec_zlange_frb_cyclic_taskpool_t*)parsec_zlantr)->arenas_datatypes[PARSEC_zlange_frb_cyclic_COL_ADT_IDX],
                                 mb * nb * sizeof(double), PARSEC_ARENA_ALIGNMENT_SSE,
                                 parsec_datatype_double_t, mb, nb, -1 );
    dplasma_add2arena_rectangle( &((parsec_zlange_frb_cyclic_taskpool_t*)parsec_zlantr)->arenas_datatypes[PARSEC_zlange_frb_cyclic_ELT_ADT_IDX],
                                 elt * sizeof(double), PARSEC_ARENA_ALIGNMENT_SSE,
                                 parsec_datatype_double_t, elt, 1, -1 );


    return (parsec_taskpool_t*)parsec_zlantr;
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 *  dplasma_zlantr_Destruct - Free the data structure associated to an taskpool
 *  created with dplasma_zlantr_New().
 *
 *******************************************************************************
 *
 * @param[in,out] taskpool
 *          On entry, the taskpool to destroy.
 *          On exit, the taskpool cannot be used anymore.
 *
 *******************************************************************************
 *
 * @sa dplasma_zlantr_New
 * @sa dplasma_zlantr
 *
 ******************************************************************************/
void
dplasma_zlantr_Destruct( parsec_taskpool_t *tp )
{
    parsec_zlange_frb_cyclic_taskpool_t *parsec_zlantr = (parsec_zlange_frb_cyclic_taskpool_t *)tp;

    parsec_tiled_matrix_destroy( (parsec_tiled_matrix_t*)(parsec_zlantr->_g_Tdist) );
    free( parsec_zlantr->_g_Tdist );

    dplasma_matrix_del2arena( &parsec_zlantr->arenas_datatypes[PARSEC_zlange_frb_cyclic_DEFAULT_ADT_IDX] );
    dplasma_matrix_del2arena( &parsec_zlantr->arenas_datatypes[PARSEC_zlange_frb_cyclic_COL_ADT_IDX] );
    dplasma_matrix_del2arena( &parsec_zlantr->arenas_datatypes[PARSEC_zlange_frb_cyclic_ELT_ADT_IDX] );

    parsec_taskpool_free(tp);
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 *  dplasma_zlantr - Computes the value
 *
 *     zlantr = ( max(abs(A(i,j))), NORM = dplasmaMaxNorm
 *              (
 *              ( norm1(A),         NORM = dplasmaOneNorm
 *              (
 *              ( normI(A),         NORM = dplasmaInfNorm
 *              (
 *              ( normF(A),         NORM = dplasmaFrobeniusNorm
 *
 *  where norm1 denotes the one norm of a matrix (maximum column sum),
 *  normI denotes the infinity norm of a matrix (maximum row sum) and
 *  normF denotes the Frobenius norm of a matrix (square root of sum
 *  of squares). Note that max(abs(A(i,j))) is not a consistent matrix
 *  norm.
 *
 *******************************************************************************
 *
 * @param[in,out] parsec
 *          The parsec context of the application that will run the operation.
 *
 * @param[in] norm
 *          = dplasmaMaxNorm: Max norm
 *          = dplasmaOneNorm: One norm
 *          = dplasmaInfNorm: Infinity norm
 *          = dplasmaFrobeniusNorm: Frobenius norm
 *
 * @param[in] uplo
 *          = dplasmaUpper: Upper triangle of A is stored;
 *          = dplasmaLower: Lower triangle of A is stored.
 *
 * @param[in] diag
 *          = dplasmaNonUnit: Non-unit diagonal
 *          = dplasmaUnit: Unit diagonal
 *
 * @param[in] A
 *          The descriptor of the matrix A.
 *          Must be a two_dim_rectangle_cyclic matrix.
 *
*******************************************************************************
 *
 * @return
 *          \retval the computed norm described above.
 *
 *******************************************************************************
 *
 * @sa dplasma_zlantr_New
 * @sa dplasma_zlantr_Destruct
 * @sa dplasma_clantr
 * @sa dplasma_dlantr
 * @sa dplasma_slantr
 *
 ******************************************************************************/
double
dplasma_zlantr( parsec_context_t *parsec,
                dplasma_enum_t norm, dplasma_enum_t uplo, dplasma_enum_t diag,
                const parsec_tiled_matrix_t *A)
{
    double result = 0.;
    parsec_taskpool_t *parsec_zlantr = NULL;

    if ( (norm != dplasmaMaxNorm) && (norm != dplasmaOneNorm)
        && (norm != dplasmaInfNorm) && (norm != dplasmaFrobeniusNorm) ) {
        dplasma_error("dplasma_zlantr", "illegal value of norm");
        return -2.;
    }
    if ( !(A->dtype & parsec_matrix_block_cyclic_type) ) {
        dplasma_error("dplasma_zlantr", "illegal type of descriptor for A");
        return -3.;
    }

    parsec_zlantr = dplasma_zlantr_New(norm, uplo, diag, A, &result);

    if ( parsec_zlantr != NULL )
    {
        parsec_context_add_taskpool( parsec, (parsec_taskpool_t*)parsec_zlantr);
        dplasma_wait_until_completion(parsec);
        dplasma_zlantr_Destruct( parsec_zlantr );
    }

    return result;
}

