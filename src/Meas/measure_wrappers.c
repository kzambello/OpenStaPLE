#include "measure_wrappers.h"

extern dev_info devinfo;

void init_meas_wrapper(measure_wrapper *meas_wrap_ptr,meastopo_param *meastopo_params_ptr, int conf_id_iter){

  meas_wrap_ptr->cool_topo_ch = (double*)malloc(sizeof(double)*(meastopo_params_ptr->coolmeasstep/meastopo_params_ptr->cool_measinterval+1));
  meas_wrap_ptr->stout_topo_ch = (double*)malloc(sizeof(double)*(meastopo_params_ptr->stoutmeasstep/meastopo_params_ptr->stout_measinterval+1));

  meas_wrap_ptr->rankloc_accettate_therm=0;
  meas_wrap_ptr->rankloc_accettate_metro=0;
  
#ifdef PAR_TEMP
  if(0==devinfo.myrank_world){
    meas_wrap_ptr->accettate_therm=malloc(sizeof(int)*rep->replicas_total_number);
    meas_wrap_ptr->accettate_metro=malloc(sizeof(int)*rep->replicas_total_number);
    meas_wrap_ptr->accettate_therm_old=malloc(sizeof(int)*rep->replicas_total_number);
    meas_wrap_ptr->accettate_metro_old=malloc(sizeof(int)*rep->replicas_total_number);
    
    // inizialization to 0
    for(int lab=0;lab<rep->replicas_total_number;lab++){
      meas_wrap_ptr->accettate_therm[lab]=0;
      meas_wrap_ptr->accettate_metro[lab]=0;
      meas_wrap_ptr->accettate_therm_old[lab]=0;
      meas_wrap_ptr->accettate_metro_old[lab]=0;
    }
  }
#endif

  meas_wrap_ptr->id_iter_offset=conf_id_iter;
}

void update_acceptances(measure_wrapper *meas_wrap_ptr){
  if(0==devinfo.myrank_world){
#ifdef PAR_TEMP
    for (int lab=0;lab<rep->replicas_total_number;lab++){
      meas_wrap_ptr->accettate_therm_old[lab]= meas_wrap_ptr->accettate_therm[lab];
      meas_wrap_ptr->accettate_metro_old[lab]= meas_wrap_ptr->accettate_metro[lab];
    }
#else
    meas_wrap_ptr->rankloc_accettate_therm_old= meas_wrap_ptr->rankloc_accettate_therm;
    meas_wrap_ptr->rankloc_accettate_metro_old= meas_wrap_ptr->rankloc_accettate_metro;
#endif

  }
}

void send_local_acceptances(measure_wrapper *meas_wrap_ptr){
#ifdef PAR_TEMP
    // send acceptance values from all ranks and receives on world master
    //TODO: this introduces some overhead, possibly optimize
    for(int ridx=0; ridx<rep->replicas_total_number; ++ridx){
      for(int salarank=0; salarank<NRANKS_D3; ++salarank){
        if(0==devinfo.myrank_world){
          if(ridx==0  && salarank==0){
            meas_wrap_ptr->rankloc_accettate_therm=meas_wrap_ptr->accettate_therm[lab];
            meas_wrap_ptr->rankloc_accettate_metro=meas_wrap_ptr->accettate_metro[lab];
          }else{
            MPI_Send((int*)&(meas_wrap_ptr->accettate_therm[rep->label[ridx]]),1,MPI_INT,ridx*NRANKS_D3+salarank,salarank,MPI_COMM_WORLD);
            MPI_Send((int*)&(meas_wrap_ptr->accettate_metro[rep->label[ridx]]),1,MPI_INT,ridx*NRANKS_D3+salarank,salarank+NRANKS_D3,MPI_COMM_WORLD);
          }
        }else{
          if(ridx==devinfo.replica_idx && salarank==devinfo.myrank){
            MPI_Recv((int*)&(meas_wrap_ptr->rankloc_accettate_therm),1,MPI_INT,0,salarank,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv((int*)&(meas_wrap_ptr->rankloc_accettate_metro),1,MPI_INT,0,salarank+NRANKS_D3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
          }
        }
      }
    }
#endif
}

void sync_local_acceptances(measure_wrapper *meas_wrap_ptr, int which_mode){
#ifdef PAR_TEMP
    for(int ridx=0; ridx<rep->replicas_total_number; ++ridx){
      if(0==devinfo.myrank_world){
        if(ridx==0){
          accettate_therm[lab]=rankloc_accettate_therm;
          accettate_metro[lab]=rankloc_accettate_metro;
        }else{
          MPI_Recv((int*)&(meas_wrap_ptr->accettate_therm[rep->label[ridx]]),1,MPI_INT,ridx*NRANKS_D3,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
          MPI_Recv((int*)&(meas_wrap_ptr->accettate_metro[rep->label[ridx]]),1,MPI_INT,ridx*NRANKS_D3,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
      }else{
        if(ridx==devinfo.replica_idx && devinfo.myrank==0){
          MPI_Send((int*)&(meas_wrap_ptr->rankloc_accettate_therm),1,MPI_INT,0,0,MPI_COMM_WORLD);
          MPI_Send((int*)&(meas_wrap_ptr->rankloc_accettate_metro),1,MPI_INT,0,1,MPI_COMM_WORLD);
        }
      }
    }
#endif
    
        if(which_mode /* is metro */ && 0==devinfo.myrank_world){
#ifdef PAR_TEMP
            int iterations = meas_wrap_ptr->id_iter-meas_wrap_ptr->id_iter_offset-meas_wrap_ptr->accettate_therm[0]+1;
            double acceptance = (double) (meas_wrap_ptr->accettate_metro[0] / iterations;
            double acc_err = sqrt((double)meas_wrap_ptr->accettate_metro[0]*(iterations-meas_wrap_ptr->accettate_metro[0])/iterations)/iterations;
            printf("Estimated HMC acceptance for this run [replica %d]: %f +- %f\n. Iterations: %d\n",0,acceptance, acc_err, iterations);
#else
            int iterations = meas_wrap_ptr->id_iter-meas_wrap_ptr->id_iter_offset-meas_wrap_ptr->rankloc_accettate_therm+1;
            double acceptance = (double) meas_wrap_ptr->rankloc_accettate_metro / iterations;
            double acc_err = sqrt((double)(meas_wrap_ptr->rankloc_accettate_metro)*(iterations-meas_wrap_ptr->rankloc_accettate_metro)/iterations)/iterations;
            printf("Estimated HMC acceptance for this run: %f +- %f\n. Iterations: %d\n",acceptance, acc_err, iterations);
#endif
        }
}

void free_meas_wrapper(measure_wrapper *meas_wrap_ptr){

  free(meas_wrap_ptr->cool_topo_ch);
  free(meas_wrap_ptr->stout_topo_ch);

#ifdef PAR_TEMP
  if(0==devinfo.myrank_world){
    free(meas_wrap_ptr->accettate_therm);
    free(meas_wrap_ptr->accettate_metro);
    free(meas_wrap_ptr->accettate_therm_old);
    free(meas_wrap_ptr->accettate_metro_old);
  }
#endif
}
