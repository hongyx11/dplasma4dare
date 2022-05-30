/*
 * Copyright (c) 2009-2020 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * @precisions normal z -> s d c
 *
 */

#include "common.h"
#include "flops.h"
#include "parsec/data_dist/matrix/sym_two_dim_rectangle_cyclic.h"
#include "parsec/data_dist/matrix/two_dim_rectangle_cyclic.h"

int main(int argc, char ** argv)
{
    parsec_context_t* parsec;
    int iparam[IPARAM_SIZEOF];
    dplasma_enum_t uplo = dplasmaLower;
    int i, info, ret = 0;

    /* Set defaults for non argv iparams */
    iparam_default_facto(iparam);
    iparam_default_ibnbmb(iparam, 0, 180, 180);

    /* Initialize PaRSEC */
    parsec = setup_parsec(argc, argv, iparam);
    PASTE_CODE_IPARAM_LOCALS(iparam);

    /* initializing matrix structure */
    LDA = max( LDA, M );
    LDB = max( LDB, M );
    KP = 1;
    KQ = 1;

    PASTE_CODE_ALLOCATE_MATRIX(dcA, 1,
        parsec_matrix_sym_block_cyclic, (&dcA, PARSEC_MATRIX_COMPLEX_DOUBLE,
                                   rank, MB, NB, LDA, N, 0, 0,
                                   M, N, P, nodes/P, uplo));

    PASTE_CODE_ALLOCATE_MATRIX(dcB, 1,
        parsec_matrix_block_cyclic, (&dcB, PARSEC_MATRIX_COMPLEX_DOUBLE,  PARSEC_MATRIX_TILE,
                               rank, MB, NB, LDB, N, 0, 0,
                               M, N, P, nodes/P, KP, KQ, IP, JQ));

    /* matrix generation */
    if(loud > 2) printf("+++ Generate matrices ... ");
    ret |= dplasma_zplghe( parsec, (double)(N), uplo,
                    (parsec_tiled_matrix_t *)&dcA, 3872);
    ret |= dplasma_zplrnt( parsec, 0, (parsec_tiled_matrix_t *)&dcB, 2354);
    if(loud > 2) printf("Done\n");

    ret |= dplasma_zprint( parsec, uplo,             (parsec_tiled_matrix_t *)&dcA );
    ret |= dplasma_zprint( parsec, dplasmaUpperLower, (parsec_tiled_matrix_t *)&dcB );

    for(i=0; i<43; i++) {
        if ( rank == 0 ) {
            fprintf(stdout, "====== Generate Test Matrix %d ======\n", i);
            fflush(stdout);
        }
        info = dplasma_zpltmg( parsec, i, (parsec_tiled_matrix_t *)&dcB, 5373 );
        if (info == 0)
            ret |= dplasma_zprint( parsec, dplasmaUpperLower, (parsec_tiled_matrix_t *)&dcB );
    }

    parsec_data_free(dcB.mat);
    parsec_tiled_matrix_destroy( (parsec_tiled_matrix_t*)&dcB );
    parsec_data_free(dcA.mat);
    parsec_tiled_matrix_destroy( (parsec_tiled_matrix_t*)&dcA);

    cleanup_parsec(parsec, iparam);

    return ret;
}
