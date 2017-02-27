GSL_LOCATION := /tmp
GSL_VERSION := gsl-2.3
GSL := $(GSL_LOCATION)/$(GSL_VERSION)
GSL_A = $(shell find $(GSL) -name "*.o" | grep libs)
SHOBJ_CFLAGS += -W -Wall -fno-common -g -ggdb -O2 -fPIC
SHOBJ_LDFLAGS += -shared -fPIC

.SUFFIXES: .cc .so .o

all: libredisgsl.so

preprocess:
	rm -Rf /tmp/gsl* && cd /tmp && wget http://ftp.cc.uoc.gr/mirrors/gnu/gsl/$(GSL_VERSION).tar.gz && tar xvf $(GSL_VERSION).tar.gz
	python preprocess.py $(GSL)
	cp redismodule.h $(GSL)
	cd $(GSL) && ./autogen.sh && ./configure && make

.cc.o:
	g++ -I. -I$(GSL) $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

libredisgsl.so: redisgsl.o
	g++ -o $@ $< $(GSL_A) $(SHOBJ_LDFLAGS) -lc

clean:
	rm -rf *.o *.so *.rdb

stop:
	kill -s SIGTERM `pidof redis-server`

run:	
	rm -f *.rdb
	redis-server /etc/redis.conf --loadmodule ./libredisgsl.so

test:	
	redis-cli FLUSHDB
	redis-cli gsl_rstat_alloc A
	redis-cli gsl_rstat_add A 1
	redis-cli gsl_rstat_add A 2
	redis-cli gsl_rstat_add A 3
	redis-cli gsl_rstat_add A 4
	redis-cli gsl_rstat_add A 5
	redis-cli gsl_rstat_add A 6
	redis-cli gsl_rstat_n A
	redis-cli gsl_rstat_min A
	redis-cli gsl_rstat_max A
	redis-cli gsl_rstat_mean A
	redis-cli stat_variance A
	redis-cli gsl_rstat_sd A
	redis-cli gsl_rstat_rms A
	redis-cli gsl_rstat_sd_mean A
	redis-cli gsl_rstat_median A
	redis-cli gsl_rstat_skew A
	redis-cli gsl_rstat_kurtosis A
	redis-cli gsl_rstat_reset A
	redis-cli gsl_rstat_n A
	redis-cli gsl_rstat_free A
	redis-cli gsl_rstat_quantile_alloc A 0.05
	redis-cli gsl_rstat_quantile_alloc A 0.5
	redis-cli gsl_rstat_quantile_alloc A 0.95
	redis-cli gsl_rstat_quantile_add A 0.05 1
	redis-cli gsl_rstat_quantile_add A 0.5 1
	redis-cli gsl_rstat_quantile_add A 0.95 1
	redis-cli gsl_rstat_quantile_add A 0.05 2
	redis-cli gsl_rstat_quantile_add A 0.5 2
	redis-cli gsl_rstat_quantile_add A 0.95 2
	redis-cli gsl_rstat_quantile_add A 0.05 3
	redis-cli gsl_rstat_quantile_add A 0.5 3
	redis-cli gsl_rstat_quantile_add A 0.95 3
	redis-cli gsl_rstat_quantile_add A 0.05 4
	redis-cli gsl_rstat_quantile_add A 0.5 4
	redis-cli gsl_rstat_quantile_add A 0.95 4
	redis-cli gsl_rstat_quantile_add A 0.05 5
	redis-cli gsl_rstat_quantile_add A 0.5 5
	redis-cli gsl_rstat_quantile_add A 0.95 5
	redis-cli gsl_rstat_quantile_add A 0.05 6
	redis-cli gsl_rstat_quantile_add A 0.5 6
	redis-cli gsl_rstat_quantile_add A 0.95 6	
	redis-cli gsl_rstat_quantile_get A 0.05
	redis-cli gsl_rstat_quantile_get A 0.5
	redis-cli gsl_rstat_quantile_get A 0.95
	redis-cli gsl_rstat_quantile_reset A 0.05
	redis-cli gsl_rstat_quantile_reset A 0.5
	redis-cli gsl_rstat_quantile_reset A 0.95
	redis-cli gsl_rstat_quantile_free A 0.05
	redis-cli gsl_rstat_quantile_free A 0.5
	redis-cli gsl_rstat_quantile_free A 0.95
