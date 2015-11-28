
#ifndef INVERTER_FULL_C_
#define INVERTER_FULL_C_

#include "../Include/common_defines.h"
#include "./struct_c_def.h"
#include "./fermionic_utilities.h"
#include "./fermion_matrix.h"
#ifndef __GNUC__
 #include "openacc.h"
#endif
#include "./inverter_full.h"

//#define DEBUG_INVERTER_FULL_OPENACC


int ker_invert_openacc(   __restrict su3_soa * const u,  // non viene aggiornata mai qui dentro
			  double_soa * const backfield,
			  ferm_param *pars,
			  __restrict vec3_soa * const out,
			  __restrict vec3_soa * const in, // non viene aggiornato mai qui dentro
			  double res,
			  __restrict vec3_soa * const trialSolution, // non viene aggiornato mai qui dentro
			  __restrict vec3_soa * const loc_r,
			  __restrict vec3_soa * const loc_h,
			  __restrict vec3_soa * const loc_s,
			  __restrict vec3_soa * const loc_p
			  ){

  int cg;
  long int i;
  double delta, alpha, lambda, omega, gammag;
  d_complex aux_comp=0.0+0.0I;

  assign_in_to_out(trialSolution,out);
  acc_Doe(u,loc_h,out,pars,backfield);
  acc_Deo(u,loc_s,loc_h,pars,backfield);

  combine_in1xferm_mass_minus_in2(out,pars->ferm_mass*pars->ferm_mass,loc_s);
  combine_in1_minus_in2(in,loc_s,loc_r);
  assign_in_to_out(loc_r,loc_p);

  delta=l2norm2_global(loc_r);

  // loop over cg iterations
  cg=0;
  do {
    cg++;    
    // s=(M^dag M)p    alpha=(p,s)
    acc_Doe(u,loc_h,loc_p,pars,backfield);
    acc_Deo(u,loc_s,loc_h,pars,backfield);

    combine_in1xferm_mass_minus_in2(loc_p,pars->ferm_mass*pars->ferm_mass,loc_s);
    alpha = real_scal_prod_global(loc_p,loc_s);

    omega=delta/alpha;     
    // out+=omega*p  r-=omega*s
    // lambda=(r,r);

    combine_in1xfactor_plus_in2(loc_p,omega,out,out);
    combine_in1xfactor_plus_in2(loc_s,-omega,loc_r,loc_r);

    lambda = l2norm2_global(loc_r);
    gammag=lambda/delta;
    delta=lambda;
    // p=r+gammag*p
    combine_in1xfactor_plus_in2(loc_p,gammag,loc_r,loc_p);

  } while( (sqrt(lambda)>res) && cg<max_cg);

#if ((defined DEBUG_MODE) || (defined DEBUG_INVERTER_FULL_OPENACC))
  printf("Terminated invert after   %d    iterations [", cg);
  acc_Doe(u,loc_h,out,pars,backfield);
  acc_Deo(u,loc_s,loc_h,pars,backfield);
  double giustoono;
  combine_in1xferm_mass2_minus_in2_minus_in3(out,pars->ferm_mass*pars->ferm_mass,loc_s,in,loc_p);
  assign_in_to_out(loc_p,loc_h);
  giustoono = real_scal_prod_global(loc_h,loc_p);
  printf(" res/stop_res=  %e , stop_res=%e ]\n\n",sqrt(giustoono)/res,res);
#endif
  if(cg==max_cg)
    {
      printf("WARNING: maximum number of iterations reached in invert\n");
    }
  
 return cg;

}


#endif

