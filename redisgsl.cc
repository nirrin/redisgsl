#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <cmath>
#include <inttypes.h>
#include "redismodule.h"
#include <gsl/gsl_rstat.h>
#include <gsl/gsl_version.h>

inline RedisModuleString* ptr_to_str(RedisModuleCtx *ctx, const void* p_) {	
	return RedisModule_CreateStringPrintf(ctx, "%" PRIu64, (unsigned long long)p_);	
}

inline void* str_to_ptr(const RedisModuleString* str_) {	
	const char* str = RedisModule_StringPtrLen(str_, NULL);	
	return (void*)atoll(str);
}

RedisModuleString* GSL_NAME = NULL;
RedisModuleKey* GSL_KEY = NULL;


inline RedisModuleString* quantile_name(RedisModuleCtx *ctx, const RedisModuleString* name, const RedisModuleString* quantile) {	
	return RedisModule_CreateStringPrintf(ctx, "%s|%s", RedisModule_StringPtrLen(name, NULL), RedisModule_StringPtrLen(quantile, NULL));	
}

inline void get_gsl_key(RedisModuleCtx *ctx) {
	if ((!GSL_NAME) || (!GSL_KEY)) {
		GSL_NAME = RedisModule_CreateString(ctx, "GSL", 3);
   		GSL_KEY = (RedisModuleKey*)RedisModule_OpenKey(ctx, GSL_NAME, REDISMODULE_READ | REDISMODULE_WRITE);
   		RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, RedisModule_CreateStringPrintf(ctx, "GSL_VERSION"), RedisModule_CreateStringPrintf(ctx, GSL_VERSION), NULL);    
	}
}

#define REPLY_WITH_OK_NULL 							RedisModule_ReplyWithNull(ctx); \
    												return REDISMODULE_OK;

#define GSL_INIT(n)									if (argc < 2) { \
    													return RedisModule_WrongArity(ctx); \
    												} \
    												RedisModule_AutoMemory(ctx); \
    												get_gsl_key(ctx);

#define GET_RSTAT_WORKSPACE 						RedisModuleString* w_ = NULL; \
													RedisModule_HashGet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], &w_, NULL); \
													if (!w_) { \
														RedisModule_ReplyWithError(ctx, "GSL Runing statistics workspace doesn't exist"); \
														return REDISMODULE_ERR; \
													} \
    												gsl_rstat_workspace* w = (gsl_rstat_workspace*)str_to_ptr(w_);

#define GET_RSTAT_QUANTILES_WORKSPACE				const RedisModuleString* name = quantile_name(ctx, argv[1], argv[2]); \
													RedisModuleString* w_ = NULL; \
													RedisModule_HashGet(GSL_KEY, REDISMODULE_HASH_NONE, name, &w_, NULL); \
													if (!w_) \
													{ \
														RedisModule_ReplyWithError(ctx, "GSL Runing statistics quantile workspace doesn't exist"); \
														return REDISMODULE_ERR; \
													} \
													gsl_rstat_quantile_workspace* w = (gsl_rstat_quantile_workspace*)str_to_ptr(w_);

#define GSL_RSTAT_F(f)								int gsl_rstat_##f##_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) \
													{ \
    													GSL_INIT(2); \
														GET_RSTAT_WORKSPACE; \
    													RedisModule_ReplyWithDouble(ctx, gsl_rstat_##f(w)); \
    													return REDISMODULE_OK; \
													}

#define CREATE_COMMAND(n, f) 						if (RedisModule_CreateCommand(ctx, n, f, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) { \
        												return REDISMODULE_ERR; \
    												}    												

extern "C" {

	int gsl_rstat_alloc_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(2);
		
    	gsl_rstat_workspace* w = gsl_rstat_alloc();    	    	
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], ptr_to_str(ctx, (void*)w), NULL);    	

    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_free_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(2);		

		GET_RSTAT_WORKSPACE;
    	gsl_rstat_free(w);
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], REDISMODULE_HASH_DELETE, NULL);  
    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(2);		

		GET_RSTAT_WORKSPACE;
    	gsl_rstat_reset(w);    	
    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_n_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(2);  	
    
		GET_RSTAT_WORKSPACE;
    		    	    
    	RedisModule_ReplyWithLongLong(ctx, gsl_rstat_n(w));
    	return REDISMODULE_OK;
	}

	int gsl_rstat_add_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(3);   	
		
		GET_RSTAT_WORKSPACE;
		
		double x;
    	RedisModule_StringToDouble(argv[2], &x);
    	gsl_rstat_add(x, w);
    	    	
    	REPLY_WITH_OK_NULL;
	}
	
	GSL_RSTAT_F(min);
	GSL_RSTAT_F(max);
	GSL_RSTAT_F(mean);
	GSL_RSTAT_F(variance);
	GSL_RSTAT_F(sd);
	GSL_RSTAT_F(rms);
	GSL_RSTAT_F(sd_mean);
	GSL_RSTAT_F(median);
	GSL_RSTAT_F(skew);
	GSL_RSTAT_F(kurtosis);

	int gsl_rstat_quantile_alloc_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(3);
		
		double x;
    	RedisModule_StringToDouble(argv[2], &x);
    	
    	gsl_rstat_quantile_workspace* w = gsl_rstat_quantile_alloc(x);

    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, quantile_name(ctx, argv[1], argv[2]), ptr_to_str(ctx, (void*)w), NULL);    	

    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_free_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(3);		

    	GET_RSTAT_QUANTILES_WORKSPACE;

    	gsl_rstat_quantile_free(w);
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, quantile_name(ctx, argv[1], argv[2]), REDISMODULE_HASH_DELETE, NULL);    	
    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(3);		

    	GET_RSTAT_QUANTILES_WORKSPACE;

    	gsl_rstat_quantile_reset(w);    	
    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_add_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(4);		

    	GET_RSTAT_QUANTILES_WORKSPACE;

    	double x;
    	RedisModule_StringToDouble(argv[3], &x);    	

    	gsl_rstat_quantile_add(x, w);    	
    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_get_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(4);		

    	GET_RSTAT_QUANTILES_WORKSPACE;
    	
    	RedisModule_ReplyWithDouble(ctx, gsl_rstat_quantile_get(w));
    	return REDISMODULE_OK;
	}
	
	int RedisModule_OnLoad(RedisModuleCtx* ctx, RedisModuleString __attribute__((unused)) **argv, int  __attribute__((unused)) argc) {		
    	if (RedisModule_Init(ctx,"RedisGSL", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
    		return REDISMODULE_ERR;
    	}
    	   
    	CREATE_COMMAND("gsl_rstat_alloc", gsl_rstat_alloc_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_free", gsl_rstat_free_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_reset", gsl_rstat_reset_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_n", gsl_rstat_n_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_add", gsl_rstat_add_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_min", gsl_rstat_min_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_max", gsl_rstat_max_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_mean", gsl_rstat_mean_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_variance", gsl_rstat_variance_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_sd", gsl_rstat_sd_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_rms", gsl_rstat_rms_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_sd_mean", gsl_rstat_sd_mean_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_median", gsl_rstat_median_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_skew", gsl_rstat_skew_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_kurtosis", gsl_rstat_kurtosis_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_quantile_alloc", gsl_rstat_quantile_alloc_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_quantile_free", gsl_rstat_quantile_free_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_quantile_reset", gsl_rstat_quantile_reset_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_quantile_add", gsl_rstat_quantile_add_RedisCommand);
    	CREATE_COMMAND("gsl_rstat_quantile_get", gsl_rstat_quantile_get_RedisCommand);	    	     
    	    	        	   
    	return REDISMODULE_OK;
	}
}