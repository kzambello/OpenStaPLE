// here macros are defined
#define PRINT_DETAILS_INSIDE_UPDATE
#define ALIGN 128

// if using GCC, there are some problems with __restrict.
#ifdef __GNUC__
#define __restrict
#endif

#ifndef __GNUC__
#include "openacc.h"

// OpenAcc context initialization
// NVIDIA GPUs
#define MY_DEVICE_TYPE acc_device_nvidia
// AMD GPUs
//#define MY_DEVICE_TYPE acc_device_radeon
// Intel XeonPhi
//#define MY_DEVICE_TYPE acc_device_xeonphi
// Select device ID

#endif

#ifdef ONE_FILE_COMPILATION
#include "../Include/all_include.h"
#endif

#include "../DbgTools/dbgtools.h"
#include "../DbgTools/debugger_hook.h"
#include "../Include/debug.h"
#include "../Include/fermion_parameters.h"
#include "../Include/montecarlo_parameters.h"
#include "../Include/inverter_tricks.h"
#include "../Include/memory_wrapper.h"
#include "../Include/setting_file_parser.h"
#include "../Include/tell_geom_defines.h"
#include "../Include/rep_info.h"
#include "../Include/acceptances_info.h"
#include "../Meas/ferm_meas.h"
#include "../Meas/gauge_meas.h"
#include "../Meas/polyakov.h"
#include "../Meas/measure_topo.h"
#include "../Meas/measure_wrappers.h"
#include "../Mpi/communications.h"
#include "../Mpi/multidev.h"
#include "../Rand/random.h"
#include "../RationalApprox/rationalapprox.h"
#include "./action.h"
#include "./alloc_vars.h"
#include "./backfield_parameters.h"
#include "./deviceinit.h"
#include "./fermion_matrix.h"
#include "./fermionic_utilities.h"
#include "./find_min_max.h"
#include "./float_double_conv.h"
#include "./inverter_full.h"
#include "./inverter_multishift_full.h"
#include "./io.h"
#include "./ipdot_gauge.h"
#include "./md_integrator.h"
#include "./md_parameters.h"
#include "./random_assignement.h"
#include "./rectangles.h"
#include "./sp_alloc_vars.h"
#include "./stouting.h"
#include "./struct_c_def.h"
#include "./su3_measurements.h"
#include "./su3_utilities.h"
#include "./update_versatile.h"
#include "./alloc_settings.h"

#ifdef __GNUC__
#include "sys/time.h"
#endif
#include "./cooling.h"
#include <unistd.h>
#include <mpi.h>
#include "../Include/stringify.h"

#include <errno.h>
#include <sys/stat.h>

int check_file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}
#include <time.h>

#include "./HPT_utilities.h"

// double level macro, necessary to stringify
// https://gcc.gnu.org/onlinedocs/cpp/Stringification.html
#define xstr(s) str(s) 
#define str(s) #s

#ifdef PAR_TEMP
#define IF_PERIODIC_REPLICA() \
  if(0==rep->label[devinfo.replica_idx]) 
#else
#define IF_PERIODIC_REPLICA()
#endif 


//TODO: maybe encapsulate these into some structure with global access
// global variables 
int conf_id_iter;
int verbosity_lv;
unsigned int myseed_default;
#ifdef PAR_TEMP
defect_info def_info;
#endif

void first_conf_measurement(measure_wrapper *meas_wrap_ptr){
  if(0 == mc_params.ntraj && 0 == mc_params.JarzynskiMode ){ // measures only
      
    printf("\n#################################################\n");
    printf("\tMEASUREMENTS ONLY ON FILE %s\n", mc_params.save_conf_name);
    printf("\n#################################################\n");

    IF_PERIODIC_REPLICA(){
      // gauge stuff measures
      printf("Gauge Measures:\n");
      meas_wrap_ptr->plq = calc_plaquette_soloopenacc(conf_acc,aux_conf_acc,local_sums);

#if !defined(GAUGE_ACT_WILSON) || !(NRANKS_D3 > 1)
      meas_wrap_ptr->rect = calc_rettangolo_soloopenacc(conf_acc,aux_conf_acc,local_sums);
#else
      MPI_PRINTF0("multidevice rectangle computation with Wilson action not implemented\n");
#endif 
      meas_wrap_ptr->poly =  (*polyakov_loop[geom_par.tmap])(conf_acc);//misura polyakov loop

      meas_wrap_ptr->s4op = sqrt(  pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0),2)
                                 + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1),2)
                                 + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2),2)
                                 + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3),2));

      meas_wrap_ptr->s4op_0 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0);
      meas_wrap_ptr->s4op_1 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1);
      meas_wrap_ptr->s4op_2 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2);
      meas_wrap_ptr->s4op_3 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3);
        
      printf("Plaquette     : %.18lf\n" ,meas_wrap_ptr->plq/GL_SIZE/3.0/6.0);
      printf("Rectangle     : %.18lf\n" ,meas_wrap_ptr->rect/GL_SIZE/3.0/6.0/2.0);
      printf("Polyakov Loop : (%.18lf,%.18lf) \n",creal(meas_wrap_ptr->poly),cimag(meas_wrap_ptr->poly));
      printf("|S4OP|        : %.18lf\n" ,meas_wrap_ptr->s4op/GL_SIZE/3.0/6.0);
      printf("S4OP          : (%.18lf,%.18lf,%.18lf,%.18lf)\n" ,meas_wrap_ptr->s4op_0/GL_SIZE/3.0/6.0,meas_wrap_ptr->s4op_1/GL_SIZE/3.0/6.0,meas_wrap_ptr->s4op_2/GL_SIZE/3.0/6.0,meas_wrap_ptr->s4op_3/GL_SIZE/3.0/6.0);

      // fermionic stuff measures
      //
      printf("Fermion Measurements: see file %s\n",
																		fm_par.fermionic_outfilename);
      fermion_measures(conf_acc,fermions_parameters,
                       &fm_par, md_parameters.residue_metro, 
                       md_parameters.max_cg_iterations, meas_wrap_ptr->id_iter_offset,
                       meas_wrap_ptr->plq/GL_SIZE/3.0/6.0,
                       meas_wrap_ptr->rect/GL_SIZE/3.0/6.0/2.0);   
    }

  }else MPI_PRINTF0("Starting generation of Configurations.\n");
}


int loc_max_update_times, glob_max_update_times;
int loc_max_flavour_cycle_times, glob_max_flavour_cycle_times;
int loc_max_run_times, glob_max_run_times;

void check_and_update_run_condition(measure_wrapper *meas_wrap_ptr){
  if(0 == devinfo.myrank_world && RUN_CONDITION_TERMINATE != mc_params.run_condition){ 
   
    // program exits if it finds a file called "stop"

    FILE * test_stop = fopen("stop","r");
    if(test_stop){
      fclose(test_stop);
      printf("File  \'stop\' found, stopping cycle now.\n");
      mc_params.run_condition = RUN_CONDITION_TERMINATE;
    }

    // program exits if time is running out
      
    struct timeval now;
    gettimeofday(&now,NULL);
    double total_duration = (double) 
      (now.tv_sec - mc_params.start_time.tv_sec)+
      (double)(now.tv_usec - mc_params.start_time.tv_usec)/1.0e6;

    double max_expected_duration_with_another_cycle;
    if(GPSTATUS_UPDATE == mc_params.next_gps){
      max_expected_duration_with_another_cycle = 
        total_duration + 1.3*glob_max_update_times;
      printf("Next step, update : %ds\n",(int) glob_max_update_times);
    }
    if(GPSTATUS_FERMION_MEASURES == mc_params.next_gps){
      max_expected_duration_with_another_cycle = 
        total_duration + 2*glob_max_flavour_cycle_times;
      printf("Next step, flavour measure cycle : %ds\n",
             (int) glob_max_flavour_cycle_times);
    }

    if(max_expected_duration_with_another_cycle > glob_max_run_times){
      printf("Time is running out (%d of %d seconds elapsed),",
             (int) total_duration, (int) glob_max_run_times);
      printf(" shutting down now.\n");
      printf("Total max expected duration: %d seconds",
             (int) max_expected_duration_with_another_cycle);
      printf("(%d elapsed now)\n",(int) total_duration);
      // https://www.youtube.com/watch?v=MfGhlVcrc8U
      // but without that much pathos
      mc_params.run_condition = RUN_CONDITION_TERMINATE;
    }

    // program exits if MaxConfIdIter is reached
    if(conf_id_iter >= mc_params.MaxConfIdIter ){

      printf("%s - MaxConfIdIter=%d reached, job done!",
             devinfo.myrankstr, mc_params.MaxConfIdIter);
      printf("%s - shutting down now.\n", devinfo.myrankstr);
      mc_params.run_condition = RUN_CONDITION_TERMINATE;
    }
    // program exits if MTraj is reached
    if(meas_wrap_ptr->id_iter >= (mc_params.ntraj+meas_wrap_ptr->id_iter_offset)){
      printf("%s - NTraj=%d reached, job done!",
             devinfo.myrankstr, mc_params.ntraj);
      printf("%s - shutting down now.\n", devinfo.myrankstr);
      mc_params.run_condition = RUN_CONDITION_TERMINATE;
    }
    if (0==mc_params.ntraj) {
      printf("%s - NTraj=%d reached, job done!",
             devinfo.myrankstr, mc_params.ntraj);
      printf("%s - shutting down now.\n", devinfo.myrankstr);
      mc_params.run_condition = RUN_CONDITION_TERMINATE;
    }
  }

#ifdef MULTIDEVICE
    MPI_Bcast((void*)&(mc_params.run_condition),1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_PRINTF1("Broadcast of run condition %d from master...\n", mc_params.run_condition);

    int loc_check=mc_params.next_gps;
    MPI_Bcast((void*)&(mc_params.next_gps),1,MPI_INT,0,MPI_COMM_WORLD);
    if(loc_check!=mc_params.next_gps){
      printf("ERROR: mismatch in the next_gps step between different ranks\n");
      MPI_Abort(MPI_COMM_WORLD,1);			
    }
    MPI_PRINTF1("Broadcast of next global program status %d from master...\n", mc_params.next_gps);

    MPI_Barrier(MPI_COMM_WORLD);
#endif
}




void apply_Jarzinsky_mode(measure_wrapper *meas_wrap_ptr){
     bf_param new_backfield_parameters = backfield_parameters;

     // direct mode 
     if(1 == mc_params.JarzynskiMode)
         new_backfield_parameters.bz = backfield_parameters.bz + 
             (double) meas_wrap_ptr->id_iter/mc_params.MaxConfIdIter;
     // reverse mode
     if(-1 == mc_params.JarzynskiMode)
         new_backfield_parameters.bz = backfield_parameters.bz -
             (double) meas_wrap_ptr->id_iter/mc_params.MaxConfIdIter;

     if(0==devinfo.myrank_world){

         if(1 == mc_params.JarzynskiMode)
             printf("\n\nJarzynskiMode - DIRECT - From bz=%f to bz=%f+1 in %d steps.\n",
                          backfield_parameters.bz , backfield_parameters.bz, 
                          mc_params.MaxConfIdIter);
         if(-1 == mc_params.JarzynskiMode)
             printf("\n\nJarzynskiMode - REVERSE - From bz=%f to bz=%f-1 in %d steps.\n",
                          backfield_parameters.bz , backfield_parameters.bz, 
                          mc_params.MaxConfIdIter);

         if(1 == mc_params.JarzynskiMode)
             printf("\n\nJarzynskiMode - DIRECT - From bz=%f to bz=%f+1 in %d steps.\n",
                          backfield_parameters.bz , backfield_parameters.bz, 
                          mc_params.MaxConfIdIter);
         if(-1 == mc_params.JarzynskiMode)
             printf("\n\nJarzynskiMode - REVERSE - From bz=%f to bz=%f-1 in %d steps.\n",
                          backfield_parameters.bz , backfield_parameters.bz, 
                          mc_params.MaxConfIdIter);

         printf("JarzynskiMode, iteration %d/%d (%d max for this run)\n",
                      meas_wrap_ptr->id_iter,mc_params.MaxConfIdIter,mc_params.ntraj);
         printf("JarzynskiMode - current bz value : %f\n", new_backfield_parameters.bz);
     }

     init_all_u1_phases(new_backfield_parameters,fermions_parameters);
     #pragma acc update device(u1_back_phases[0:8*alloc_info.NDiffFlavs])
     #pragma acc update device(u1_back_phases_f[0:8*alloc_info.NDiffFlavs])
     printf("Jarzynski mode's end\n");
}


void print_action_HMC(char before_after[]){
    double action;
    action  = - C_ZERO * BETA_BY_THREE * calc_plaquette_soloopenacc(conf_acc, aux_conf_acc, local_sums);
#ifdef GAUGE_ACT_TLSM
    action += - C_ONE  * BETA_BY_THREE * calc_rettangolo_soloopenacc(conf_acc, aux_conf_acc, local_sums);
#endif

#ifdef PAR_TEMP
    int lab=rep->label[devinfo.replica_idx];
    printf("REPLICA %d (index %d):\n",lab,devinfo.replica_idx);
    printf("ACTION %s HMC STEP REPLICA %d (idx %d): %.15lg\n", before_after, lab, devinfo.replica_idx, action);
#else
    printf("ACTION %s HMC STEP: %.15lg\n", before_after, action);
#endif

}

#ifdef PAR_TEMP
void perform_replica_step(){
    if(rep->replicas_total_number>1){
        // conf swap
        if (0==devinfo.myrank_world) {printf("CONF SWAP PROPOSED\n");}
        #pragma acc update host(conf_acc[0:alloc_info.conf_acc_size])
        manage_replica_swaps(conf_acc, aux_conf_acc, local_sums, &def_info, &(r_utils->swap_number),r_utils->all_swap_vector,r_utils->acceptance_vector,rep);

        if (0==devinfo.myrank_world) {printf("Number of accepted swaps: %d\n", r_utils->swap_number);}       
        #pragma acc update host(conf_acc[0:8])

        // periodic conf translation
        if(0==rep->label[devinfo.replica_idx]){
          trasl_conf(conf_acc,auxbis_conf_acc);
        }
    }
    #pragma acc update host(conf_acc[0:8])
}
#endif

void sync_acceptance_to_print(measure_wrapper *meas_wrap_ptr){
#ifdef PAR_TEMP
    if(0==devinfo.myrank_world){
      meas_wrap_ptr->acceptance_to_print=meas_wrap_ptr->accettate_therm[0]+meas_wrap_ptr->accettate_metro[0]-meas_wrap_ptr->accettate_therm_old[0]-meas_wrap_ptr->accettate_metro_old[0];
      int ridx_lab0 = get_index_of_pbc_replica(); // finds index corresponding to label=0
      if(ridx_lab0!=0){
        MPI_Send((int*)&meas_wrap_ptr->acceptance_to_print,1,MPI_INT,ridx_lab0*NRANKS_D3,0,MPI_COMM_WORLD);
      }
    }else{
      if(0==rep->label[devinfo.replica_idx] && devinfo.myrank==0){
        MPI_Recv((int*)&meas_wrap_ptr->acceptance_to_print,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      }
    }
#else
    meas_wrap_ptr->acceptance_to_print=meas_wrap_ptr->rankloc_accettate_therm+meas_wrap_ptr->rankloc_accettate_metro-meas_wrap_ptr->rankloc_accettate_therm_old-meas_wrap_ptr->rankloc_accettate_metro_old;
#endif
}


void perform_all_measurements(measure_wrapper *meas_wrap_ptr){

  // perform measurement on the periodic replica, or in all if activated meas_all_reps
#ifdef PAR_TEMP
  if(0==rep->label[devinfo.replica_idx] || 1==rep->meas_all_reps) 
#endif
  {
    printf("===========GAUGE MEASURING============\n");
      
    meas_wrap_ptr->plq  = calc_plaquette_soloopenacc(conf_acc,aux_conf_acc,local_sums);
#if !defined(GAUGE_ACT_WILSON) || !(NRANKS_D3 > 1)
    meas_wrap_ptr->rect = calc_rettangolo_soloopenacc(conf_acc,aux_conf_acc,local_sums);
#else
    MPI_PRINTF0("multidevice rectangle computation with Wilson action not implemented\n");
#endif
    meas_wrap_ptr->poly =  (*polyakov_loop[geom_par.tmap])(conf_acc);

    meas_wrap_ptr->s4op = sqrt(  pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0),2)
                               + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1),2)
                               + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2),2)
                               + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3),2));

    meas_wrap_ptr->s4op_0 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0);
    meas_wrap_ptr->s4op_1 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1);
    meas_wrap_ptr->s4op_2 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2);
    meas_wrap_ptr->s4op_3 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3);
      
    if(meastopo_params.meascool && conf_id_iter%meastopo_params.cooleach==0){
      su3_soa *conf_to_use;
      meas_wrap_ptr->cool_topo_ch[0]=compute_topological_charge(conf_acc,auxbis_conf_acc,topo_loc);
      for(int cs = 1; cs <= meastopo_params.coolmeasstep; cs++){
        if(cs==1)
          conf_to_use=(su3_soa*)conf_acc;
        else
          conf_to_use=(su3_soa*)aux_conf_acc;
        cool_conf(conf_to_use,aux_conf_acc,auxbis_conf_acc);
        if(cs%meastopo_params.cool_measinterval==0)
          meas_wrap_ptr->cool_topo_ch[cs/meastopo_params.cool_measinterval]=compute_topological_charge(aux_conf_acc,auxbis_conf_acc,topo_loc);
      }
      MPI_PRINTF0("Printing cooled charge - only by master rank...\n");
      if(devinfo.myrank ==0){
        FILE *cooloutfile;
        IF_PERIODIC_REPLICA()
        {
          if(!check_file_exists(meastopo_params.pathcool)){ 
            cooloutfile = fopen(meastopo_params.pathcool,"wt");
            fprintf(cooloutfile,"#conf_id\tCoolStp\tTopoChCool\n");  
            fclose(cooloutfile);
          }
          cooloutfile = fopen(meastopo_params.pathcool,"at");
          for(int i = 0; i <= meastopo_params.coolmeasstep/meastopo_params.cool_measinterval;i++)
            fprintf(cooloutfile,"%d\t%d\t%18.18lf\n",conf_id_iter,
                    i*meastopo_params.cool_measinterval,
                    meas_wrap_ptr->cool_topo_ch[i]);
          fclose(cooloutfile);
        }
#ifdef PAR_TEMP
        if(1==rep->meas_all_reps){ // replica dependent measurements
          if(!check_file_exists(meas_wrap_ptr->pathcool_rep_idx)){ 
            cooloutfile = fopen(meas_wrap_ptr->pathcool_rep_idx,"wt");
            fprintf(cooloutfile,"#conf_id\trep_lab\tCoolStp\tTopoChCool\n");
            fclose(cooloutfile);
          }
          cooloutfile = fopen(meas_wrap_ptr->pathcool_rep_idx,"at");
          for(int i = 0; i <= meastopo_params.coolmeasstep/meastopo_params.cool_measinterval;i++)
            fprintf(cooloutfile,"%d\t%d\t%d\t%18.18lf\n",conf_id_iter,
                    rep->label[devinfo.replica_idx],
                    i*meastopo_params.cool_measinterval,
                    meas_wrap_ptr->cool_topo_ch[i]);
          fclose(cooloutfile);
        }
#endif
      }
    }

    if(meastopo_params.measstout && conf_id_iter%meastopo_params.stouteach==0){
      stout_wrapper(conf_acc,gstout_conf_acc_arr,1);
      meas_wrap_ptr->stout_topo_ch[0]=compute_topological_charge(conf_acc,auxbis_conf_acc,topo_loc);
      for(int ss = 0; ss < meastopo_params.stoutmeasstep; ss+=meastopo_params.stout_measinterval){
        int topoindx =1+ss/meastopo_params.stout_measinterval; 
        meas_wrap_ptr->stout_topo_ch[topoindx]=compute_topological_charge(&gstout_conf_acc_arr[8*ss],auxbis_conf_acc,topo_loc);
      }

      MPI_PRINTF0("Printing stouted charge - only by master rank...\n");
      if(devinfo.myrank ==0){
        FILE *stoutoutfile;
        IF_PERIODIC_REPLICA()
        {
          if(!check_file_exists(meastopo_params.pathstout)){ 
            stoutoutfile = fopen(meastopo_params.pathstout,"wt");
            fprintf(stoutoutfile,"#conf_id\tStoutStp\tTopoChStout\n");
            fclose(stoutoutfile);
          }
          stoutoutfile = fopen(meastopo_params.pathstout,"at");
          for(int i = 0; i <= meastopo_params.stoutmeasstep/meastopo_params.stout_measinterval;i++)
            fprintf(stoutoutfile,"%d\t%d\t%18.18lf\n",conf_id_iter,
                    i*meastopo_params.stout_measinterval,
                    meas_wrap_ptr->stout_topo_ch[i]);
          fclose(stoutoutfile);
        }
#ifdef PAR_TEMP
        if(1==rep->meas_all_reps){ // replica dependent measurements
          if(!check_file_exists(meas_wrap_ptr->pathstout_rep_idx)){ 
            stoutoutfile = fopen(meas_wrap_ptr->pathstout_rep_idx,"wt");
            fprintf(stoutoutfile,"#conf_id\trep_lab\tStoutStp\tTopoChStout\n");
            fclose(stoutoutfile);
          }
          stoutoutfile = fopen(meas_wrap_ptr->pathstout_rep_idx,"at");


          for(int i = 0; i <= meastopo_params.stoutmeasstep/meastopo_params.stout_measinterval;i++)
            fprintf(stoutoutfile,"%d\t%d\t%d\t%18.18lf\n",conf_id_iter,
                    rep->label[devinfo.replica_idx],
                    i*meastopo_params.stout_measinterval,
                    meas_wrap_ptr->stout_topo_ch[i]);
          fclose(stoutoutfile);
        }
#endif // def PAR_TEMP
      }
    }//if stout end

    MPI_PRINTF0("Printing gauge obs - only by master rank...\n");
    if(devinfo.myrank ==0){
      FILE *goutfile;
      IF_PERIODIC_REPLICA()
      {
        if(!check_file_exists(gauge_outfilename)){ 
          goutfile = fopen(gauge_outfilename,"wt");
          fprintf(goutfile,"#conf_id\tacc\tplq\trect\tReP\tImP\tS4OP\tS4OP_0\tS4OP_1\tS4OP_2\tS4OP_3\n");
          fclose(goutfile);
        }
        goutfile = fopen(gauge_outfilename,"at");
        if(meas_wrap_ptr->id_iter<mc_params.therm_ntraj){
          printf("Therm_iter %d",conf_id_iter );
          printf("Plaquette = %.18lf    ", meas_wrap_ptr->plq/GL_SIZE/6.0/3.0);
          printf("Rectangle = %.18lf\n",meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0);
        }else printf("Metro_iter %d   Plaquette= %.18lf    Rectangle = %.18lf\n",conf_id_iter,meas_wrap_ptr->plq/GL_SIZE/6.0/3.0,meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0);

        fprintf(goutfile,"%d\t%d\t",conf_id_iter,meas_wrap_ptr->acceptance_to_print);
              
        fprintf(goutfile,"%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\n",
                meas_wrap_ptr->plq/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0, 
                creal(meas_wrap_ptr->poly), cimag(meas_wrap_ptr->poly),
                meas_wrap_ptr->s4op/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_0/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_1/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_2/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_3/GL_SIZE/6.0/3.0);
        fclose(goutfile);
      }
#ifdef PAR_TEMP
      if(1==rep->meas_all_reps){ // replica dependent measurements
        if(!check_file_exists(meas_wrap_ptr->pathgauge_rep_idx)){ 
          goutfile = fopen(meas_wrap_ptr->pathgauge_rep_idx,"wt");
          fprintf(goutfile,"#conf_id\trep_lab\tacc\tplq\trect\tReP\tImP\tS4OP\tS4OP_0\tS4OP_1\tS4OP_2\tS4OP_3\n");
          fclose(goutfile);
        }
        goutfile = fopen(meas_wrap_ptr->pathgauge_rep_idx,"at");
        if(meas_wrap_ptr->id_iter<mc_params.therm_ntraj){
          MPI_PRINTF1("Therm_iter %d",conf_id_iter );
          MPI_PRINTF1("Plaquette = %.18lf    ", meas_wrap_ptr->plq/GL_SIZE/6.0/3.0);
          MPI_PRINTF1("Rectangle = %.18lf\n",meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0);
        }else MPI_PRINTF1("Metro_iter %d   Plaquette= %.18lf    Rectangle = %.18lf\n",conf_id_iter,meas_wrap_ptr->plq/GL_SIZE/6.0/3.0,meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0);

        fprintf(goutfile,"%d\t%d\t%d\t",
            conf_id_iter,
            rep->label[devinfo.replica_idx],
            meas_wrap_ptr->acceptance_to_print);
              
        fprintf(goutfile,"%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\t%.18lf\n",
                meas_wrap_ptr->plq/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->rect/GL_SIZE/6.0/3.0/2.0, 
                creal(meas_wrap_ptr->poly), cimag(meas_wrap_ptr->poly),
                meas_wrap_ptr->s4op/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_0/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_1/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_2/GL_SIZE/6.0/3.0,
                meas_wrap_ptr->s4op_3/GL_SIZE/6.0/3.0);
        fclose(goutfile);
      }
#endif // def PAR_TEMP
    }
  }
}






void update_times(){
        loc_max_update_times=mc_params.max_update_time;
        loc_max_flavour_cycle_times=mc_params.max_flavour_cycle_time;
        loc_max_run_times=mc_params.MaxRunTimeS;

#ifdef MULTIDEVICE
        MPI_Allreduce((void*)&loc_max_update_times, (void*)&glob_max_update_times,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
        MPI_Allreduce((void*)&loc_max_flavour_cycle_times, (void*)&glob_max_flavour_cycle_times,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
        MPI_Allreduce((void*)&loc_max_run_times, (void*)&glob_max_run_times,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
#else
        glob_max_update_times= loc_max_update_times;
        glob_max_flavour_cycle_times= loc_max_flavour_cycle_times;
        glob_max_run_times= loc_max_run_times;
#endif
}





void main_loop(){
  measure_wrapper meas_wrap;
  init_meas_wrapper(&meas_wrap,&meastopo_params, conf_id_iter);

  // plaquette measures and polyakov loop measures.
  printf("PLAQUETTE START\n");
    
  IF_PERIODIC_REPLICA(){
    meas_wrap.plq = calc_plaquette_soloopenacc(conf_acc,aux_conf_acc,local_sums);
    MPI_PRINTF1("Therm_iter %d Placchetta    = %.18lf \n", conf_id_iter,meas_wrap.plq/GL_SIZE/6.0/3.0);
  }
    
  printf("PLAQUETTE END\n");

#if !defined(GAUGE_ACT_WILSON) || !(NRANKS_D3 > 1)
  IF_PERIODIC_REPLICA(){
    meas_wrap.rect = calc_rettangolo_soloopenacc(conf_acc,aux_conf_acc,local_sums);
    MPI_PRINTF1("Therm_iter %d Rettangolo = %.18lf \n", conf_id_iter,meas_wrap.rect/GL_SIZE/6.0/3.0/2.0);
  }
#else
  MPI_PRINTF0("multidevice rectangle computation with Wilson action not implemented\n");
#endif

  IF_PERIODIC_REPLICA(){
    meas_wrap.poly =  (*polyakov_loop[geom_par.tmap])(conf_acc);
    MPI_PRINTF1("Therm_iter %d Polyakov Loop = (%.18lf, %.18lf)  \n", conf_id_iter,creal(meas_wrap.poly),cimag(meas_wrap.poly));
  }


  IF_PERIODIC_REPLICA(){
    meas_wrap.s4op = sqrt(  pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0),2)
                          + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1),2)
                          + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2),2)
                          + pow(calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3),2));
    meas_wrap.s4op_0 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,0);
    meas_wrap.s4op_1 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,1);
    meas_wrap.s4op_2 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,2);
    meas_wrap.s4op_3 = calc_s4op_soloopenacc(conf_acc,aux_conf_acc,local_sums,3);
    MPI_PRINTF1("Therm_iter %d |S4OP| = %.18lf  S4OP = (%.18lf, %.18lf, %.18lf, %.18lf) \n", conf_id_iter,meas_wrap.s4op/GL_SIZE/3.0/6.0,meas_wrap.s4op_0/GL_SIZE/3.0/6.0,meas_wrap.s4op_1/GL_SIZE/3.0/6.0,meas_wrap.s4op_2/GL_SIZE/3.0/6.0,meas_wrap.s4op_3/GL_SIZE/3.0/6.0);
  }
	
  first_conf_measurement(&meas_wrap);
     
  // thermalization & metropolis updates

  meas_wrap.id_iter=meas_wrap.id_iter_offset;
    
  init_global_program_status(); 
  loc_max_update_times=0;
  loc_max_flavour_cycle_times=0;
  loc_max_run_times=0;

  printf("run_condition: %d\n",mc_params.run_condition);
  if ( 0 != mc_params.ntraj ){
    while ( RUN_CONDITION_TERMINATE != mc_params.run_condition){
      if(GPSTATUS_UPDATE == mc_params.next_gps){
        struct timeval tstart_cycle,tend_cycle;
        gettimeofday(&tstart_cycle, NULL);

        if(0 != mc_params.JarzynskiMode ) apply_Jarzinsky_mode(&meas_wrap);

        double avg_unitarity_deviation,max_unitarity_deviation;
        check_unitarity_device(conf_acc,&max_unitarity_deviation, &avg_unitarity_deviation);
        MPI_PRINTF1("Avg/Max unitarity deviation on device: %e / %e\n",avg_unitarity_deviation,max_unitarity_deviation);

        update_acceptances(&meas_wrap);  

        if(0==devinfo.myrank_world){
          printf("\n#################################################\n"); 
          printf(  "   GENERATING CONF %d of %d, %dx%dx%dx%d,%1.3f \n", 
          conf_id_iter,mc_params.ntraj+meas_wrap.id_iter_offset,
                                 geom_par.gnx,geom_par.gny,
                                 geom_par.gnz,geom_par.gnt,
                                 act_params.beta);
                printf(  "#################################################\n\n");
          }

#ifdef PAR_TEMP
          for(int i=0;i<rep->replicas_total_number-1;i++){
            r_utils->acceptance_vector_old[i]=r_utils->acceptance_vector[i];
          }
#endif
          {
                // initial action
                if (verbosity_lv>10){
                    print_action_HMC("BEFORE");
                }

                // HMC step
                int which_mode=(meas_wrap.id_iter<mc_params.therm_ntraj)? 0 : 1; // 0: therm, 1: metro
                int *rankloc_accettate_which[2]={(int*)&(meas_wrap.rankloc_accettate_therm),(int*)&(meas_wrap.rankloc_accettate_metro)};
                int effective_iter = meas_wrap.id_iter-meas_wrap.id_iter_offset-(which_mode==1? meas_wrap.rankloc_accettate_therm : 0);


#ifdef PAR_TEMP
                send_local_acceptances(&meas_wrap);
#endif
    
                *rankloc_accettate_which[which_mode] = UPDATE_SOLOACC_UNOSTEP_VERSATILE(conf_acc,
                                                          md_parameters.residue_metro,md_parameters.residue_md, effective_iter,
                                                          *(rankloc_accettate_which[which_mode]),which_mode,md_parameters.max_cg_iterations);

                // sync acceptance array on world master
                sync_local_acceptances(&meas_wrap,which_mode);

                #pragma acc update host(conf_acc[0:8])

                // final action
                if (verbosity_lv>10){
                    print_action_HMC("AFTER");
                }

#ifdef PAR_TEMP
                perform_replica_step();
#endif
          }

          meas_wrap.id_iter++;
          conf_id_iter++;
    
#ifdef PAR_TEMP
          MPI_PRINTF0("Printing acceptances - only by master master rank...\n");
          if(devinfo.myrank_world ==0){
    
              if(rep->replicas_total_number>1){
                  r_utils->file_label=fopen(acc_info->file_label_name,"at");
                  if(!(r_utils->file_label)){r_utils->file_label=fopen(acc_info->file_label_name,"wt");}
                  label_print(rep, r_utils->file_label, conf_id_iter);

                  r_utils->hmc_acc_file=fopen(acc_info->hmc_file_name,"at");
                  if(!r_utils->hmc_acc_file){r_utils->hmc_acc_file=fopen(acc_info->hmc_file_name,"wt");}
                  fprintf(r_utils->hmc_acc_file,"%d\t",conf_id_iter);
          
                  r_utils->swap_acc_file=fopen(acc_info->swap_file_name,"at");
                  if(!r_utils->swap_acc_file){r_utils->swap_acc_file=fopen(acc_info->swap_file_name,"wt");}
                  fprintf(r_utils->swap_acc_file,"%d\t",conf_id_iter);

              }
// print   acceptances
              for(int lab=0;lab<rep->replicas_total_number;lab++){
                  if(lab<rep->replicas_total_number-1){
                      r_utils->mean_acceptance=(double)r_utils->acceptance_vector[lab]/r_utils->all_swap_vector[lab];
                      printf("replica couple [labels: %d/%d]: proposed %d, accepted %d, mean_acceptance %f\n",lab,lab+1,r_utils->all_swap_vector[lab],r_utils->acceptance_vector[lab],r_utils->mean_acceptance);
                      if(rep->replicas_total_number>1){
                          fprintf(r_utils->swap_acc_file,"%d\t",r_utils->acceptance_vector[lab]-r_utils->acceptance_vector_old[lab]);
                      }
                  }

                  if(rep->replicas_total_number>1){
                      fprintf(r_utils->hmc_acc_file,"%d\t", meas_wrap.accettate_therm[lab]+meas_wrap.accettate_metro[lab] -meas_wrap.accettate_therm_old[lab]-meas_wrap.accettate_metro_old[lab]);
                  }
      
              }
    
              if(rep->replicas_total_number>1){
    
                  fprintf(r_utils->hmc_acc_file,"\n");
                  fprintf(r_utils->swap_acc_file,"\n");
    
                  fclose(r_utils->hmc_acc_file);
                  fclose(r_utils->swap_acc_file);
                  fclose(r_utils->file_label);
    
              }
          }
#endif
  
    // gauge stuff measures
    sync_acceptance_to_print(&meas_wrap);


    perform_all_measurements(&meas_wrap);
 
    // saving conf_store
    // saves gauge conf and rng status to file
    if(conf_id_iter%mc_params.storeconfinterval==0){ 
      char tempname[50];
      char serial[10];
      strcpy(tempname,mc_params.store_conf_name);
      sprintf(serial,".%05d",conf_id_iter);
      strcat(tempname,serial);
      MPI_PRINTF1("Storing conf %s.\n", tempname);
      save_conf_wrapper(conf_acc,tempname,conf_id_iter,
                        debug_settings.use_ildg);
      strcpy(tempname,mc_params.RandGenStatusFilename);
      sprintf(serial,".%05d",conf_id_iter);
      strcat(tempname,serial);
      MPI_PRINTF1("Storing rng status in %s.\n" , tempname);
      saverand_tofile(tempname);
    }

    if(conf_id_iter%mc_params.saveconfinterval==0){
      {
#ifdef PAR_TEMP        
          snprintf(r_utils->rep_str,20,"replica_%d",devinfo.replica_idx);
          strcat(mc_params.save_conf_name,r_utils->rep_str);
#endif
          if (debug_settings.SaveAllAtEnd){
              MPI_PRINTF1("Saving conf %s.\n", mc_params.save_conf_name);
              save_conf_wrapper(conf_acc,mc_params.save_conf_name, conf_id_iter,
                                                  debug_settings.use_ildg);
          }else
              MPI_PRINTF0("WARNING, \'SaveAllAtEnd\'=0,NOT SAVING/OVERWRITING CONF AND RNG STATUS.\n\n\n");
#ifdef PAR_TEMP        
          strcpy(mc_params.save_conf_name,r_utils->aux_name_file);
#endif
      }
      if (debug_settings.SaveAllAtEnd){
          MPI_PRINTF1("Saving rng status in %s.\n", mc_params.RandGenStatusFilename);
          saverand_tofile(mc_params.RandGenStatusFilename);
      }
    }

    gettimeofday(&tend_cycle, NULL);

    double update_time = (double) 
        (tend_cycle.tv_sec - tstart_cycle.tv_sec)+
        (double)(tend_cycle.tv_usec - tstart_cycle.tv_usec)/1.0e6;

    mc_params.max_update_time = (update_time > mc_params.max_update_time)?
        update_time :mc_params.max_update_time;


    if(0==devinfo.myrank_world){
        printf("Tot time : %f sec (with measurements)\n", update_time);
        if(debug_settings.save_diagnostics == 1){
            FILE *foutfile = fopen(debug_settings.diagnostics_filename,"at");
            fprintf(foutfile,"TOTTIME  %f \n",update_time);
            fclose(foutfile);
        }

    }

  }


    if (GPSTATUS_FERMION_MEASURES == mc_params.next_gps){
        // fermionic stuff measures

        if(0 != mc_params.JarzynskiMode ){ // halfway measurements for Jarzynski

            bf_param new_backfield_parameters = backfield_parameters;

            // direct mode 
            if(1 == mc_params.JarzynskiMode)
                new_backfield_parameters.bz = backfield_parameters.bz + 
                    (double) (meas_wrap.id_iter+0.5)/mc_params.MaxConfIdIter;
            // reverse mode
            if(-1 == mc_params.JarzynskiMode)
                new_backfield_parameters.bz = backfield_parameters.bz -
                    (double) (meas_wrap.id_iter+0.5)/mc_params.MaxConfIdIter;


            if(0==devinfo.myrank_world){

                printf("JarzynskiMode, iteration %d/%d (%d max for this run) - MEASUREMENTS AT HALFWAY \n",
                             meas_wrap.id_iter,mc_params.MaxConfIdIter,mc_params.ntraj);
                printf("JarzynskiMode - current bz value : %f (HALFWAY)\n", new_backfield_parameters.bz);
            }

            init_all_u1_phases(new_backfield_parameters,fermions_parameters);
            #pragma acc update device(u1_back_phases[0:8*alloc_info.NDiffFlavs])
            #pragma acc update device(u1_back_phases_f[0:8*alloc_info.NDiffFlavs])

        }

        double avg_unitarity_deviation, max_unitarity_deviation;
        check_unitarity_device(conf_acc,&max_unitarity_deviation, &avg_unitarity_deviation);
        MPI_PRINTF1("Avg/Max unitarity deviation on device: %e / %e\n",avg_unitarity_deviation,max_unitarity_deviation);

        IF_PERIODIC_REPLICA(){
            struct timeval tf0, tf1;
            gettimeofday(&tf0, NULL);
            fermion_measures(conf_acc,fermions_parameters,
                                             &fm_par, md_parameters.residue_metro,
                                             md_parameters.max_cg_iterations,conf_id_iter,
                                             meas_wrap.plq/GL_SIZE/3.0/6.0,
                                             meas_wrap.rect/GL_SIZE/3.0/6.0/2.0);   

            gettimeofday(&tf1, NULL);

            double fermionMeasureTiming =
                (double) (tf1.tv_sec - tf0.tv_sec)+
                (double)(tf1.tv_usec - tf0.tv_usec)/1.0e6;

            if(debug_settings.save_diagnostics == 1){
                FILE *foutfile = 
                    fopen(debug_settings.diagnostics_filename,"at");

                if(conf_id_iter % fm_par.measEvery == 0 )
                    fprintf(foutfile,"FERMMEASTIME  %f \n",fermionMeasureTiming);
                fclose(foutfile);
            }
            
        } // closes if(0==rep->label[devinfo.replica_idx]) or just the scope if PAR_TEMP is not defined

#ifdef PAR_TEMP
        {
            int ridx_lab0 = get_index_of_pbc_replica(); // finds index corresponding to label=0
            MPI_Bcast((void*)&(mc_params.measures_done),1,MPI_INT,ridx_lab0,MPI_COMM_WORLD);
        }
#endif

        // save RNG status
        if(conf_id_iter%mc_params.storeconfinterval==0){
            char tempname[50];
            char serial[10];
            strcpy(tempname,mc_params.RandGenStatusFilename);
            sprintf(serial,".%05d",conf_id_iter);
            strcat(tempname,serial);
            MPI_PRINTF1("Storing rng status in %s.\n" , tempname);
            saverand_tofile(tempname);
        } 

        if(conf_id_iter%mc_params.saveconfinterval==0){
            if( debug_settings.SaveAllAtEnd){
                MPI_PRINTF1("Saving rng status in %s.\n", mc_params.RandGenStatusFilename);
                saverand_tofile(mc_params.RandGenStatusFilename);
            }
            else MPI_PRINTF0("WARNING, \'SaveAllAtEnd\'=0,NOT SAVING/OVERWRITING RNG STATUS.\n\n\n");
        }

    } // closes if (GPSTATUS_FERMION_MEASURES == mc_params.next_gps)
    
    // determining next thing to do
    if(0 == conf_id_iter % fm_par.measEvery && 0 != alloc_info.NDiffFlavs)
      mc_params.next_gps = GPSTATUS_FERMION_MEASURES;
    if(mc_params.measures_done == fm_par.SingleInvNVectors){
      mc_params.next_gps = GPSTATUS_UPDATE;
      mc_params.measures_done = 0;
    }

        
        update_times();

        // check update run condition and print times
        check_and_update_run_condition(&meas_wrap);

      } // while id_iter loop ends here             
  } // closes if (0 != mc_params.ntraj)

  free_meas_wrapper(&meas_wrap);
}


void read_or_generate_conf(){
#ifdef PAR_TEMP
    snprintf(r_utils->rep_str,20,"replica_%d",devinfo.replica_idx);
		strcat(mc_params.save_conf_name,r_utils->rep_str);
#endif
		
    if(debug_settings.do_norandom_test){
      if(!read_conf_wrapper(conf_acc,"conf_norndtest",&conf_id_iter,debug_settings.use_ildg)){
				MPI_PRINTF0("Stored Gauge Conf conf_norndtest Read : OK\n");
      }
      else{
				// cold start
				MPI_PRINTF0("COMPILED IN NORANDOM MODE. A CONFIGURATION FILE NAMED \"conf_norndtest\" MUST BE PRESENT\n");
				exit(1);
      }
    }else{
      if(!read_conf_wrapper(conf_acc,mc_params.save_conf_name,
														&conf_id_iter,debug_settings.use_ildg)){
				MPI_PRINTF1("Stored Gauge Conf \"%s\" Read : OK \n", mc_params.save_conf_name);
      }else{
				generate_Conf_cold(conf_acc,mc_params.eps_gen);
				MPI_PRINTF0("Cold Gauge Conf Generated : OK \n");
				conf_id_iter=0;
      }
    }


#ifdef PAR_TEMP
    if(conf_id_iter==0){
      // first label initialization
      for(int ri=0; ri<rep->replicas_total_number; ++ri)
        rep->label[ri]=ri;
      if(devinfo.myrank_world ==0){ // write first labeling (usual increasing order)
        r_utils->file_label=fopen(acc_info->file_label_name,"at");
        if(!r_utils->file_label){r_utils->file_label=fopen(acc_info->file_label_name,"wt");} // create label file

        label_print(rep, r_utils->file_label, conf_id_iter); // populate it
        fclose(r_utils->file_label);
      }
      if (0==devinfo.myrank_world) printf("%d/%d Defect initialization\n",devinfo.replica_idx,rep->replicas_total_number); 
    }else{ // not first iteration: initialize boundaries from label file
      if(devinfo.myrank_world ==0){ // read labeling from file
        r_utils->file_label=fopen(acc_info->file_label_name,"r");
        if(!r_utils->file_label){ 
          printf("\n\nERROR! Cannot open label file.\n\n");
#ifdef MULTIDEVICE
          MPI_Abort(MPI_COMM_WORLD,1);			
#else
          exit(EXIT_FAILURE);			
#endif
        }else{ // file exists
          // read label file
          int itr_num=-1,trash_bin;
          printf("conf_id_iter: %d\n",conf_id_iter);
          while(fscanf(r_utils->file_label,"%d",&itr_num)==1){
            printf("%d ",itr_num);
            for(int idx=0; idx<NREPLICAS; ++idx){
              fscanf(r_utils->file_label,"%d",(itr_num==conf_id_iter)? &(rep->label[idx]) : &trash_bin);
              printf("%d ",(itr_num==conf_id_iter)? (rep->label[idx]) : trash_bin);
            }
            printf("\n");
          }
          fclose(r_utils->file_label);
        }
      }
      // broadcast it to all replicas and ranks 
      MPI_Bcast((void*)&(rep->label[0]),NREPLICAS,MPI_INT,0,MPI_COMM_WORLD);
    }
		strcpy(mc_params.save_conf_name,r_utils->aux_name_file);
		
    init_k(conf_acc,rep->cr_vec[rep->label[devinfo.replica_idx]],rep->defect_boundary,rep->defect_coordinates,&def_info,0);
#if NRANKS_D3 > 1
    if(devinfo.async_comm_gauge) init_k(&conf_acc[8],rep->cr_vec[rep->label[devinfo.replica_idx]],rep->defect_boundary,rep->defect_coordinates,&def_info,1);
#endif
		#pragma acc update device(conf_acc[0:alloc_info.conf_acc_size])

		if(md_parameters.singlePrecMD){
			convert_double_to_float_su3_soa(conf_acc,conf_acc_f);
			//^^ NOTE: doing this because a K initialization for su3_soa_f doesn't exist. Please create it.
#if NRANKS_D3 > 1
			if(devinfo.async_comm_gauge){
				convert_double_to_float_su3_soa(&conf_acc[8],&conf_acc_f[8]);
				//^^ NOTE: doing this because a K initialization for su3_soa_f doesn't exist. Please create it.
			}
#endif
			
			#pragma acc update host(conf_acc_f[0:alloc_info.conf_acc_size])
		}
#else // no PAR_TEMP
		#pragma acc update device(conf_acc[0:alloc_info.conf_acc_size])
#endif
}



void print_compilation_banners(int correct_input_call){
  if(0==devinfo.myrank_world){
    printf("****************************************************\n");
		if (correct_input_call==0) printf("          COMPILATION INFO                        \n");
    if (correct_input_call==1) printf("          PRE INIT - READING SETTING  FILE          \n");
    if (correct_input_call==1) printf("     check which parameter corresponds to what! \n");
    printf("commit: %s\n", xstr(COMMIT_HASH) );
    printf("****************************************************\n");
  }
  
  if (ACTION_TYPE == TLSM ){
    if(0==devinfo.myrank_world) printf("\nCOMPILED WITH TREE-LEVEL SYMANZIK IMPROVED GAUGE ACTION\n\n");
  }else{ // ACTION_TYPE == WILSON
    if(0==devinfo.myrank_world) printf("COMPILED WITH WILSON GAUGE ACTION\n\n");
  }

#ifdef PAR_TEMP
  if(0==devinfo.myrank_world) printf("COMPILED FOR PARALLEL TEMPERING ON BOUNDARY CONDITIONS\n\n");
#else
  if(0==devinfo.myrank_world) printf("COMPILED WITHOUT PARALLEL TEMPERING (1 REPLICA RUN)\n\n");
#endif
}




void print_mode_banners(){
  if(0==devinfo.myrank_world){
    if(0 != mc_params.JarzynskiMode){
      printf("****************************************************\n");
      printf("                   JARZYNSKI MODE              \n");
      printf("     check which parameter corresponds to what! \n");
      printf("****************************************************\n");


    }else {
      printf("****************************************************\n");
      printf("                    NORMAL MODE                \n");
      printf("****************************************************\n");
    }
    if(debug_settings.do_norandom_test){
      printf("****************************************************\n");
      printf("         WELCOME. This is a NORANDOM test.    \n");
      printf("     MOST things will not be random generated,\n");
      printf("            but read from memory instead.     \n");
      printf("                  CHECK THE CODE!!            \n");
      printf("   ALSO: setting the number of trajectories to 1.\n");
      printf("****************************************************\n");
      mc_params.ntraj = 1;

    }
  }
}




void assign_device(){
#ifndef __GNUC__
  acc_device_t my_device_type = MY_DEVICE_TYPE;
  MPI_PRINTF0("Selecting device.\n");
#ifdef MULTIDEVICE
  select_init_acc_device(my_device_type, (devinfo.single_dev_choice + devinfo.myrank_world)%devinfo.proc_per_node);
#else
  select_init_acc_device(my_device_type, devinfo.single_dev_choice);
#endif // MULTIDEVICE
  printf("Device Selected : OK \n"); //checking printing.
#endif // __GNUC__
}



void allocate_memory(){
  mem_alloc_core();
  mem_alloc_extended();
 
  // single/double precision allocation
  MPI_PRINTF0("Memory allocation (double) : OK \n\n\n");
  if(inverter_tricks.useMixedPrecision || md_parameters.singlePrecMD){
    mem_alloc_core_f();
    MPI_PRINTF0("Memory allocation (float) [CORE]: OK \n\n\n");
  }

  if( md_parameters.singlePrecMD){
    mem_alloc_extended_f();
    MPI_PRINTF0("Memory allocation (float) [EXTENDED]: OK \n\n\n");
  }
   
  MPI_PRINTF1("Total allocated memory: %zu \n\n\n",max_memory_used);
}

#ifdef PAR_TEMP
void assign_replica_defects(defect_info *def){
  int vec_aux_bound[3]={1,1,1};

	if (0==devinfo.myrank_world) printf("Auxiliary confs defect initialization\n");
  init_k(   aux_conf_acc,1,0,vec_aux_bound,def,1);
  init_k(auxbis_conf_acc,1,0,vec_aux_bound,def,1);
	#pragma acc update device(aux_conf_acc[0:8])
	#pragma acc update device(auxbis_conf_acc[0:8])

	if(md_parameters.singlePrecMD){
		convert_double_to_float_su3_soa(   aux_conf_acc,   aux_conf_acc_f);
		convert_double_to_float_su3_soa(auxbis_conf_acc,auxbis_conf_acc_f);
		#pragma acc update host(aux_conf_acc_f[0:8])
		#pragma acc update host(auxbis_conf_acc_f[0:8])
	}

	if(alloc_info.stoutAllocations){
		int stout_steps = ((act_params.topo_stout_steps>act_params.stout_steps) & (act_params.topo_action==1)?
											  act_params.topo_stout_steps:act_params.stout_steps );
		for (int i = 0; i < stout_steps; i++)
			init_k(&gstout_conf_acc_arr[8*i],1,0,vec_aux_bound,def,1);
		#pragma acc update device(gstout_conf_acc_arr[0:8*stout_steps])
		if(md_parameters.singlePrecMD){
			for (int i = 0; i < stout_steps; i++)
				convert_double_to_float_su3_soa(&gstout_conf_acc_arr[8*i],&gstout_conf_acc_arr_f[8*i]);
			#pragma acc update host(gstout_conf_acc_arr_f[0:8*stout_steps])
		}
	}
}
#endif // def PAR_TEMP


void initial_setup(int argc, char* argv[]){
  gettimeofday ( &(mc_params.start_time), NULL );

  srand(time(NULL));
  if(argc!=2){
    if(0==devinfo.myrank_world) print_geom_defines();
    if(0==devinfo.myrank_world) printf("\n\nERROR! Use mpirun -n <num_tasks> %s input_file to execute the code!\n\n", argv[0]);
    exit(EXIT_FAILURE);			
  }
    
#ifdef MULTIDEVICE
	pre_init_multidev1D(&devinfo);
	gdbhook();
#else
	devinfo.replica_idx=0;
#endif
  
  int correct_input_call=(argc==2)? 1:0;
  print_compilation_banners(correct_input_call);

  // this allocates stuff, also rep and r_utils
  int input_file_read_check = set_global_vars_and_fermions_from_input_file(argv[1]);

#ifdef MULTIDEVICE
  if(input_file_read_check){
    MPI_PRINTF0("input file reading failed, Aborting...\n");
    MPI_Abort(MPI_COMM_WORLD,1);
  }else init_multidev1D(&devinfo);
#else
  devinfo.myrank = 0;
  devinfo.nranks = 1;
#endif

	if(input_file_read_check){
		MPI_PRINTF0("input file reading failed, aborting...\n");
		exit(1);
	}

	if(0==devinfo.myrank_world) print_geom_defines();
	verbosity_lv = debug_settings.input_vbl;

  print_mode_banners();

  if(verbosity_lv > 2) 
    MPI_PRINTF0("Input file read and initialized multidev1D...\n");

  assign_device();

#ifdef PAR_TEMP
  setup_replica_utils(mc_params.save_conf_name);
#endif

  // set RNG seed
  myseed_default =  (unsigned int) mc_params.seed; 
#ifdef MULTIDEVICE
  myseed_default =  (unsigned int) (myseed_default + devinfo.myrank_world) ;
  char myrank_string[6];
  sprintf(myrank_string,".R%d",devinfo.myrank_world);
  strcat(mc_params.RandGenStatusFilename,myrank_string);
#endif
  initrand_fromfile(mc_params.RandGenStatusFilename,myseed_default);

  // init ferm params and read rational approx coeffs
  if(init_ferm_params(fermions_parameters)){
    MPI_PRINTF0("Finalizing...\n"); //cp
#ifdef MULTIDEVICE
    MPI_Finalize();
#endif
    exit(1);
  }
	#pragma acc enter data copyin(fermions_parameters[0:alloc_info.NDiffFlavs])
    

  allocate_memory();
  
	gl_stout_rho=act_params.stout_rho;
	gl_topo_rho=act_params.topo_rho;
	#pragma acc enter data copyin(gl_stout_rho)
	#pragma acc enter data copyin(gl_topo_rho)

  compute_nnp_and_nnm_openacc();
	#pragma acc enter data copyin(nnp_openacc)
	#pragma acc enter data copyin(nnm_openacc)
  MPI_PRINTF0("nn computation : OK\n");
  init_all_u1_phases(backfield_parameters,fermions_parameters);
	#pragma acc update device(u1_back_phases[0:8*alloc_info.NDiffFlavs])
	#pragma acc update device(mag_obs_re[0:8*alloc_info.NDiffFlavs])
	#pragma acc update device(mag_obs_im[0:8*alloc_info.NDiffFlavs])

  if(inverter_tricks.useMixedPrecision || md_parameters.singlePrecMD){
		#pragma acc update device(u1_back_phases_f[0:8*alloc_info.NDiffFlavs])
  }
  MPI_PRINTF0("u1_backfield initialization (float & double): OK \n");

  initialize_md_global_variables(md_parameters);
  MPI_PRINTF0("init md vars : OK \n");

  read_or_generate_conf();

#ifdef PAR_TEMP
  assign_replica_defects(&def_info);
#endif 
	
  // check conf sanity
  double avg_unitarity_deviation,max_unitarity_deviation;
  check_unitarity_host(conf_acc,&max_unitarity_deviation, &avg_unitarity_deviation);
  MPI_PRINTF1("Avg/Max unitarity deviation on device: %e / %e\n",avg_unitarity_deviation,max_unitarity_deviation);
}




void save_conf_and_status(){
  // save gauge confs (with replicas if present)
#ifdef PAR_TEMP
    snprintf(r_utils->rep_str,20,"replica_%d",devinfo.replica_idx); // initialize rep_str
    strcat(mc_params.save_conf_name,r_utils->rep_str); // append rep_str
#endif
		
    if (debug_settings.SaveAllAtEnd){
      MPI_PRINTF1("Saving conf %s.\n", mc_params.save_conf_name);
      save_conf_wrapper(conf_acc,mc_params.save_conf_name, conf_id_iter, debug_settings.use_ildg);
    }else MPI_PRINTF0("WARNING, \'SaveAllAtEnd\'=0,NOT SAVING/OVERWRITING CONF AND RNG STATUS.\n\n\n");
#ifdef PAR_TEMP
    strcpy(mc_params.save_conf_name,r_utils->aux_name_file);
#endif

  // save RNG status
	if (debug_settings.SaveAllAtEnd){
		MPI_PRINTF1("Saving rng status in %s.\n", mc_params.RandGenStatusFilename);
  saverand_tofile(mc_params.RandGenStatusFilename);
	}


  // save program status
  if(0 == devinfo.myrank_world && debug_settings.SaveAllAtEnd){
    save_global_program_status(mc_params, glob_max_update_times,glob_max_flavour_cycle_times); // WARNING: this function in some cases does not work
  }

}

void free_all_allocations(){
  MPI_PRINTF0("Double precision free [CORE]\n");
  mem_free_core();
    
  MPI_PRINTF0("Double precision free [EXTENDED]\n");
  mem_free_extended();

  if(inverter_tricks.useMixedPrecision || md_parameters.singlePrecMD){
    MPI_PRINTF0("Single precision free [CORE]\n");
    mem_free_core_f();
  }
  if( md_parameters.singlePrecMD){
    MPI_PRINTF0("Signle precision free [EXTENDED]\n");
    mem_free_extended_f();
  }

#ifdef PAR_TEMP
  free_replica_utils();
#endif
	
  MPI_PRINTF0("freeing device nnp and nnm\n");
	#pragma acc exit data delete(nnp_openacc)
	#pragma acc exit data delete(nnm_openacc)
	#pragma acc exit data delete(gl_stout_rho)
	#pragma acc exit data delete(gl_topo_rho)

  MPI_PRINTF1("Allocated memory before the shutdown: %zu \n\n\n",memory_used);
  struct memory_allocated_t *all=memory_allocated_base;
    
  while(all!=NULL)
    {
      MPI_PRINTF1("To be deallocated: %s having size %zu (or maybe is to be counted)\n\n\n",all->varname,all->size);
      // free_wrapper(all->ptr);
      all=all->next;
    };
}


void cleanup(){
  save_conf_and_status();

  free_all_allocations();

#ifndef __GNUC__
  // OpenAcc context closing
  acc_device_t my_device_type = MY_DEVICE_TYPE;
  shutdown_acc_device(my_device_type);
#endif

#ifdef MULTIDEVICE
  shutdown_multidev();
#endif
    
  if(0==devinfo.myrank_world){printf("The End\n");}
}


int main(int argc, char* argv[]){

  initial_setup(argc,argv);

  main_loop();

  cleanup();

  return(EXIT_SUCCESS);
}
