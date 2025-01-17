/*
 * Copyright (c) 2015-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2021      Nanook Consulting.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "test_spawn.h"
#include <time.h>

typedef struct {
    int in_progress;
    pmix_nspace_t nspace;
} spawn_cbdata;

static void spawn_cb(pmix_status_t status,
                      char nspace[], void *cbdata)
{
    spawn_cbdata *cb = (spawn_cbdata*)cbdata;

    PMIX_LOAD_NSPACE(cb->nspace, nspace);
    cb->in_progress = 0;
}

static int test_spawn_common(char *my_nspace, int my_rank, int blocking)
{
    int rc;
    pmix_app_t *apps;
    size_t napps;
    pmix_nspace_t nspace;
    memset(nspace, 0, PMIX_MAX_NSLEN+1);
    napps = 1;
    PMIX_APP_CREATE(apps, napps);
    apps[0].cmd = strdup("foo");  // need SOMETHING we intend to spawn!
    if (blocking) {
        if (PMIX_SUCCESS != (rc = PMIx_Spawn(NULL, 0, apps, napps, nspace))) {
            PMIX_APP_FREE(apps, napps);
            exit(rc);
        }
    } else {
        spawn_cbdata cbdata;
        cbdata.in_progress = 1;
        memset(cbdata.nspace, 0, PMIX_MAX_NSLEN+1);
        rc = PMIx_Spawn_nb(NULL, 0, apps, napps, spawn_cb, (void*)&cbdata);
        if (PMIX_SUCCESS != rc) {
            PMIX_APP_FREE(apps, napps);
            exit(rc);
        }
        PMIX_WAIT_FOR_COMPLETION(cbdata.in_progress);
        PMIX_LOAD_NSPACE(nspace, cbdata.nspace);
    }
    PMIX_APP_FREE(apps, napps);
    if (!PMIX_CHECK_NSPACE(nspace, "foobar")) {
        exit(PMIX_ERROR);
    }
    return rc;
}

int test_spawn(char *my_nspace, int my_rank)
{
    int rc;
    rc = test_spawn_common(my_nspace, my_rank, 1);
    if (PMIX_SUCCESS != rc) {
        TEST_ERROR(("%s:%d: Spawn blocking test failed.", my_nspace, my_rank));
        exit(rc);
    }
    TEST_VERBOSE(("%s:%d: Spawn blocking test succeded.", my_nspace, my_rank));
    rc = test_spawn_common(my_nspace, my_rank, 0);
    if (PMIX_SUCCESS != rc) {
        TEST_ERROR(("%s:%d: Spawn non-blocking test failed.", my_nspace, my_rank));
        exit(rc);
    }
    TEST_VERBOSE(("%s:%d: Spawn non-blocking test succeded.", my_nspace, my_rank));
    return PMIX_SUCCESS;
}
