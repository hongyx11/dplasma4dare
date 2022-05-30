/*
 * Copyright (c) 2010-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2013      Inria. All rights reserved.
 * $COPYRIGHT
 *
 */

#include "dplasma.h"
#include "dplasma/types.h"
#include "dplasmaaux.h"

#include "map.h"

/**
 *******************************************************************************
 *
 * @ingroup dplasma
 *
 * dplasma_map_New - Generates an taskpool that performs a map operation with
 * two similar matrices, applying the operator on each tile of A:
 *
 *    operator( A )
 *
 * See dplasma_zlaset_New() or dplasma_zlascal_New() as example of function using
 * the map operator.
 *
 * WARNING: The computations are not done by this call.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          Specifies on which part of matrix A, the operator must be
 *          applied
 *          = dplasmaUpperLower: All matrix is referenced.
 *          = dplasmaUpper:      Only upper part is refrenced.
 *          = dplasmaLower:      Only lower part is referenced.
 *
 * @param[in,out] A
 *          Descriptor of the distributed matrix A on which operator is applied.
 *
 * @param[in] operator
 *          Unary operator describing the operation to perform on each couple of
 *          tiles. The parameters of the descriptor are: operator(
 *          execution_context, tileA, op_args, uplo, m, n), where
 *          execution_context is the execution context that runs the operator,
 *          tileA, the pointer to the tile of matrix A, op_args the parameters
 *          given to each operator call, uplo the part of the tiles on which the
 *          operator is working, and (m,n) the tile indices.
 *
 * @param[in] op_args
 *          Arguments given to each call to the unary operator.
 *
 *******************************************************************************
 *
 * @return
 *          \retval NULL if incorrect parameters are given.
 *          \retval The parsec taskpool describing the operation that can be
 *          enqueued in the runtime with parsec_context_add_taskpool(). It, then, needs to be
 *          destroy with dplasma_map_Destruct();
 *
 *******************************************************************************
 *
 * @sa dplasma_map
 * @sa dplasma_map_Destruct
 *
 ******************************************************************************/
parsec_taskpool_t *
dplasma_map_New( dplasma_enum_t uplo,
                 parsec_tiled_matrix_t *A,
                 parsec_tiled_matrix_unary_op_t operator,
                 void *op_args )
{
    parsec_map_taskpool_t *parsec_map = NULL;

    if ((uplo != dplasmaUpperLower) &&
        (uplo != dplasmaUpper)      &&
        (uplo != dplasmaLower))
    {
        dplasma_error("dplasma_map", "illegal value of uplo");
        return NULL;
    }

    parsec_map = parsec_map_new( uplo,
                               A,
                               operator, op_args);

    switch( A->mtype ) {
    case PARSEC_MATRIX_COMPLEX_DOUBLE :
        dplasma_add2arena_tile( &parsec_map->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX],
                                A->mb*A->nb*sizeof(dplasma_complex64_t),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_double_complex_t, A->mb);
        break;
    case PARSEC_MATRIX_COMPLEX_FLOAT  :
        dplasma_add2arena_tile( &parsec_map->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX],
                                A->mb*A->nb*sizeof(dplasma_complex32_t),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_complex_t, A->mb);
        break;
    case PARSEC_MATRIX_DOUBLE    :
        dplasma_add2arena_tile( &parsec_map->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX],
                                A->mb*A->nb*sizeof(double),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_double_t, A->mb);
        break;
    case PARSEC_MATRIX_FLOAT     :
        dplasma_add2arena_tile( &parsec_map->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX],
                                A->mb*A->nb*sizeof(float),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_float_t, A->mb);
        break;
    case PARSEC_MATRIX_INTEGER       :
    default:
        dplasma_add2arena_tile( &parsec_map->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX],
                                A->mb*A->nb*sizeof(int),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_int_t, A->mb);
    }


    return (parsec_taskpool_t*)parsec_map;
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma
 *
 *  dplasma_map_Destruct - Free the data structure associated to an taskpool
 *  created with dplasma_map_New().
 *
 *******************************************************************************
 *
 * @param[in,out] taskpool
 *          On entry, the taskpool to destroy.
 *          On exit, the taskpool cannot be used anymore.
 *
 *******************************************************************************
 *
 * @sa dplasma_map_New
 * @sa dplasma_map
 *
 ******************************************************************************/
void
dplasma_map_Destruct( parsec_taskpool_t *tp )
{
    parsec_map_taskpool_t *omap = (parsec_map_taskpool_t *)tp;

    if ( omap->_g_op_args ) {
        free( omap->_g_op_args );
    }

    dplasma_matrix_del2arena( &omap->arenas_datatypes[PARSEC_map_DEFAULT_ADT_IDX] );

    parsec_taskpool_free(tp);
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma
 *
 * dplasma_map_New - Performs a map operation with two similar matrices,
 * applying the operator on each tile of A:
 *
 *    operator( A )
 *
 * See dplasma_zlaset() or dplasma_zlascal() as example of function using
 * the map operator.
 *
 *******************************************************************************
 *
 * @param[in,out] parsec
 *          The parsec context of the application that will run the operation.
 *
 * @param[in] uplo
 *          Specifies on which part of matrix A, the operator must be
 *          applied
 *          = dplasmaUpperLower: All matrix is referenced.
 *          = dplasmaUpper:      Only upper part is refrenced.
 *          = dplasmaLower:      Only lower part is referenced.
 *
 * @param[in,out] A
 *          Descriptor of the distributed matrix A on which operator is applied.
 *
 * @param[in] operator
 *          Unary operator describing the operation to perform on each couple of
 *          tiles. The parameters of the descriptor are: operator(
 *          execution_context, tileA, op_args, uplo, m, n), where
 *          execution_context is the execution context that runs the operator,
 *          tileA, the pointer to the tile of matrix A, op_args the parameters
 *          given to each operator call, uplo the part of the tiles on which the
 *          operator is working, and (m,n) the tile indices.
 *
 * @param[in] op_args
 *          Arguments given to each call to the unary operator.
 *
 *******************************************************************************
 *
 * @return
 *          \retval -i if the ith parameters is incorrect.
 *          \retval 0 on success.
 *
 *******************************************************************************
 *
 * @sa dplasma_map_New
 * @sa dplasma_map_Destruct
 *
 ******************************************************************************/
int
dplasma_map( parsec_context_t *parsec,
             dplasma_enum_t uplo,
             parsec_tiled_matrix_t *A,
             parsec_tiled_matrix_unary_op_t operator,
             void *op_args )
{
    parsec_taskpool_t *parsec_map = NULL;

    if ((uplo != dplasmaUpperLower) &&
        (uplo != dplasmaUpper)      &&
        (uplo != dplasmaLower))
    {
        dplasma_error("dplasma_map", "illegal value of uplo");
        return -2;
    }

    parsec_map = dplasma_map_New( uplo, A, operator, op_args );

    if ( parsec_map != NULL )
    {
        parsec_context_add_taskpool( parsec, parsec_map );
        dplasma_wait_until_completion( parsec );
        dplasma_map_Destruct( parsec_map );
    }

    return 0;
}
