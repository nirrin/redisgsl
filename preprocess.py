#!/usr/bin/env python

import sys
import glob
import re

memory_allocation_or_deallocation_re = re.compile(r"( free\()|( malloc\()|( calloc\()|( realloc\()|( strdup\()")
include_re = re.compile(r"^\#include")

def find_c_files(path):
    return glob.glob(path + "/**/*.c")

def contain_memory_allocation_or_deallocation_function(file):
    with open(file, 'r') as f:
        content = f.read()
        return memory_allocation_or_deallocation_re.match(content) != None

def replace(line):
    line = line.replace(" free(", " RedisModule_Free(")
    line = line.replace(" malloc(", " RedisModule_Alloc(")
    line = line.replace(" calloc(" ," RedisModule_Calloc(")
    line = line.replace(" realloc(", " RedisModule_Realloc(")
    line = line.replace(" strdup(", " RedisModule_Strdup(")
    return line

def preprocess(path):
    files = [f for f in find_c_files(sys.argv[1]) if contain_memory_allocation_or_deallocation_function]
    for file in files:
        lines = None
        with open(file, "r") as f:
            lines = f.readlines()
            i = 0
            included = False
            for line in lines:                
                if not included and include_re.match(line) != None:
                    lines.insert(i + 1, "#include \"redismodule.h\"\n")
                    included = True
                lines[i] = replace(lines[i])
                i = i + 1           
        if lines != None:
            with open(file, "w") as f:
                for line in lines:
                    f.write(line)

if __name__ == "__main__":
    preprocess(sys.argv[1])
    