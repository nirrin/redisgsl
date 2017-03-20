#ifndef REDISGSL_HELPERS_H
#define REDISGSL_HELPERS_H

#include <stdlib.h>
#include "redismodule.h"

inline RedisModuleString* ptr_to_str(RedisModuleCtx *ctx, const void* p_) {	
	return RedisModule_CreateStringPrintf(ctx, "%" PRIu64, (unsigned long long)p_);	
}

inline void* str_to_ptr(const RedisModuleString* str_) {	
	return (void*)atoll(RedisModule_StringPtrLen(str_, NULL));
}

#endif