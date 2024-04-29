#ifndef REP_INFO_H
#define REP_INFO_H

#include <stdlib.h>
#include <stdio.h>
#include "../OpenAcc/HPT_utilities.h"

typedef struct rep_info_t {    
	int replicas_total_number;
  int is_evenodd;
	int defect_boundary;
	int defect_coordinates[3];
	double *cr_vec;
	int *label;
} rep_info;
extern rep_info *rep;

int get_index_of_pbc_replica();

void setup_replica_utils();


typedef struct rep_utils_t {
  FILE *hmc_acc_file;
  FILE *swap_acc_file;
  FILE *file_label;

  int *all_swap_vector;
  int *acceptance_vector;
  double mean_acceptance;
  int *acceptance_vector_old;
  defect_info def;
  char rep_str [20];
  char aux_name_file[200];
} rep_utils;
extern rep_utils *r_utils;

#endif
