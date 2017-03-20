#ifndef GSL_SERIALIZE_H
#define GSL_SERIALIZE_H

#include <sstream>
#include <string>
#include <gsl/gsl_rstat.h>
#include <gsl/gsl_histogram.h>

inline std::string gsl_rstat_quantile_workspace_serialize(const gsl_rstat_quantile_workspace* w) {
	std::stringstream s;
	s << w->p << " ";
	for (size_t i = 0; i < 5; i++) {
		s << w->q[i] << " ";
	}
	for (size_t i = 0; i < 5; i++) {
		s << w->npos[i] << " ";
	}
	for (size_t i = 0; i < 5; i++) {
		s << w->np[i] << " ";
	}
	for (size_t i = 0; i < 5; i++) {
		s << w->dnp[i] << " ";
	}
	s << w->n;
	return s.str();
}

inline void gsl_rstat_quantile_workspace_deserialize_(std::stringstream& s, gsl_rstat_quantile_workspace* w) {	
	s >> w->p;
	for (size_t i = 0; i < 5; i++) {
		s >> w->q[i];
	}
	for (size_t i = 0; i < 5; i++) {
		s >> w->npos[i];
	}
	for (size_t i = 0; i < 5; i++) {
		s >> w->np[i];
	}
	for (size_t i = 0; i < 5; i++) {
		s >> w->dnp[i];
	}
	s >> w->n;	
}

inline void gsl_rstat_quantile_workspace_deserialize(const std::string& str, gsl_rstat_quantile_workspace* w) {
	std::stringstream s;
	s.str(str);
	gsl_rstat_quantile_workspace_deserialize_(s, w);
}

inline std::string gsl_rstat_workspace_serialize(const gsl_rstat_workspace* w) {
	std::stringstream s;
	s << w->min << " " << w->max << " " << w->mean << " " << w->M2 << " " << w->M3 << w->M4 << " " << w->n << " " << gsl_rstat_quantile_workspace_serialize(w->median_workspace_p);	
	return s.str();
}

inline void gsl_rstat_quantile_workspace_deserialize(const std::string& str, gsl_rstat_workspace* w) {
	std::stringstream s;
	s.str(str);
	s >> w->min >> w->max >> w->mean >> w->M2 >> w->M3 >> w->M4>> w->n;
	gsl_rstat_quantile_workspace_deserialize_(s, (w->median_workspace_p));
}

inline std::string gsl_histogram_serialize(const gsl_histogram* h) {
	std::stringstream s;
	s << h->n << " ";
	for (size_t i = 0; i < h->n; i++) {
		s << h->range[i] << " ";
	}
	for (size_t i = 0; i < h->n; i++) {
		s << h->bin[i] << " ";
	}
	return s.str();
}

inline void gsl_histogram_deserialize(const std::string& str, gsl_histogram* h) {
	std::stringstream s;
	s.str(str);
	s >> h->n;
	for (size_t i = 0; i < h->n; i++) {
		s >> h->range[i];
	}
	for (size_t i = 0; i < h->n; i++) {
		s >> h->bin[i];
	}
}

inline std::string gsl_histogram_pdf_serialize(const gsl_histogram_pdf* p) {
	std::stringstream s;
	s << p->n << " ";
	for (size_t i = 0; i < p->n; i++) {
		s << p->range[i] << " ";
	}
	for (size_t i = 0; i < p->n; i++) {
		s << p->sum[i] << " ";
	}
	return s.str();
}

inline void gsl_histogram_pdf_serialize(const std::string& str, gsl_histogram_pdf* p) {
	std::stringstream s;
	s.str(str);
	s >> p->n;
	for (size_t i = 0; i < p->n; i++) {
		s >> p->range[i];
	}
	for (size_t i = 0; i < p->n; i++) {
		s >> p->sum[i];
	}
}

#endif