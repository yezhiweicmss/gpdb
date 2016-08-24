// Required by building CPP dynamic library via Makefile of GPDB.
#define PGDLLIMPORT "C"

#include <signal.h>
#include <cstdlib>

#include <openssl/err.h>
#include <pthread.h>

#include "postgres.h"

#include "access/extprotocol.h"
#include "catalog/pg_proc.h"
#include "fmgr.h"
#include "funcapi.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/memutils.h"

#include "gpreader.h"
#include "gpwriter.h"

#include "s3common.h"
#include "s3conf.h"
#include "s3log.h"
#include "s3utils.h"

/* Do the module magic dance */
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(s3_export);
PG_FUNCTION_INFO_V1(s3_import);

extern "C" {
Datum s3_export(PG_FUNCTION_ARGS);
Datum s3_import(PG_FUNCTION_ARGS);
}

/*
 * Import data into GPDB.
 * invoked by GPDB, be careful with C++ exceptions.
 */
Datum s3_import(PG_FUNCTION_ARGS) {
    /* Must be called via the external table format manager */
    if (!CALLED_AS_EXTPROTOCOL(fcinfo))
        elog(ERROR, "extprotocol_import: not called by external protocol manager");

    /* Get our internal description of the protocol */
    GPReader *gpreader = (GPReader *)EXTPROTOCOL_GET_USER_CTX(fcinfo);

    /* last call. destroy reader */
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        thread_cleanup();

        if (!reader_cleanup(&gpreader)) {
            ereport(ERROR,
                    (0, errmsg("Failed to cleanup S3 extension: %s", s3extErrorMessage.c_str())));
        }

        EXTPROTOCOL_SET_USER_CTX(fcinfo, NULL);

        PG_RETURN_INT32(0);
    }

    /* first call. do any desired init */
    if (gpreader == NULL) {
        const char *url_with_options = EXTPROTOCOL_GET_URL(fcinfo);

        thread_setup();

        gpreader = reader_init(url_with_options);
        if (!gpreader) {
            ereport(ERROR, (0, errmsg("Failed to init S3 extension, segid = %d, "
                                      "segnum = %d, please check your "
                                      "configurations and net connection: %s",
                                      s3ext_segid, s3ext_segnum, s3extErrorMessage.c_str())));
        }

        EXTPROTOCOL_SET_USER_CTX(fcinfo, gpreader);
    }

    char *data_buf = EXTPROTOCOL_GET_DATABUF(fcinfo);
    int32 data_len = EXTPROTOCOL_GET_DATALEN(fcinfo);

    if (!reader_transfer_data(gpreader, data_buf, data_len)) {
        ereport(ERROR,
                (0, errmsg("s3_import: could not read data: %s", s3extErrorMessage.c_str())));
    }
    PG_RETURN_INT32(data_len);
}

/*
 * Export data out of GPDB.
 * invoked by GPDB, be careful with C++ exceptions.
 */
Datum s3_export(PG_FUNCTION_ARGS) {
    /* Must be called via the external table format manager */
    if (!CALLED_AS_EXTPROTOCOL(fcinfo))
        elog(ERROR, "extprotocol_import: not called by external protocol manager");

    /* Get our internal description of the protocol */
    GPWriter *gpwriter = (GPWriter *)EXTPROTOCOL_GET_USER_CTX(fcinfo);

    /* last call. destroy writer */
    if (EXTPROTOCOL_IS_LAST_CALL(fcinfo)) {
        thread_cleanup();

        if (!writer_cleanup(&gpwriter)) {
            ereport(ERROR,
                    (0, errmsg("Failed to cleanup S3 extension: %s", s3extErrorMessage.c_str())));
        }

        EXTPROTOCOL_SET_USER_CTX(fcinfo, NULL);

        PG_RETURN_INT32(0);
    }

    /* first call. do any desired init */
    if (gpwriter == NULL) {
        const char *url_with_options = EXTPROTOCOL_GET_URL(fcinfo);

        thread_setup();

        gpwriter = writer_init(url_with_options);
        if (!gpwriter) {
            ereport(ERROR, (0, errmsg("Failed to init S3 extension, segid = %d, "
                                      "segnum = %d, please check your "
                                      "configurations and net connection: %s",
                                      s3ext_segid, s3ext_segnum, s3extErrorMessage.c_str())));
        }

        EXTPROTOCOL_SET_USER_CTX(fcinfo, gpwriter);
    }

    char *data_buf = EXTPROTOCOL_GET_DATABUF(fcinfo);
    int32 data_len = EXTPROTOCOL_GET_DATALEN(fcinfo);

    if (!writer_transfer_data(gpwriter, data_buf, data_len)) {
        ereport(ERROR,
                (0, errmsg("s3_export: could not write data: %s", s3extErrorMessage.c_str())));
    }

    PG_RETURN_INT32(data_len);
}
