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
#include <gsl/gsl_histogram.h>
#include <err/gsl_errno.h>

inline RedisModuleString* ptr_to_str(RedisModuleCtx *ctx, const void* p_) {	
	return RedisModule_CreateStringPrintf(ctx, "%" PRIu64, (unsigned long long)p_);	
}

inline void* str_to_ptr(const RedisModuleString* str_) {	
	return (void*)atoll(RedisModule_StringPtrLen(str_, NULL));
}

RedisModuleString* GSL_NAME = NULL;
RedisModuleKey* GSL_KEY = NULL;

inline void get_gsl_key(RedisModuleCtx *ctx) {
	if ((!GSL_NAME) || (!GSL_KEY)) {
		GSL_NAME = RedisModule_CreateString(ctx, "GSL", 3);
   		GSL_KEY = (RedisModuleKey*)RedisModule_OpenKey(ctx, GSL_NAME, REDISMODULE_READ | REDISMODULE_WRITE);
   		RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, RedisModule_CreateStringPrintf(ctx, "GSL_VERSION"), RedisModule_CreateStringPrintf(ctx, GSL_VERSION), NULL);  		
	}
}

template<typename T>
inline T* get_gsl_object(const RedisModuleString* name) {
	RedisModuleString* o_ = NULL;
	RedisModule_HashGet(GSL_KEY, REDISMODULE_HASH_NONE, name, &o_, NULL);
	return (o_) ? (T*)str_to_ptr(o_) : NULL;
}

#define REPLY_WITH_OK_NULL 									RedisModule_ReplyWithNull(ctx); \
    														return REDISMODULE_OK;

#define GSL_INIT(n)											if (argc < n) { \
    															return RedisModule_WrongArity(ctx); \
    														} \
    														get_gsl_key(ctx); \
    														RedisModule_AutoMemory(ctx);				

#define GSL_RSTAT_WORKSPACE_INIT(ctx, n, name)            	GSL_INIT(n); \
															gsl_rstat_workspace* w = get_gsl_object<gsl_rstat_workspace>(name); \
															if (!w) { \
    															RedisModule_ReplyWithError(ctx, "GSL Running Statistics Workspace doesn't exist"); \
    															return REDISMODULE_ERR; \
    														}

#define GSL_RSTAT_F(f)										int gsl_rstat_##f##_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) \
															{ \
    															GSL_RSTAT_WORKSPACE_INIT(ctx, 2, argv[1]); \
    															RedisModule_ReplyWithDouble(ctx, gsl_rstat_##f(w)); \
    															return REDISMODULE_OK; \
															}

#define GSL_RSTAT_QUANTILE_WORKSPACE_INIT(ctx, n, name)		GSL_INIT(n); \
															gsl_rstat_quantile_workspace* w = get_gsl_object<gsl_rstat_quantile_workspace>(name); \
															if (!w) { \
    															RedisModule_ReplyWithError(ctx, "GSL Running Statistics Quantiles Workspace doesn't exist"); \
    															return REDISMODULE_ERR; \
    														}
															

#define GSL_HISTOGRAM_DOESNT_EXIST(ctx, h)					if (!h) { \
    															RedisModule_ReplyWithError(ctx, "GSL Histogram doesn't exist"); \
																return REDISMODULE_ERR; \
    														}

#define GSL_HISTOGRAM_INIT(ctx, n, name)					GSL_INIT(n); \
															gsl_histogram* h = get_gsl_object<gsl_histogram>(name); \
															GSL_HISTOGRAM_DOESNT_EXIST(ctx, h);															  																													

#define GSL_HISTOGRAM_F_DOUBLE(f) 							int gsl_histogram_##f##_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) \
															{ \
																GSL_HISTOGRAM_INIT(ctx, 2, argv[1]); \
																RedisModule_ReplyWithDouble(ctx, gsl_histogram_##f(h)); \
    															return REDISMODULE_OK; \
															}

#define GSL_HISTOGRAM_F_LONG_LONG(f)						int gsl_histogram_##f##_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) \
															{ \
																GSL_HISTOGRAM_INIT(ctx, 2, argv[1]); \
																RedisModule_ReplyWithLongLong(ctx, (long long int)gsl_histogram_##f(h)); \
    															return REDISMODULE_OK; \
															}

#define GSL_HISTOGRAM_OPERATION(o)							int gsl_histogram_##o##_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) \
															{ \
																GSL_HISTOGRAM_INIT(ctx, 3, argv[1]); \
																gsl_histogram* h2 = get_gsl_object<gsl_histogram>(argv[2]); \
    															GSL_HISTOGRAM_DOESNT_EXIST(ctx, h2); \
    															gsl_histogram_##o(h, h2); \
    															REPLY_WITH_OK_NULL; \
															}

#define GSL_HISOGRAM_PDF_INIT(ctx, n, name)     			GSL_INIT(n); \
															gsl_histogram_pdf* p = get_gsl_object<gsl_histogram_pdf>(name); \
    														if (!p) { \
    															RedisModule_ReplyWithError(ctx, "GSL Histogram PDF doesn't exist"); \
    															return REDISMODULE_ERR; \
    														}

#define CREATE_COMMAND(n, f) 								if (RedisModule_CreateCommand(ctx, n, f, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) { \
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
    	GSL_RSTAT_WORKSPACE_INIT(ctx, 2, argv[1]);
    	gsl_rstat_free(w);
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], REDISMODULE_HASH_DELETE, NULL);      	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_WORKSPACE_INIT(ctx, 2, argv[1]);
    	gsl_rstat_reset(w);    	    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_n_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_WORKSPACE_INIT(ctx, 2, argv[1]);
    	RedisModule_ReplyWithLongLong(ctx, gsl_rstat_n(w));
    	return REDISMODULE_OK;
	}

	int gsl_rstat_add_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_WORKSPACE_INIT(ctx, 3, argv[1]);		
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
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], ptr_to_str(ctx, (void*)w), NULL);    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_free_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_QUANTILE_WORKSPACE_INIT(ctx, 2, argv[1]);
    	gsl_rstat_quantile_free(w);
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], REDISMODULE_HASH_DELETE, NULL);    	    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_QUANTILE_WORKSPACE_INIT(ctx, 2, argv[1]);
    	gsl_rstat_quantile_reset(w);    	    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_add_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_QUANTILE_WORKSPACE_INIT(ctx, 3, argv[1]);
    	double x;
    	RedisModule_StringToDouble(argv[2], &x);    	
    	gsl_rstat_quantile_add(x, w);    	    	    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_rstat_quantile_get_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_RSTAT_QUANTILE_WORKSPACE_INIT(ctx, 2, argv[1]);    	
    	RedisModule_ReplyWithDouble(ctx, gsl_rstat_quantile_get(w));
    	return REDISMODULE_OK;
	}

	int gsl_histogram_alloc_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_INIT(3);
		long long int n;
    	RedisModule_StringToLongLong(argv[2], &n);     	
    	gsl_histogram* h = gsl_histogram_alloc(n);    	
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], ptr_to_str(ctx, (void*)h), NULL);    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_set_ranges_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
    	const size_t n = argc - 2;
    	double range[n];
    	for (int i = 2; i < argc; i++) {
    		double r;
    		RedisModule_StringToDouble(argv[i], &r);    		
    		range[i - 2] = r;
    	}
    	gsl_histogram_set_ranges(h, range, n);
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_set_ranges_uniform_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 4, argv[1]);		
		double xmin;
		double xmax;
    	RedisModule_StringToDouble(argv[2], &xmin); 
    	RedisModule_StringToDouble(argv[3], &xmax); 
    	gsl_histogram_set_ranges_uniform(h, xmin, xmax);    	
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_free_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 2, argv[1]);	
		gsl_histogram_free(h);
		RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], REDISMODULE_HASH_DELETE, NULL);
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_memcpy_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{		
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		gsl_histogram* dest = get_gsl_object<gsl_histogram>(argv[2]);    	
    	GSL_HISTOGRAM_DOESNT_EXIST(ctx, dest);
		gsl_histogram_memcpy(dest, h);
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_clone_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);		
		gsl_histogram* cloned = gsl_histogram_clone(h);		
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[2], ptr_to_str(ctx, (void*)cloned), NULL);    
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_increment_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		double x;
    	RedisModule_StringToDouble(argv[2], &x); 
    	gsl_histogram_increment(h, x);
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_accumulate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 4, argv[1]);
		double x;
		double weight;
    	RedisModule_StringToDouble(argv[2], &x);
    	RedisModule_StringToDouble(argv[3], &weight);
    	gsl_histogram_accumulate(h, x, weight);
		REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_get_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		long long int i;		
    	RedisModule_StringToLongLong(argv[2], &i);
		RedisModule_ReplyWithDouble(ctx, gsl_histogram_get(h, (size_t)i));
    	return REDISMODULE_OK;
	}

	int gsl_histogram_get_range_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		long long int i;		
    	RedisModule_StringToLongLong(argv[2], &i);
    	double lower;
    	double upper;
    	if (gsl_histogram_get_range(h, (size_t)i, &lower, &upper) != 0) {
    		RedisModule_ReplyWithError(ctx, "GSL Histogram: index outside the valid range of indices");
			return REDISMODULE_ERR;
    	}
    	RedisModule_ReplyWithArray(ctx, 2);
		RedisModule_ReplyWithDouble(ctx, lower);
		RedisModule_ReplyWithDouble(ctx, upper);
    	return REDISMODULE_OK;
	}

	GSL_HISTOGRAM_F_DOUBLE(max);
	GSL_HISTOGRAM_F_DOUBLE(min);
	GSL_HISTOGRAM_F_LONG_LONG(bins);

	int gsl_histogram_reset_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 2, argv[1]);
		gsl_histogram_reset(h);
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_find_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		double x;		
    	RedisModule_StringToDouble(argv[2], &x);    	
    	size_t i;
    	if (gsl_histogram_find(h, x, &i) != 0) {
    		RedisModule_ReplyWithError(ctx, "GSL Histogram: coordinate outside the valid range of histogram");
			return REDISMODULE_ERR;
    	}      	
    	RedisModule_ReplyWithLongLong(ctx, (long long int)i);
    	return REDISMODULE_OK;
	}

	GSL_HISTOGRAM_F_DOUBLE(max_val);
	GSL_HISTOGRAM_F_LONG_LONG(max_bin);
	GSL_HISTOGRAM_F_DOUBLE(min_val);
	GSL_HISTOGRAM_F_LONG_LONG(min_bin);
	GSL_HISTOGRAM_F_DOUBLE(mean);
	GSL_HISTOGRAM_F_DOUBLE(sigma);
	GSL_HISTOGRAM_F_DOUBLE(sum);

	int gsl_histogram_equal_bins_p_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		gsl_histogram* h2 = get_gsl_object<gsl_histogram>(argv[2]);    	
    	GSL_HISTOGRAM_DOESNT_EXIST(ctx, h2);
    	RedisModule_ReplyWithLongLong(ctx, (long long int) gsl_histogram_equal_bins_p(h, h2));
    	return REDISMODULE_OK; 
	}

	GSL_HISTOGRAM_OPERATION(add);
	GSL_HISTOGRAM_OPERATION(sub);
	GSL_HISTOGRAM_OPERATION(mul);
	GSL_HISTOGRAM_OPERATION(div);

	int gsl_histogram_scale_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		double scale;		
    	RedisModule_StringToDouble(argv[2], &scale);
    	gsl_histogram_scale(h, scale);
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_shift_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
		GSL_HISTOGRAM_INIT(ctx, 3, argv[1]);
		double offset;		
    	RedisModule_StringToDouble(argv[2], &offset);
    	gsl_histogram_shift(h, offset);
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_pdf_alloc_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_INIT(3);
    	long long int n;
    	RedisModule_StringToLongLong(argv[2], &n);		
    	gsl_histogram_pdf* p = gsl_histogram_pdf_alloc((size_t)n);    	    	
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], ptr_to_str(ctx, (void*)p), NULL);    	
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_pdf_init_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{    
    	GSL_HISOGRAM_PDF_INIT(ctx, 3, argv[1])
    	gsl_histogram* h = get_gsl_object< gsl_histogram>(argv[2]); 
    	GSL_HISTOGRAM_DOESNT_EXIST(ctx, h);
    	gsl_histogram_pdf_init(p, h);
    	REPLY_WITH_OK_NULL;
	}

	int gsl_histogram_pdf_free_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_HISOGRAM_PDF_INIT(ctx, 2, argv[1]) 	
    	gsl_histogram_pdf_free(p);
    	RedisModule_HashSet(GSL_KEY, REDISMODULE_HASH_NONE, argv[1], REDISMODULE_HASH_DELETE, NULL);
    	REPLY_WITH_OK_NULL;
	}

	int  gsl_histogram_pdf_sample_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
	{
    	GSL_HISOGRAM_PDF_INIT(ctx, 3, argv[1])
    	double r;
    	RedisModule_StringToDouble(argv[2], &r);	   	
    	RedisModule_ReplyWithDouble(ctx, gsl_histogram_pdf_sample(p, r));
    	return REDISMODULE_OK; 
	}
	
	int RedisModule_OnLoad(RedisModuleCtx* ctx, RedisModuleString __attribute__((unused)) **argv, int  __attribute__((unused)) argc) {		
    	if (RedisModule_Init(ctx,"RedisGSL", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
    		return REDISMODULE_ERR;
    	}

    	gsl_set_error_handler_off();
    	   
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

    	CREATE_COMMAND("gsl_histogram_alloc", gsl_histogram_alloc_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_set_ranges", gsl_histogram_set_ranges_RedisCommand);	
    	CREATE_COMMAND("gsl_histogram_set_ranges_uniform", gsl_histogram_set_ranges_uniform_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_free", gsl_histogram_free_RedisCommand);	 
    	CREATE_COMMAND("gsl_histogram_memcpy", gsl_histogram_memcpy_RedisCommand);	
    	CREATE_COMMAND("gsl_histogram_clone", gsl_histogram_clone_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_increment", gsl_histogram_increment_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_increment", gsl_histogram_increment_RedisCommand);	
    	CREATE_COMMAND("gsl_histogram_accumulate", gsl_histogram_accumulate_RedisCommand);	 
    	CREATE_COMMAND("gsl_histogram_get", gsl_histogram_get_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_get_range", gsl_histogram_get_range_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_max", gsl_histogram_max_RedisCommand);	
    	CREATE_COMMAND("gsl_histogram_min", gsl_histogram_min_RedisCommand);	
    	CREATE_COMMAND("gsl_histogram_bins", gsl_histogram_bins_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_reset", gsl_histogram_bins_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_find", gsl_histogram_find_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_max_val", gsl_histogram_max_val_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_max_bin", gsl_histogram_max_bin_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_min_val", gsl_histogram_min_val_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_min_bin", gsl_histogram_min_bin_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_mean", gsl_histogram_mean_RedisCommand);    	     
    	CREATE_COMMAND("gsl_histogram_sigma", gsl_histogram_sigma_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_sum", gsl_histogram_sum_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_equal_bins_p", gsl_histogram_equal_bins_p_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_add", gsl_histogram_add_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_sub", gsl_histogram_sub_RedisCommand);
    	CREATE_COMMAND("gsl_histogram_mul", gsl_histogram_mul_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_div", gsl_histogram_div_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_scale", gsl_histogram_scale_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_shift", gsl_histogram_shift_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_pdf_alloc", gsl_histogram_pdf_alloc_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_pdf_init", gsl_histogram_pdf_init_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_pdf_free", gsl_histogram_pdf_free_RedisCommand); 
    	CREATE_COMMAND("gsl_histogram_pdf_sample", gsl_histogram_pdf_sample_RedisCommand); 
    	    	        	   
    	return REDISMODULE_OK;
	}
}