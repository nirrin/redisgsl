#ifndef REDISGSL_RSTAT_H
#define REDISGSL_RSTAT_H

#include <stdlib.h>
#include <gsl/gsl_rstat.h>
#include "redismodule.h"
#include "gsl_serialize.h"

static RedisModuleType* rstat_quantile_type = NULL;
static RedisModuleType* rstat_type = NULL;

int gsl_rstat_quantile_workspace_set_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
   	RedisModule_AutoMemory(ctx);
    if (argc != 24) {
    	return RedisModule_WrongArity(ctx);
    }
    RedisModuleKey* key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    const int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY) && (RedisModule_ModuleTypeGetType(key) != rstat_quantile_type)
    	return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    }
    double p = 0;
    if (RedisModule_StringToDouble(argv[2], &p) != REDISMODULE_OK) {
    	return RedisModule_ReplyWithError(ctx, "invalid p-quantile");
    }
	gsl_rstat_quantile_workspace* gsl_rstat_quantile_alloc(p);
	for (size_t i = 0; i < 5; i++) {
		if (RedisModule_StringToDouble(argv[3 + i], &(w->q[i])) != REDISMODULE_OK) {
    		return RedisModule_ReplyWithError(ctx, "invalid heights q_i");
    	}
	}
	for (size_t i = 0; i < 5; i++) {
		long long int npos = 0;
		if (RedisModule_StringToLongLong(argv[8 + i], &npos) != REDISMODULE_OK) {
    		return RedisModule_ReplyWithError(ctx, "invalid positions n_i");
    	}
    	w->npos[i] = (int)npos;
	}
	for (size_t i = 0; i < 5; i++) {
		if (RedisModule_StringToDouble(argv[13 + i], &(w->np[i])) != REDISMODULE_OK) {
    		return RedisModule_ReplyWithError(ctx, "invalid desired positions n_i'");
    	}
	}
	for (size_t i = 0; i < 5; i++) {
		if (RedisModule_StringToDouble(argv[18 + i], &(w->dnp[i])) != REDISMODULE_OK) {
    		return RedisModule_ReplyWithError(ctx, "invalid increments dn_i'");
    	}
	}
	long long int n = 0;
	if (RedisModule_StringToLongLong(argv[23], &n) != REDISMODULE_OK) {
		return RedisModule_ReplyWithError(ctx, "invalid number of data added");
	}
	w->n = (size_t)n;
	if (RedisModule_ModuleTypeSetValue(key, rstat_quantile_type, w) != REDISMODULE_OK) {
		return RedisModule_ReplyWithError(ctx, "set failed");
	}
	return REDISMODULE_OK;    
}

void* gsl_rstat_quantile_workspace_RdbLoad(RedisModuleIO *rdb, int encver) {
    if (encver != ENCODING_VER) {
        return NULL;
    }
    const double p = RedisModule_LoadDouble(rdb);
    gsl_rstat_quantile_workspace* w = gsl_rstat_quantile_alloc(p);
    for (size_t i = 0; i < 5; i++) {
    	w->q[i] = RedisModule_LoadDouble(rdb);
    }
    for (size_t i = 0; i < 5; i++) {
    	w->npos[i] = (int)RedisModule_LoadSigned(rdb);
    }
    for (size_t i = 0; i < 5; i++) {
    	w->np[i] = RedisModule_LoadDouble(rdb);    	
    }
    for (size_t i = 0; i < 5; i++) {
    	w->dnp[i] = RedisModule_LoadDouble(rdb);
    }
    w->n = (size_t)RedisModule_LoadUnsigned(rdb);
    return (void*)w;
}

void gsl_rstat_quantile_workspace_RdbSave(RedisModuleIO *rdb, void* p) {
    gsl_rstat_quantile_workspace* w = (gsl_rstat_quantile_workspace*)p;
    RedisModule_SaveDouble(rdb, w->p);
    for (size_t i = 0; i < 5; i++) {
    	RedisModule_SaveDouble(rdb, w->q[i]);
    }
    for (size_t i = 0; i < 5; i++) {
    	RedisModule_SaveSigned(rdb, (int64_t)w->npos[i]);
    }
    for (size_t i = 0; i < 5; i++) {
    	RedisModule_SaveDouble(rdb, w->np[i]);    	
    }
    for (size_t i = 0; i < 5; i++) {
    	RedisModule_SaveDouble(rdb, w->dnp[i]);
    }
    RedisModule_SaveUnsigned(rdb, (uint64_t)w->n);
    return (void*)w;
}

void gsl_rstat_quantile_workspace_AofRewrite(RedisModuleIO* aof, RedisModuleString* key, void* value) {
    gsl_rstat_quantile_workspace* w = (gsl_rstat_quantile_workspace*)value;
    RedisModule_EmitAOF(aof, "gsl_rstat_quantile_workspace_set", "ss", key, gsl_rstat_quantile_workspace_serialize(w).c_str());
}

size_t gsl_rstat_quantile_workspace_MemUsage(const void __attribute__((unused)) *p) {
	return sizeof(gsl_rstat_quantile_workspace);
}

void gsl_rstat_quantile_workspace_Free(void* p) {
    gsl_rstat_quantile_free((gsl_rstat_quantile_workspace*)p);
}

void gsl_rstat_workspace_RdbSave(RedisModuleIO *rdb, void* p) {
    gsl_rstat_quantile_workspace* w = (gsl_rstat_quantile_workspace*)p;
	RedisModule_SaveDouble(rdb, w->min);
	RedisModule_SaveDouble(rdb, w->max);
	RedisModule_SaveDouble(rdb, w->mean);
	RedisModule_SaveDouble(rdb, w->M2);
	RedisModule_SaveDouble(rdb, w->M3);
	RedisModule_SaveDouble(rdb, w->M4);	
	RedisModule_SaveUnsigned(rdb, (uint64_t)w->n);
	gsl_rstat_quantile_workspace_RdbSave(rdb, (void*)w->median_workspace_p);
}

void* gsl_rstat_workspace_RdbLoad(RedisModuleIO *rdb, int encver) {
    if (encver != ENCODING_VER) {
        return NULL;
    }
    gsl_rstat_workspace* w = gsl_rstat_alloc();
    w->min = RedisModule_LoadDouble(rdb);
    w->max = RedisModule_LoadDouble(rdb);
    w->min = RedisModule_LoadDouble(rdb);
    w->M2 = RedisModule_LoadDouble(rdb);
    w->M3 = RedisModule_LoadDouble(rdb);
    w->M4 = RedisModule_LoadDouble(rdb);
    w->n = RedisModule_LoadUnsigned(rdb);
    w->median_workspace_p = (gsl_rstat_quantile_workspace*)gsl_rstat_quantile_workspace_RdbLoad(rdb, encver);
    return (void*)w;
}

size_t gsl_rstat_workspace_MemUsage(const void __attribute__((unused)) *p) {
	return sizeof(gsl_rstat_workspace) + gsl_rstat_quantile_workspace_MemUsage(p);
}

void gsl_rstat_workspace_Free(void* p) {
    gsl_rstat_free((gsl_rstat_workspace*)p);
}

#define RSTAT_QUANTILE_ENCODING_VERSION 0
#define RSTAT_ENCODING_VERSION 0

inline int gsl_rstat_on_load(RedisModuleCtx* ctx) {
	RedisModuleTypeMethods gsl_rstat_quantile_workspace_type_methods = {
    	.version = REDISMODULE_TYPE_METHOD_VERSION,
    	.rdb_load = gsl_rstat_quantile_workspace_RdbLoad,
    	.rdb_save = gsl_rstat_quantile_workspace_RdbSave,
    	.aof_rewrite = gsl_rstat_quantile_workspace_AofRewrite,
    	.free = gsl_rstat_quantile_workspace_Free
	};

    rstat_quantile_type = RedisModule_CreateDataType(ctx, "RSTTQ-GSL", RSTAT_QUANTILE_ENCODING_VERSION, &gsl_rstat_quantile_workspace_type_methods);
    if (rstat_quantile_type == NULL) {
    	return REDISMODULE_ERR;
    }
}


#endif