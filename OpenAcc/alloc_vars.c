#ifndef ALLOC_DEF_
#define ALLOC_DEF_

double_soa * u1_back_field_phases;
tamat_soa * ipdot_acc;
su3_soa  * conf_acc_bkp; // the old stored conf that will be recovered if the metro test fails.
su3_soa  * aux_conf_acc; // auxiliary 
su3_soa  * auxbis_conf_acc; // auxiliary 
su3_soa  * stout_conf_acc;
vec3_soa * ferm_chi_acc; // questo e' il chi [NPS_tot]
//ACC_MultiFermion * ferm_chi_acc; // questo e' il chi
vec3_soa * ferm_phi_acc; // questo e' il phi [NPS_tot]
//ACC_MultiFermion * ferm_phi_acc; // questo e' il phi
vec3_soa * ferm_out_acc; // questo e' uno ausiliario [NPS_tot]
//ACC_MultiFermion * ferm_out_acc; // questo e' uno ausiliario
vec3_soa * ferm_shiftmulti_acc; // ausiliario per l'invertitore multishift [max_ps*max_approx_order]
//ACC_ShiftMultiFermion * ferm_shiftmulti_acc; // ausiliario per l'invertitore multishift [max_ps][max_approx_order]
vec3_soa * kloc_r;  // vettore ausiliario
vec3_soa * kloc_h;  // vettore ausiliario
vec3_soa * kloc_s;  // vettore ausiliario
vec3_soa * kloc_p;  // vettore ausiliario
vec3_soa * k_p_shiftferm; // ausiliario [max_nshift=max_approx_order]
//ACC_ShiftFermion *k_p_shiftferm;
thmat_soa * momenta;
dcomplex_soa * local_sums;
double_soa * d_local_sums;

void mem_alloc(){
  printf("Allocating resources for NPS_tot=%d pseudofermions in total, with max_approx_order=%d\n", NPS_tot, max_approx_order);
  int allocation_check;  
#ifdef BACKFIELD
  allocation_check =  posix_memalign((void **)&u1_back_field_phases, ALIGN, 8*sizeof(double_soa));   //  -->  4*size phases (as many as links)
  if(allocation_check != 0)  printf("Errore nella allocazione di u1_back_field_phases \n");
#else
  u1_back_field_phases=NULL;
#endif
  allocation_check =  posix_memalign((void **)&momenta, ALIGN, 8*sizeof(thmat_soa));   //  -->  4*size
  if(allocation_check != 0)  printf("Errore nella allocazione di momenta \n");
  allocation_check =  posix_memalign((void **)&kloc_r, ALIGN, sizeof(vec3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di kloc_r \n");
  allocation_check =  posix_memalign((void **)&kloc_h, ALIGN, sizeof(vec3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di kloc_h \n");
  allocation_check =  posix_memalign((void **)&kloc_s, ALIGN, sizeof(vec3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di kloc_s \n");
  allocation_check =  posix_memalign((void **)&kloc_p, ALIGN, sizeof(vec3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di kloc_p \n");
  allocation_check =  posix_memalign((void **)&k_p_shiftferm, ALIGN, max_approx_order* sizeof(vec3_soa));
  // allocation_check =  posix_memalign((void **)&k_p_shiftferm, ALIGN, sizeof(ACC_ShiftFermion));
  if(allocation_check != 0)  printf("Errore nella allocazione di k_p_shiftferm \n");
  allocation_check =  posix_memalign((void **)&aux_conf_acc, ALIGN, 8*sizeof(su3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di aux_conf_acc \n");
  allocation_check =  posix_memalign((void **)&auxbis_conf_acc, ALIGN, 8*sizeof(su3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di auxbis_conf_acc \n");
  allocation_check =  posix_memalign((void **)&stout_conf_acc, ALIGN, 8*sizeof(su3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di stout_conf_acc \n");
  allocation_check =  posix_memalign((void **)&conf_acc_bkp, ALIGN, 8*sizeof(su3_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di aux_conf_bkp \n");
  allocation_check =  posix_memalign((void **)&ipdot_acc, ALIGN, 8*sizeof(tamat_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di ipdot_acc \n");
  
  allocation_check =  posix_memalign((void **)&ferm_chi_acc  , ALIGN, NPS_tot * sizeof(vec3_soa));
  //allocation_check =  posix_memalign((void **)&ferm_chi_acc  , ALIGN, sizeof(ACC_MultiFermion));
  if(allocation_check != 0)  printf("Errore nella allocazione di ferm_chi_acc \n");

  allocation_check =  posix_memalign((void **)&ferm_phi_acc  , ALIGN, NPS_tot * sizeof(vec3_soa));
  //allocation_check =  posix_memalign((void **)&ferm_phi_acc  , ALIGN, sizeof(ACC_MultiFermion));
  if(allocation_check != 0)  printf("Errore nella allocazione di ferm_phi_acc \n");

   allocation_check =  posix_memalign((void **)&ferm_out_acc  , ALIGN, NPS_tot * sizeof(vec3_soa));
  //allocation_check =  posix_memalign((void **)&ferm_out_acc , ALIGN, sizeof(ACC_MultiFermion));
  if(allocation_check != 0)  printf("Errore nella allocazione di ferm_out_acc \n");

  allocation_check =  posix_memalign((void **)&ferm_shiftmulti_acc, ALIGN, max_ps*max_approx_order*sizeof(vec3_soa));
  
  //allocation_check =  posix_memalign((void **)&ferm_shiftmulti_acc , ALIGN, sizmof(ACC_ShiftMultiFermion));
  if(allocation_check != 0)  printf("Errore nella allocazione di ferm_shiftmulti_acc \n");

  allocation_check =  posix_memalign((void **)&d_local_sums, ALIGN, sizeof(double_soa));
  if(allocation_check != 0)  printf("Errore nella allocazione di d_local_sums \n");
  allocation_check =  posix_memalign((void **)&local_sums, ALIGN, 2*sizeof(dcomplex_soa));  // --> size complessi --> vettore per sommare cose locali
  if(allocation_check != 0)  printf("Errore nella allocazione di local_sums \n");
}

void mem_free(){
#ifdef BACKFIELD
  free(u1_back_field_phases);
#endif
  free(momenta);
  free(aux_conf_acc);
  free(stout_conf_acc);
  free(conf_acc_bkp);
  free(ipdot_acc);

  free(ferm_chi_acc);
  free(ferm_phi_acc);
  free(ferm_out_acc);

  free(ferm_shiftmulti_acc);

  free(kloc_r);
  free(kloc_s);
  free(kloc_h);
  free(kloc_p);
  free(k_p_shiftferm);

  free(local_sums);
  free(d_local_sums);
}

#endif
