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

flushdb:
	redis-cli FLUSHDB

test-rstat:
	redis-cli gsl_rstat_alloc A > /dev/null
	for x in 1 2 3 4 5 6 ; do \
		redis-cli gsl_rstat_add A $$x > /dev/null ; \
	done    
	[ `redis-cli gsl_rstat_n A`  == '6' ] || exit 1
	[ `redis-cli gsl_rstat_min A` == '1' ] || exit 1
	[ `redis-cli gsl_rstat_max A` == '6' ] || exit 1
	[ `redis-cli gsl_rstat_mean A` == '3.5' ] || exit 1
	[ `redis-cli gsl_rstat_variance A` == '3.5' ] || exit 1
	[ `redis-cli gsl_rstat_sd A` == '1.8708286933869707' ] || exit 1
	[ `redis-cli gsl_rstat_rms A` == '3.8944404818493079' ] || exit 1
	[ `redis-cli gsl_rstat_sd_mean A` == '0.76376261582597338' ] || exit 1
	[ `redis-cli gsl_rstat_median A` == '3' ] || exit 1
	[ `redis-cli gsl_rstat_skew A` == '0' ] || exit 1
	[ `redis-cli gsl_rstat_kurtosis A` == '-1.7976190476190474' ] || exit 1
	redis-cli gsl_rstat_reset A > /dev/null
	[ `redis-cli gsl_rstat_n A` == '0' ] || exit 1
	redis-cli gsl_rstat_free A > /dev/null
	redis-cli HGETALL GSL

QUANTILES := 0.05 0.5 0.95

test-rstat-quantile:
	for q in $(QUANTILES) ; do \
		redis-cli gsl_rstat_quantile_alloc A_$$q $$q > /dev/null ; \
	done    
	for q in $(QUANTILES) ; do \
		for x in `seq 1 100` ; do \
			redis-cli gsl_rstat_quantile_add A_$$q $$x > /dev/null ; \
		done \
	done    
	[ `redis-cli gsl_rstat_quantile_get A_0.05` == '5' ] || exit 1
	[ `redis-cli gsl_rstat_quantile_get A_0.5` == '50' ] || exit 1
	[ `redis-cli gsl_rstat_quantile_get A_0.95` == '95' ] || exit 1
	for q in $(QUANTILES) ; do \
		redis-cli gsl_rstat_quantile_reset A_$$q > /dev/null ; \
		[ `redis-cli gsl_rstat_quantile_get A_$$q` == '0' ] || exit 1 ; \
		redis-cli gsl_rstat_quantile_free A_$$q > /dev/null ; \
	done
	redis-cli HGETALL GSL

HISTOGRAMS := A B

test-histogram:
	redis-cli gsl_histogram_alloc A 10 > /dev/null
	redis-cli gsl_histogram_alloc B 10 > /dev/null
	redis-cli gsl_histogram_set_ranges A 0 10 20 30 40 50 60 70 80 90 100 > /dev/null
	redis-cli gsl_histogram_set_ranges_uniform B 0 100 > /dev/null
	for h in $(HISTOGRAMS) ; do \
		for x in `seq 1 100` ; do \
			redis-cli gsl_histogram_increment $$h $$x > /dev/null ; \
		done ; \
		for x in `seq 1 50` ; do \
			redis-cli gsl_histogram_accumulate $$h $$x 5 > /dev/null ; \
		done ; \
		[ `redis-cli gsl_histogram_max $$h` == '100' ] || exit 1; \
		[ `redis-cli gsl_histogram_min $$h` == '0' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_bins $$h` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_max_val $$h` == '60' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_max_bin $$h` == '1' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_min_val $$h` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_min_bin $$h` == '6' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 0` == '54' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 1` == '60' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 2` == '60' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 3` == '60' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 4` == '60' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 5` == '15' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 6` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 7` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 8` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 9` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get $$h 10` == '0' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get_range $$h 0 | head -n1` == '0' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_get_range $$h 0 | tail -n1` == '10' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_mean $$h` == '32.936962750716333' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_sigma $$h` == '22.40002386795928' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_sum $$h` == '349' ] || exit 1 ; \
		for x in `seq 1 99` ; do \
			[ `redis-cli gsl_histogram_find $$h $$x` == `expr $$x / 10` ] || exit 1 ; \
		done ; \
		redis-cli gsl_histogram_pdf_alloc P 10 > /dev/null ; \
		redis-cli gsl_histogram_pdf_init P $$h > /dev/null ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.05` ==  '3.2314814814814818' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.10` ==  '6.4629629629629637' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.25` ==  '15.541666666666666' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.5` ==  '30.083333333333332' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.75` ==  '44.625' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.90` ==  '65.099999999999994' ] || exit 1 ; \
		[ `redis-cli gsl_histogram_pdf_sample P 0.95` ==  '82.54999999999994' ] || exit 1 ; \
		redis-cli gsl_histogram_pdf_free P ; \
	done
	redis-cli gsl_histogram_alloc C 10 > /dev/null
	redis-cli gsl_histogram_memcpy A C > /dev/null
	redis-cli gsl_histogram_clone A D > /dev/null
	redis-cli gsl_histogram_clone A E > /dev/null
	redis-cli gsl_histogram_clone A F > /dev/null
	redis-cli gsl_histogram_clone A G > /dev/null
	redis-cli gsl_histogram_clone A H > /dev/null
	[ `redis-cli gsl_histogram_equal_bins_p A C` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_equal_bins_p A D` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_equal_bins_p A E` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_equal_bins_p A F` == '1' ] || exit 1
	redis-cli gsl_histogram_add C A > /dev/null
	redis-cli gsl_histogram_sub D A > /dev/null
	redis-cli gsl_histogram_mul E A > /dev/null
	redis-cli gsl_histogram_div F A > /dev/null
	redis-cli gsl_histogram_scale G 10 > /dev/null
	redis-cli gsl_histogram_shift H 100 > /dev/null
	[ `redis-cli gsl_histogram_max C` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min C` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins C` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val C` == '120' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin C` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_min_val C` == '20' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin C` == '6' ] || exit 1
	[ `redis-cli gsl_histogram_mean C` == '32.936962750716333' ] || exit 1
	[ `redis-cli gsl_histogram_sigma C` == '22.40002386795928' ] || exit 1
	[ `redis-cli gsl_histogram_sum C` == '698' ] || exit 1
	[ `redis-cli gsl_histogram_max D` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins D` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_min_val D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_mean D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_sigma D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_sum D` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_max E` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min E` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins E` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val E` == '3600' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin E` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_min_val E` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin E` == '6' ] || exit 1
	[ `redis-cli gsl_histogram_mean E` == '27.36497408171228' ] || exit 1
	[ `redis-cli gsl_histogram_sigma E` == '16.165760673900206' ] || exit 1
	[ `redis-cli gsl_histogram_sum E` == '17941' ] || exit 1
	[ `redis-cli gsl_histogram_max F` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min F` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins F` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val F` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin F` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_min_val F` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin F` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_mean F` == '50' ] || exit 1
	[ `redis-cli gsl_histogram_sigma F` == '28.722813232690143' ] || exit 1
	[ `redis-cli gsl_histogram_sum F` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max G` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min G` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins G` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val G` == '600' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin G` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_min_val G` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin G` == '6' ] || exit 1
	[ `redis-cli gsl_histogram_mean G` == '32.936962750716333' ] || exit 1
	[ `redis-cli gsl_histogram_sigma G` == '22.40002386795928' ] || exit 1
	[ `redis-cli gsl_histogram_sum G` == '3490' ] || exit 1	
	[ `redis-cli gsl_histogram_max H` == '100' ] || exit 1
	[ `redis-cli gsl_histogram_min H` == '0' ] || exit 1
	[ `redis-cli gsl_histogram_bins H` == '10' ] || exit 1
	[ `redis-cli gsl_histogram_max_val H` == '160' ] || exit 1
	[ `redis-cli gsl_histogram_max_bin H` == '1' ] || exit 1
	[ `redis-cli gsl_histogram_min_val H` == '110' ] || exit 1
	[ `redis-cli gsl_histogram_min_bin H` == '6' ] || exit 1
	[ `redis-cli gsl_histogram_mean H` == '45.585618977020012' ] || exit 1
	[ `redis-cli gsl_histogram_sigma H` == '28.234921532164169' ] || exit 1
	[ `redis-cli gsl_histogram_sum H` == '1349' ] || exit 1		
	for h in A B C D E F G H; do \
		redis-cli gsl_histogram_reset $$h > /dev/null ; \
		[ `redis-cli gsl_histogram_bins $$h` == '10' ] || exit 1; \
		[ `redis-cli gsl_histogram_sum $$h` == '0' ] || exit 1 ; \
		redis-cli gsl_histogram_free $$h > /dev/null ; \
	done

test: clear test-rstat test-rstat-quantile test-histogram 
