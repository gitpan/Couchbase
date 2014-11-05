#ifndef PERL_COUCHBASE_H_
#define PERL_COUCHBASE_H_
#define NO_XSLOCKS

#include <sys/types.h> /*for size_t*/
#include <libcouchbase/couchbase.h>
#include <libcouchbase/api3.h>
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"

#define PLCB_BKT_CLASSNAME "Couchbase::Bucket"
#define PLCB_RET_CLASSNAME "Couchbase::Document"
#define PLCB_OPCTX_CLASSNAME "Couchbase::OpContext"
#define PLCB_PUB_CONSTANTS_PKG "Couchbase::Constants"
#define PLCB_PRIV_CONSTANTS_PKG "Couchbase::_GlueConstants"
#define PLCB_OBS_PLHELPER "Couchbase::Bucket::__obshelper"
#define PLCB_STATS_PLHELPER "Couchbase::Bucket::__statshelper"

#if IVSIZE >= 8
#define PLCB_PERL64
#endif
#include "plcb-util.h"

typedef struct PLCB_st PLCB_t;

enum {
    PLCB_CONVERTERS_CUSTOM = 1,
    PLCB_CONVERTERS_JSON,
    PLCB_CONVERTERS_STORABLE
};

enum {
    PLCB_SETTING_INT,
    PLCB_SETTING_UINT,
    PLCB_SETTING_U32,
    PLCB_SETTING_SIZE,
    PLCB_SETTING_STRING,
    PLCB_SETTING_TIMEOUT
};

enum {
    PLCB_CMD_GET,
    PLCB_CMD_GAT,
    PLCB_CMD_TOUCH,
    PLCB_CMD_LOCK,
    PLCB_CMD_SET,
    PLCB_CMD_ADD,
    PLCB_CMD_REPLACE,
    PLCB_CMD_COUNTER,
    PLCB_CMD_APPEND,
    PLCB_CMD_PREPEND,
    PLCB_CMD_REMOVE,
    PLCB_CMD_UNLOCK,
    PLCB_CMD_STATS,
    PLCB_CMD_KEYSTATS,
    PLCB_CMD_OBSERVE,
};

enum {
    PLCB_RETIDX_PARENT = 0,
    PLCB_RETIDX_KEY,
    PLCB_RETIDX_VALUE,
    PLCB_RETIDX_ERRNUM,
    PLCB_RETIDX_CAS,
    PLCB_RETIDX_OPTIONS,
    PLCB_RETIDX_EXP,
    PLCB_RETIDX_FMTSPEC,
    PLCB_RETIDX_MAX
};

enum {
    PLCB_OPCTXIDX_FLAGS = 0,
    PLCB_OPCTXIDX_CBO,
    PLCB_OPCTXIDX_REMAINING,
    PLCB_OPCTXIDX_QUEUE,
    PLCB_OPCTXIDX_EXTRA
};

enum {
    PLCB_LF_JSON = 0x00,
    PLCB_LF_STORABLE = 0x01 << 3,
    PLCB_LF_RAW = 0x03 << 3,
    PLCB_LF_UTF8 = 0x04 << 3,
    PLCB_LF_MASK = 0xFF,

    PLCB_CF_NONE,
    PLCB_CF_PRIVATE = 0x01 << 24,
    PLCB_CF_STORABLE = PLCB_CF_PRIVATE,

    PLCB_CF_JSON = 0x02 << 24,
    PLCB_CF_RAW = 0x03 << 24,
    PLCB_CF_UTF8 = 0x04 << 24,
    PLCB_CF_MASK = 0xFF << 24
};

struct PLCB_st {
    lcb_t instance; /*our library handle*/
    HV *ret_stash; /*stash with which we bless our return objects*/
    HV *view_stash;
    HV *design_stash;
    HV *handle_av_stash;
    HV *opctx_sync_stash;
    HV *opctx_cb_stash;

    int connected;
    
    SV *cv_serialize;
    SV *cv_deserialize;
    SV *cv_jsonenc;
    SV *cv_jsondec;
    SV *cv_customenc;
    SV *cv_customdec;

    SV *deflctx;
    SV *curctx;
    SV *selfobj;

    /*how many operations are pending on this object*/
    int npending;
};

typedef struct {
    unsigned nremaining;
    unsigned flags;
    SV *parent; /* PLCB_T */
    AV *ctxqueue; /* For queued operations */
} plcb_OPCTX;

typedef struct {
    int cmdbase; /* Effective command passed, without flags or modifiers */
    PLCB_t *parent;
    AV *docav; /* The document */
    SV *opctx; /* The context */
    SV *cmdopts; /* Command options */
    SV *docrv; /* Reference for the document */
    void *cookie;
} plcb_SINGLEOP;

/* Temporary structure used for encoding/storing values */
typedef struct {
    SV *value;
    uint32_t flags;
    uint32_t spec;
    short need_free;
    const char *encoded;
    size_t len;
} plcb_DOCVAL;

#define PLCB_OPCTXf_IMPLICIT 0x01
#define PLCB_OPCTXf_CALLBACKS 0x02
#define PLCB_OPCTXf_WAITONE 0x04

/*need to include this after defining PLCB_t*/
#include "plcb-return.h"
#include "perl-couchbase-couch.h"
#include "plcb-args.h"

void plcb_callbacks_setup(PLCB_t *object);

/*options for common constructor settings*/
void plcb_ctor_cbc_opts(AV *options, struct lcb_create_st *cropts);
void plcb_ctor_conversion_opts(PLCB_t *object, AV *options);
void plcb_ctor_init_common(PLCB_t *object, lcb_t instance, AV *options);

/*cleanup functions*/
void plcb_cleanup(PLCB_t *object);

/*conversion functions*/
void
plcb_convert_storage(PLCB_t* object, AV *doc, plcb_DOCVAL *vspec);

void plcb_convert_storage_free(PLCB_t *object, plcb_DOCVAL *vspec);

SV*
plcb_convert_retrieval(PLCB_t *object, AV *doc, const char *data, size_t data_len, uint32_t flags);


/**
 * This function decrements the wait count by one, and possibly calls stop_event_loop
 * if the reference count has hit 0.
 */
void plcb_evloop_wait_unref(PLCB_t *obj);

/**
 * Returns a new blessed operation context, also makes it the current
 * context
 */
SV *PLCB_opctx_new(PLCB_t *);

/** Operation functions */
SV *PLCB_op_get(PLCB_t*,plcb_SINGLEOP*);
SV *PLCB_op_set(PLCB_t*,plcb_SINGLEOP*);
SV* PLCB_op_counter(PLCB_t *object, plcb_SINGLEOP *opinfo);
SV *PLCB_op_remove(PLCB_t*,plcb_SINGLEOP*);
SV *PLCB_op_observe(PLCB_t *object, plcb_SINGLEOP *args);
SV *PLCB_op_endure(PLCB_t *object, plcb_SINGLEOP *args);
SV *PLCB_op_unlock(PLCB_t *object, plcb_SINGLEOP *args);
SV *PLCB_op_stats(PLCB_t *object, plcb_SINGLEOP *args);
SV *PLCB_op_observe(PLCB_t *object, plcb_SINGLEOP *args);

SV *
PLCB_args_return(plcb_SINGLEOP *so, lcb_error_t err);

void plcb_define_constants(void);

/* Declare these ahead of time */
XS(boot_Couchbase__View);
XS(boot_Couchbase__BucketConfig);

#endif /* PERL_COUCHBASE_H_ */
