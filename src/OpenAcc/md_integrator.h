#ifndef MD_INTEGRATOR_H
#define MD_INTEGRATOR_H

#include "./struct_c_def.h"
#include "../Include/fermion_parameters.h"

// if using GCC, there are some problems with __restrict.
#ifdef __GNUC__
 #define __restrict
#endif

extern double deltas_Omelyan[7];// must be declared here to copy it in the device in the main


typedef struct md_param_t{

    int no_md;// number of MD steps
    int gauge_scale;   // Update fermions every gauge_scale gauge updates
    double t ;
    double residue_md;
    double residue_metro;
    double expected_max_eigenvalue;

} md_param; 

extern md_param md_parameters;

void initialize_md_global_variables(md_param );

void multistep_2MN_gauge(su3_soa *tconf_acc,su3_soa *local_staples,tamat_soa *tipdot,thmat_soa *tmomenta);

void multistep_2MN_SOLOOPENACC( tamat_soa * tipdot_acc,
				su3_soa  * tconf_acc,
#ifdef STOUT_FERMIONS
                su3_soa  * tstout_conf_acc_arr, // huge parking for stouting
#endif
                su3_soa  * tauxbis_conf_acc, 
				su3_soa  * taux_conf_acc,
				ferm_param * tfermions_parameters,// [nflavs]
				int tNDiffFlavs,
				vec3_soa * ferm_in_acc, //[NPS_tot], will be ferm_chi_acc
				vec3_soa * tferm_shiftmulti_acc,// parking variable [max_ps*max_approx_order]
				vec3_soa * tkloc_r, // parking
				vec3_soa * tkloc_h, // parking
				vec3_soa * tkloc_s, // parking
				vec3_soa * tkloc_p, // parking
				vec3_soa * tk_p_shiftferm, // parking, [max_nshift]
				thmat_soa * tmomenta,
				dcomplex_soa * local_sums,
				double res);



#endif

