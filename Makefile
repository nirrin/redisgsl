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

clear:
	clear

clean:
	rm -rf *.o *.so *.rdb

stop:
	kill -s SIGTERM `pidof redis-server`

run: clear	
	rm -f *.rdb	
	redis-server /etc/redis.conf --loadmodule ./libredisgsl.so

test-rstat:
	redis-cli gsl_rstat_alloc A
	for x in 1 2 3 4 5 6 ; do \
		redis-cli gsl_rstat_add A $$x ; \
	done
	redis-cli gsl_rstat_n A
	redis-cli gsl_rstat_min A
	redis-cli gsl_rstat_max A
	redis-cli gsl_rstat_mean A
	redis-cli gsl_rstat_variance A
	redis-cli gsl_rstat_sd A
	redis-cli gsl_rstat_rms A
	redis-cli gsl_rstat_sd_mean A
	redis-cli gsl_rstat_median A
	redis-cli gsl_rstat_skew A
	redis-cli gsl_rstat_kurtosis A
	redis-cli gsl_rstat_reset A
	redis-cli gsl_rstat_n A
	redis-cli gsl_rstat_free A
	redis-cli HGETALL GSL

QUANTILES := 0.05 0.5 0.95

test-rstat-quantile:
	for q in $(QUANTILES) ; do \
		redis-cli gsl_rstat_quantile_alloc A_$$q q ; \
	done	
	for q in $(QUANTILES) ; do \
		for x in 1 2 3 4 5 6 ; do \
			redis-cli gsl_rstat_quantile_add A_$$q $$x ; \
		done \
	done 
	for q in $(QUANTILES) ; do \
		redis-cli gsl_rstat_quantile_get A_$$q ; \
		redis-cli gsl_rstat_quantile_reset A_$$q ; \
		redis-cli gsl_rstat_quantile_get A_$$q ; \
		redis-cli gsl_rstat_quantile_free A_$$q ; \
	done		
	redis-cli HGETALL GSL

HISTOGRAMS := A B

test-rstat-histogram:
	redis-cli gsl_histogram_alloc A 3
	redis-cli gsl_histogram_alloc B 3
	redis-cli gsl_histogram_set_ranges A 1 3 5 7
	redis-cli gsl_histogram_set_ranges_uniform B 1 7
	for h in $(HISTOGRAMS) ; do \
		redis-cli gsl_histogram_increment $$h 2 ; \
		redis-cli gsl_histogram_increment $$h 2 ; \
		redis-cli gsl_histogram_increment $$h 2 ; \
		redis-cli gsl_histogram_accumulate $$h 4 2 ; \
		redis-cli gsl_histogram_increment $$h 6 ; \
		redis-cli gsl_histogram_get $$h 0 ; \
		redis-cli gsl_histogram_get $$h 1 ; \
		redis-cli gsl_histogram_get $$h 2 ; \
		redis-cli gsl_histogram_get_range $$h 0 ; \
		redis-cli gsl_histogram_get_range $$h 1 ; \
		redis-cli gsl_histogram_get_range $$h 2 ; \
		redis-cli gsl_histogram_max $$h ; \
		redis-cli gsl_histogram_min $$h ; \
		redis-cli gsl_histogram_bins $$h ; \
		redis-cli gsl_histogram_max_val $$h ; \
		redis-cli gsl_histogram_max_bin $$h ; \
		redis-cli gsl_histogram_min_val $$h ; \
		redis-cli gsl_histogram_min_bin $$h ; \
		redis-cli gsl_histogram_mean $$h ; \
		redis-cli gsl_histogram_sigma $$h ; \
		redis-cli gsl_histogram_find $$h 0 ; \
		redis-cli gsl_histogram_find $$h 1 ; \
		redis-cli gsl_histogram_find $$h 2 ; \
		redis-cli gsl_histogram_find $$h 3 ; \
		redis-cli gsl_histogram_find $$h 4 ; \
		redis-cli gsl_histogram_find $$h 5 ; \
		redis-cli gsl_histogram_find $$h 6 ; \
		redis-cli gsl_histogram_find $$h 7 ; \
		redis-cli gsl_histogram_find $$h 8 ; \
		redis-cli gsl_histogram_sum $$h ; \
	done
	redis-cli gsl_histogram_alloc C 3
	redis-cli gsl_histogram_alloc D 3
	redis-cli gsl_histogram_memcpy A C
	redis-cli gsl_histogram_memcpy B D
	redis-cli gsl_histogram_clone A E
	redis-cli gsl_histogram_clone B F	
	redis-cli gsl_histogram_reset A
	redis-cli gsl_histogram_reset B
	for h in $(HISTOGRAMS) ; do \
		redis-cli gsl_histogram_get $$h 0 ; \
		redis-cli gsl_histogram_get $$h 1 ; \
		redis-cli gsl_histogram_get $$h 2 ; \
	done
	redis-cli gsl_histogram_free A
	redis-cli gsl_histogram_free B
	for h in C D E F; do \
		redis-cli gsl_histogram_get $$h 0 ; \
		redis-cli gsl_histogram_get $$h 1 ; \
		redis-cli gsl_histogram_get $$h 2 ; \
		redis-cli gsl_histogram_get_range $$h 0 ; \
		redis-cli gsl_histogram_get_range $$h 1 ; \
		redis-cli gsl_histogram_get_range $$h 2 ; \
		redis-cli gsl_histogram_max $$h ; \
		redis-cli gsl_histogram_min $$h ; \
		redis-cli gsl_histogram_bins $$h ; \
		redis-cli gsl_histogram_max_val $$h ; \
		redis-cli gsl_histogram_max_bin $$h ; \
		redis-cli gsl_histogram_min_val $$h ; \
		redis-cli gsl_histogram_min_bin $$h ; \
		redis-cli gsl_histogram_mean $$h ; \
		redis-cli gsl_histogram_sigma $$h ; \
		redis-cli gsl_histogram_find $$h 0 ; \
		redis-cli gsl_histogram_find $$h 1 ; \
		redis-cli gsl_histogram_find $$h 2 ; \
		redis-cli gsl_histogram_find $$h 3 ; \
		redis-cli gsl_histogram_find $$h 4 ; \
		redis-cli gsl_histogram_find $$h 5 ; \
		redis-cli gsl_histogram_find $$h 6 ; \
		redis-cli gsl_histogram_find $$h 7 ; \
		redis-cli gsl_histogram_find $$h 8 ; \
		redis-cli gsl_histogram_sum $$h ; \
		redis-cli gsl_histogram_free $$h ; \
	done
	redis-cli HGETALL GSL

test: clear test-rstat test-rstat-quantile test-rstat-histogram	
