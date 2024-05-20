#ifndef MEASURE_WRAPPERS_H_
#define MEASURE_WRAPPERS_H_

#include <stdio.h>
#include <stdlib.h>
#include "../OpenAcc/double_complex.h"
#include "../Mpi/multidev.h"
#include "./measure_topo.h"


typedef struct measure_wrapper_t {

  int id_iter;
  int id_iter_offset;
  double plq;
  double rect;

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

} measure_wrapper;

void init_meas_wrapper(measure_wrapper *meas_wrap_ptr,meastopo_param *meastopo_params_ptr, int conf_id_iter);

void update_acceptances(measure_wrapper *meas_wrap_ptr);

void send_local_acceptances(measure_wrapper *meas_wrap_ptr);

void sync_local_acceptances(measure_wrapper *meas_wrap_ptr, int which_mode);

void free_meas_wrapper(measure_wrapper *meas_wrap_ptr);

#endif // ndef MEASURE_WRAPPERS_H_
