#ifndef REP_INFO_H
#define REP_INFO_H

#include <stdlib.h>
#include <stdio.h>
#include "common_defines.h"

//TODO: remove from code if ndef PAR_TEMP
typedef struct rep_info_t {    
	int replicas_total_number;
  int is_evenodd;
	int defect_boundary;
	int defect_coordinates[3];
	double *cr_vec;
	int *label;
} rep_info;
extern rep_info *rep;

#ifdef PAR_TEMP
typedef struct rep_utils_t {
  FILE *hmc_acc_file;
  FILE *swap_acc_file;
  FILE *file_label;

  int swap_number;
  int *all_swap_vector;
  int *acceptance_vector;
  double mean_acceptance;
  int *acceptance_vector_old;
  char rep_str [20];
  char aux_name_file[200];
} rep_utils;
extern rep_utils *r_utils;

int get_index_of_pbc_replica();
void setup_replica_utils();
void free_replica_utils();

void label_print(rep_info * hpt_params, FILE *file, int step_number);

#endif // PAR_TEMP

#endif // REP_INFO_H
