#ifndef REDISMODULE_HELPERS_H
#define REDISMODULE_HELPERS_H

#include <stdlib.h>
#include "redismodule.h"

#define REPLY_WITH_OK_NULL 									RedisModule_ReplyWithNull(ctx); \
    														return REDISMODULE_OK;

inline RedisModuleString* ptr_to_str(RedisModuleCtx *ctx, const void* p_) {	
	return RedisModule_CreateStringPrintf(ctx, "%" PRIu64, (unsigned long long)p_);	
}

inline void* str_to_ptr(const RedisModuleString* str_) {	
	return (void*)atoll(RedisModule_StringPtrLen(str_, NULL));
}

inline double get_double(const RedisModuleString* str) {
	double x;
    RedisModule_StringToDouble(str, &x);
    return x;
}

inline double get_long_long(const RedisModuleString* str) {
	long long int x;
    RedisModule_StringToLongLong(str, &x);
    return x;
}

#endif