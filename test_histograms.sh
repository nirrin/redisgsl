#!/usr/bin/env bash

HISTOGRAMS='A B'
redis-cli gsl_histogram_alloc A 10
redis-cli gsl_histogram_alloc B 10
redis-cli gsl_histogram_set_ranges A 0 10 20 30 40 50 60 70 80 90 100
redis-cli gsl_histogram_set_ranges_uniform B 0 100
for h in $HISTOGRAMS ; do \
	for x in `seq 1 100` ; do \
		redis-cli gsl_histogram_increment $$h $$x > /dev/null ; \
	done ; \
	for x in `seq 1 50` ; do \
		redis-cli gsl_histogram_accumulate $$h $$x 5 > /dev/null ; \
	done ; \
	[ `redis-cli gsl_histogram_max $$h` == '100' ] ; \
	[ `redis-cli gsl_histogram_min $$h` == '0' ] ; \
	[ `redis-cli gsl_histogram_bins $$h` == '10' ] ; \
	[ `redis-cli gsl_histogram_max_val $$h` == '60' ] ; \
	[ `redis-cli gsl_histogram_max_bin $$h` == '1' ] ; \
	[ `redis-cli gsl_histogram_min_val $$h` == '10' ] ; \
	[ `redis-cli gsl_histogram_min_bin $$h` == '6' ] ; \
	[ `redis-cli gsl_histogram_get $$h 0` == '54' ] ; \
	[ `redis-cli gsl_histogram_get $$h 1` == '60' ] ; \
	[ `redis-cli gsl_histogram_get $$h 2` == '60' ] ; \
	[ `redis-cli gsl_histogram_get $$h 3` == '60' ] ; \
	[ `redis-cli gsl_histogram_get $$h 4` == '60' ] ; \
	[ `redis-cli gsl_histogram_get $$h 5` == '15' ] ; \
	[ `redis-cli gsl_histogram_get $$h 6` == '10' ] ; \
	[ `redis-cli gsl_histogram_get $$h 7` == '10' ] ; \
	[ `redis-cli gsl_histogram_get $$h 8` == '10' ] ; \
	[ `redis-cli gsl_histogram_get $$h 9` == '10' ] ; \
	[ `redis-cli gsl_histogram_get $$h 10` == '0' ] ; \
	[ `redis-cli gsl_histogram_get_range $$h 0 | head -n1` == '0' ] ; \
	[ `redis-cli gsl_histogram_get_range $$h 0 | tail -n1` == '10' ] ; \
	[ `redis-cli gsl_histogram_mean $$h` ==  '432.936962750716333' ] ; \
	[ `redis-cli gsl_histogram_sigma $$h` == '22.40002386795928' ] ; \
	[ `redis-cli gsl_histogram_sum $$h` == '349' ] ; \
	for x in `seq 1 99` ; do \
		[ `redis-cli gsl_histogram_find $$h $$x` == `expr $$x / 10` ] ; \
	done ; \
done