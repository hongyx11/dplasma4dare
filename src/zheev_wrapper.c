/*
 * Copyright (c) 2010-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2013      Inria. All rights reserved.
 * $COPYRIGHT
 *
 * @precisions normal z -> s d c
 *
 */

#include "dplasma.h"
#include "dplasma/types.h"
#include "dplasmaaux.h"

#include "parsec/data_dist/matrix/two_dim_rectangle_cyclic.h"
#include "parsec/data_dist/matrix/sym_two_dim_rectangle_cyclic.h"
#include "parsec/data_dist/matrix/diag_band_to_rect.h"
#include "zhbrdt.h"

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 * dplasma_zheev_New - TO FILL IN CORRECTLY BY THE PERSON WORKING ON IT !!!
 *
 * WARNING: The computations are not done by this call.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *
 * @param[in,out] A
 *
 * @param[out] info
 *
 *******************************************************************************
 *
 * @return
 *
 *******************************************************************************
 *
 * @sa dplasma_zheev
 * @sa dplasma_zheev_Destruct
 * @sa dplasma_cheev_New
 * @sa dplasma_dheev_New
 * @sa dplasma_sheev_New
 *
 ******************************************************************************/
parsec_taskpool_t*
dplasma_zheev_New(dplasma_enum_t jobz, dplasma_enum_t uplo,
                  parsec_tiled_matrix_t* A,
                  parsec_tiled_matrix_t* W,  /* Should be removed: internal workspace as T */
                  parsec_tiled_matrix_t* Z,
                  int* info )
{
    (void)Z;
    *info = 0;

    /* Check input arguments */
    if( (jobz != dplasmaNoVec) && (jobz != dplasmaVec) ) {
        dplasma_error("dplasma_zheev_New", "illegal value of jobz");
        *info = -1;
        return NULL;
    }
    if( (uplo != dplasmaLower) && (uplo != dplasmaUpper) ) {
        dplasma_error("dplasma_zheev_New", "illegal value of uplo");
        *info = -2;
        return NULL;
    }

    /* TODO: remove those extra check when those options will be implemented */
    if( jobz == dplasmaVec ) {
        dplasma_error("dplasma_zheev_New", "dplasmaVec not implemented (yet)");
        *info = -1;
        return NULL;
    }

    if( dplasmaLower == uplo ) {
        parsec_taskpool_t* zherbt_obj, * zhbrdt_obj;
        parsec_diag_band_to_rect_taskpool_t* band2rect_obj;
        parsec_taskpool_t* zheev_compound;
        parsec_matrix_sym_block_cyclic_t* As = (parsec_matrix_sym_block_cyclic_t*)A;
        int ib=A->nb/3;

        parsec_matrix_block_cyclic_t* T = calloc(1, sizeof(parsec_matrix_block_cyclic_t));
        parsec_matrix_block_cyclic_init(T, PARSEC_MATRIX_COMPLEX_DOUBLE, PARSEC_MATRIX_TILE,
             A->super.myrank, ib, A->nb, A->mt*ib, A->n, 0, 0,
             A->mt*ib, A->n, As->grid.rows, As->grid.cols, As->grid.krows, As->grid.krows, As->grid.ip, As->grid.jq);
        T->mat = parsec_data_allocate((size_t)T->super.nb_local_tiles *
                                     (size_t)T->super.bsiz *
                                     (size_t)parsec_datadist_getsizeoftype(T->super.mtype));
        parsec_data_collection_set_key((parsec_data_collection_t*)T, "zheev_dcT");

        zherbt_obj = (parsec_taskpool_t*)dplasma_zherbt_New( uplo, ib, A, (parsec_tiled_matrix_t*)T );
        band2rect_obj = parsec_diag_band_to_rect_new((parsec_matrix_sym_block_cyclic_t*)A, (parsec_matrix_block_cyclic_t*)W,
                A->mt, A->nt, A->mb, A->nb, sizeof(dplasma_complex64_t));
        zhbrdt_obj = (parsec_taskpool_t*)dplasma_zhbrdt_New(W);
        zheev_compound = parsec_compose( zherbt_obj, (parsec_taskpool_t*)band2rect_obj );
        zheev_compound = parsec_compose( zheev_compound, zhbrdt_obj );

        parsec_arena_datatype_t* adt = &band2rect_obj->arenas_datatypes[PARSEC_diag_band_to_rect_DEFAULT_ADT_IDX];
        dplasma_add2arena_tile( adt,
                                A->mb*A->nb*sizeof(dplasma_complex64_t),
                                PARSEC_ARENA_ALIGNMENT_SSE,
                                parsec_datatype_double_complex_t, A->mb );

        return zheev_compound;
    }
    dplasma_error("dplasma_zheev_New", "dplasmaUpper not implemented (yet)");
    *info = -2;
    return NULL;
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 *  dplasma_zheev_Destruct - Free the data structure associated to an taskpool
 *  created with dplasma_zheev_New().
 *
 *******************************************************************************
 *
 * @param[in,out] taskpool
 *          On entry, the taskpool to destroy.
 *          On exit, the taskpool cannot be used anymore.
 *
 *******************************************************************************
 *
 * @sa dplasma_zheev_New
 * @sa dplasma_zheev
 *
 ******************************************************************************/
void
dplasma_zheev_Destruct( parsec_taskpool_t *tp )
{
#if 0
    parsec_matrix_block_cyclic_t* T = ???
    parsec_data_free(T->mat);
    parsec_tiled_matrix_destroy((parsec_tiled_matrix_t*)T); free(T);

    dplasma_matrix_del2arena( &((parsec_diag_band_to_rect_taskpool_t *)tp)->arenas_datatypes[PARSEC_diag_band_to_rect_DEFAULT_ADT_IDX] );
#endif
    parsec_taskpool_free(tp);
}

/**
 *******************************************************************************
 *
 * @ingroup dplasma_complex64
 *
 * dplasma_zheev - TO FILL IN CORRECTLY BY THE PERSON WORKING ON IT !!!
 *
 *******************************************************************************
 *
 * @param[in,out] parsec
 *          The parsec context of the application that will run the operation.
 *
 * COPY OF NEW INTERFACE
 *
 *******************************************************************************
 *
 * @return
 *
 *******************************************************************************
 *
 * @sa dplasma_zheev_New
 * @sa dplasma_zheev_Destruct
 * @sa dplasma_cheev
 * @sa dplasma_dheev
 * @sa dplasma_sheev
 *
 ******************************************************************************/
int
dplasma_zheev( parsec_context_t *parsec,
               dplasma_enum_t jobz, dplasma_enum_t uplo,
               parsec_tiled_matrix_t* A,
               parsec_tiled_matrix_t* W, /* Should be removed */
               parsec_tiled_matrix_t* Z )
{
    parsec_taskpool_t *parsec_zheev = NULL;
    int info = 0;

    /* Check input arguments */
    if( (jobz != dplasmaNoVec) && (jobz != dplasmaVec) ) {
        dplasma_error("dplasma_zheev", "illegal value of jobz");
        return -1;
    }
    if( (uplo != dplasmaLower) && (uplo != dplasmaUpper) ) {
        dplasma_error("dplasma_zheev", "illegal value of uplo");
        return -2;
    }

    parsec_zheev = dplasma_zheev_New( jobz, uplo, A, W, Z, &info );

    if ( parsec_zheev != NULL )
    {
        parsec_context_add_taskpool( parsec, (parsec_taskpool_t*)parsec_zheev);
        dplasma_wait_until_completion(parsec);
        dplasma_zheev_Destruct( parsec_zheev );
        return info;
    }
    else {
        return -101;
    }
}
