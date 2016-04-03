#ifndef IPDOT_GAUGE_C
#define IPDOT_GAUGE_C

#include "struct_c_def.h"
#include "./su3_utilities.h"
#include "./su3_measurements.h"
#include "./plaquettes.h"
#include "./rettangoli.h"
#include "./ipdot_gauge.h"
#include "./geometry.h"
#include "../Include/debug.h"


#include "sys/time.h"
//#define TIMING_STAPLES

extern int verbosity_lv;

#ifdef DEBUG_MD
#include "../Mpi/multidev.h"
#include "../DbgTools/dbgtools.h"
int already_printed_staples_debug = 0;
#endif



extern tamat_soa * ipdot_g_old; // see alloc_vars.c

void calc_ipdot_gauge_soloopenacc_std( 
        __restrict  su3_soa * const tconf_acc, 
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero(local_staples);
    calc_loc_staples_nnptrick_all(tconf_acc,local_staples);
    conf_times_staples_ta_part(tconf_acc,local_staples,tipdot);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"staples_0_%s",devinfo.myrankstr);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,"tipdot_staples_0_%s",devinfo.myrankstr);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif


#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("FULL STAPLES CALC OPENACC                       PreKer->PostKer   : %f sec  \n",dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc_tlsm( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero(local_staples);
    calc_loc_staples_nnptrick_all(tconf_acc,local_staples);

    // QUESTA CHE FA TUTTO IN UNA BOTTA SEMBRA ANDARE PIU' PIANO
    //    calc_loc_improved_staples_typeABC_nnptrick_all(tconf_acc,local_staples);

    calc_loc_improved_staples_typeA_nnptrick_all(tconf_acc,local_staples);
    calc_loc_improved_staples_typeB_nnptrick_all(tconf_acc,local_staples);
    calc_loc_improved_staples_typeC_nnptrick_all(tconf_acc,local_staples);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"impr_staples_0_%s",devinfo.myrankstr);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,"tipdot_improved_staples_0_%s",devinfo.myrankstr);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif
    conf_times_staples_ta_part(tconf_acc,local_staples,tipdot);

#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("FULL STAPLES CALC OPENACC                       PreKer->PostKer   : %f sec  \n",dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{
    if(GAUGE_ACTION==0){
        calc_ipdot_gauge_soloopenacc_std(tconf_acc,local_staples,tipdot);
    }
    if(GAUGE_ACTION==1){
        calc_ipdot_gauge_soloopenacc_tlsm(tconf_acc,local_staples,tipdot);
    }

    if(debug_settings.save_diagnostics == 1){
        double  force_norm, diff_force_norm;
        force_norm = calc_force_norm(tipdot);
        diff_force_norm = calc_diff_force_norm(tipdot,ipdot_g_old);
        copy_ipdot_into_old(tipdot,ipdot_g_old);

        FILE *foutfile = 
            fopen(debug_settings.diagnostics_filename,"at");
        fprintf(foutfile,"GFHN %e \tDGFHN %e \t",force_norm,diff_force_norm);
        fclose(foutfile);



        if(verbosity_lv > 1)
            printf("\t\t\tGauge Force Half Norm: %e, Diff with previous: %e \n", 
                    force_norm, diff_force_norm);
    } 




}

#ifdef MULTIDEVICE
void calc_ipdot_gauge_soloopenacc_std_bulk( 
        __restrict  su3_soa * const tconf_acc, 
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero_bulk(local_staples);
    calc_loc_staples_nnptrick_all_bulk(tconf_acc,local_staples);
    conf_times_staples_ta_part_bulk(tconf_acc,local_staples,tipdot);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"staples_0_%s_bulk",devinfo.myrankstr);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,"tipdot_staples_0_%s_bulk",devinfo.myrankstr);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif


#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("BULK STAPLES CALC OPENACC PreKer->PostKer   : %f sec  \n",dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc_tlsm_bulk( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero_bulk(local_staples);
    calc_loc_staples_nnptrick_all_bulk(tconf_acc,local_staples);

    // QUESTA CHE FA TUTTO IN UNA BOTTA SEMBRA ANDARE PIU' PIANO
    //    calc_loc_improved_staples_typeABC_nnptrick_all(tconf_acc,local_staples);

    calc_loc_improved_staples_typeA_nnptrick_all_bulk(tconf_acc,local_staples);
    calc_loc_improved_staples_typeB_nnptrick_all_bulk(tconf_acc,local_staples);
    calc_loc_improved_staples_typeC_nnptrick_all_bulk(tconf_acc,local_staples);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"impr_staples_0_%s_bulk",
                devinfo.myrankstr);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,"tipdot_improved_staples_0_%s_bulk",
                devinfo.myrankstr);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif
    conf_times_staples_ta_part_bulk(tconf_acc,local_staples,tipdot);

#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("BULK STAPLES CALC OPENACC, PreKer->PostKer   : %f sec  \n",
            dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc_bulk( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot)
{
    if(GAUGE_ACTION==0){
        calc_ipdot_gauge_soloopenacc_std_bulk(tconf_acc,local_staples,tipdot);
    }
    if(GAUGE_ACTION==1){
        calc_ipdot_gauge_soloopenacc_tlsm_bulk(tconf_acc,local_staples,tipdot);
    }

}

void calc_ipdot_gauge_soloopenacc_std_d3c( 
        __restrict  su3_soa * const tconf_acc, 
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot,
        int offset3, int thickness3)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero_d3c(local_staples,offset3,thickness3);
    calc_loc_staples_nnptrick_all_d3c(tconf_acc,local_staples,
            offset3,thickness3);
    conf_times_staples_ta_part_d3c(tconf_acc,local_staples,tipdot,
            offset3,thickness3);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"staples_0_%s_d3c_%d_%d",
                devinfo.myrankstr,offset3,offset3+thickness3);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,"tipdot_staples_0_%s_d3c_%d_%d",
                devinfo.myrankstr,offset3,offset3+thickness3);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif


#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("D3C (%d-%d) STAPLES CALC OPENACC PreKer->PostKer :%f sec\n",
            offset3,offset3+thickness3,dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc_tlsm_d3c( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot,
        int offset3, int thickness3)
{

#ifdef TIMING_STAPLES
    struct timeval t1,t2;
    gettimeofday ( &t1, NULL );
#endif

    set_su3_soa_to_zero_d3c(local_staples,offset3,thickness3);
    calc_loc_staples_nnptrick_all_d3c(tconf_acc,local_staples,
            offset3,thickness3);

    // QUESTA CHE FA TUTTO IN UNA BOTTA SEMBRA ANDARE PIU' PIANO
    //    calc_loc_improved_staples_typeABC_nnptrick_all(tconf_acc,local_staples);

    calc_loc_improved_staples_typeA_nnptrick_all_d3c(tconf_acc,local_staples,offset3,thickness3);
    calc_loc_improved_staples_typeB_nnptrick_all_d3c(tconf_acc,local_staples,offset3,thickness3);
    calc_loc_improved_staples_typeC_nnptrick_all_d3c(tconf_acc,local_staples,offset3,thickness3);
#ifdef DEBUG_MD
    if(!already_printed_staples_debug){
        char genericfilename[50];
        sprintf(genericfilename,"impr_staples_0_%s_d3c_%d_%d",
                devinfo.myrankstr,offset3,offset3+thickness3);
        dbgprint_gl3_soa(local_staples,genericfilename,1000);
        sprintf(genericfilename,
                "tipdot_improved_staples_0_%s_d3c_%d_%d",
                devinfo.myrankstr,offset3,offset3+thickness3);
        print_tamat_soa(tipdot,genericfilename);
        already_printed_staples_debug = 1;
    }
#endif
    conf_times_staples_ta_part_d3c(tconf_acc,local_staples,tipdot,
            offset3,thickness3);

#ifdef TIMING_STAPLES
    gettimeofday ( &t2, NULL );
    double dt_preker_to_postker = (double)(t2.tv_sec - t1.tv_sec) + ((double)(t2.tv_usec - t1.tv_usec)/1.0e6);
    printf("D3C (%d-%d) STAPLES CALC OPENACC,PreKer->PostKer:%f sec\n",
            offset3,offset3+thickness3,dt_preker_to_postker);
#endif

}

void calc_ipdot_gauge_soloopenacc_d3c( 
        __restrict  su3_soa * const tconf_acc,  
        __restrict su3_soa * const local_staples,
        __restrict tamat_soa * const tipdot,
        int offset3, int thickness3)
{
    if(GAUGE_ACTION==0){
        calc_ipdot_gauge_soloopenacc_std_d3c(tconf_acc,
                local_staples,tipdot,
                offset3,thickness3);
    }
    if(GAUGE_ACTION==1){
        calc_ipdot_gauge_soloopenacc_tlsm_d3c(tconf_acc,
                local_staples,tipdot,
                offset3,thickness3);
    }

}



#endif



#endif
