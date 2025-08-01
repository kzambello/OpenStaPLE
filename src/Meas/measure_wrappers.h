#ifndef MEASURE_WRAPPERS_H_
#define MEASURE_WRAPPERS_H_

#include <stdio.h>
#include <stdlib.h>
#include "../OpenAcc/double_complex.h"
#include "../Mpi/multidev.h"
#include "./measure_topo.h"
#include "./gauge_meas.h"
#include "../Include/rep_info.h"


typedef struct measure_wrapper_t {

  int id_iter;
  int id_iter_offset;
  double plq;
  double rect;
  double s4op; // sqrt(sum(P_mu^2))
  double s4op_0;
  double s4op_1;
  double s4op_2;
  double s4op_3;

  d_complex poly;

  // measurement buffers
  double *cool_topo_ch;
  double *stout_topo_ch;

  // acceptance buffers 
  int rankloc_accettate_therm;
  int rankloc_accettate_metro;
  int rankloc_accettate_therm_old;
  int rankloc_accettate_metro_old;

#ifdef PAR_TEMP
  int *accettate_therm;
  int *accettate_metro;
  int *accettate_therm_old;
  int *accettate_metro_old;
#endif

  int acceptance_to_print;

  char pathgauge_rep_idx[50];
  char  pathcool_rep_idx[50];
  char pathstout_rep_idx[50];

} measure_wrapper;

void init_meas_wrapper(measure_wrapper *meas_wrap_ptr,meastopo_param *meastopo_params_ptr, int conf_id_iter);

void update_acceptances(measure_wrapper *meas_wrap_ptr);

void send_local_acceptances(measure_wrapper *meas_wrap_ptr);

void sync_local_acceptances(measure_wrapper *meas_wrap_ptr, int which_mode);

void free_meas_wrapper(measure_wrapper *meas_wrap_ptr);

#endif // ndef MEASURE_WRAPPERS_H_
