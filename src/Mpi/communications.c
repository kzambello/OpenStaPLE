#ifndef COMMUNICATIONS_C_
#define COMMUNICATIONS_C_

#ifdef __GNUC__
#define _POSIX_C_SOURCE 200809L // not to have warning on posix memalign
#endif

#include <stdlib.h>
#include "../OpenAcc/geometry.h"
#include "./communications.h"
#include "../OpenAcc/struct_c_def.h"
#include "../OpenAcc/single_types.h"
#include "./multidev.h"

extern int verbosity_lv;

#ifdef MULTIDEVICE
#include "mpi.h"

#define ALIGN 128
#define ALLOCCHECK(control_int,var)  if(control_int != 0 ) \
    printf("\tError in  allocation of %s . \n", #var);\
    else if(verbosity_lv > 2) printf("\tAllocation of %s : OK , %p\n", #var, var );\

#define FREECHECK(var) if(verbosity_lv >2) \
    printf("\tFreed %s, %p ...", #var,var);\
    free(var); if(verbosity_lv > 2)  printf(" done.\n");

// fermions

// offset stands for the 
void sendrecv_vec3soa_borders_1Dcut(vec3_soa *lnh_fermion,
        int rankL, int rankR, int thickness)
{
   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 5) printf("MPI%02d - sendrecv_vec3soa_borders_1Dcut() \n",
           devinfo.myrank);

/*   // PREAMBLE
   // This function is written taking the following assumptions:
   // 1. The domain is divided only along one direction, which is 
   //  the 'slowest' (strong assumption);
   // 2. That direction is the T direction (a bit weaker assumption, 
   //  changing that requires a complete redefinition of the site 
   //  ordering, which can be, perhaps, easily done by redefining the
   //  various '*snum' functions in Geometry/geometry.cc, and by 
   //  writing a tool to 'transpose' the configurations which have
   //  been written in a 'standard' ordering.*/

   //must be done for the three components of the fermion.

  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2)*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH
  MPI_Status status;
#ifdef USE_MPI_CUDA_AWARE
#pragma acc host_data use_device(lnh_fermion)
  {
#endif
      d_complex *c[3] ;
      c[0] = lnh_fermion->c0;
      c[1] = lnh_fermion->c1;
      c[2] = lnh_fermion->c2;

      int ii;
      for(int ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[offset_size:slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,
                  (void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankR,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[(sizeh-offset_size):slab_sizeh])
#endif

          sendtag = ii+3;
          recvtag = ii+3;

#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[(sizeh-offset_size-slab_sizeh):slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,
                  (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[offset_size-slab_sizeh:slab_sizeh])
#endif
      }
#ifdef USE_MPI_CUDA_AWARE
  }
#endif
 }


void sendrecv_vec3soa_borders_1Dcut_hostonly(vec3_soa *lnh_fermion,
        int rankL, int rankR, int thickness)
{
   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 5) printf("MPI%02d - sendrecv_vec3soa_borders_1Dcut() \n",
           devinfo.myrank);


   //must be done for the three components of the fermion.

   // no. of 'fermion' point in each slab)
   int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2)*thickness;
   int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
   // NOTICE THERE IS LNH_NXH
   MPI_Status status;
   d_complex *c[3] ;
   c[0] = lnh_fermion->c0;
   c[1] = lnh_fermion->c1;
   c[2] = lnh_fermion->c2;

   int ii;
   for(int ii =0; ii<3; ii++){

       // ASK FOR BACKS FIRST, THEN FACES
       int sendtag = ii;
       int recvtag = ii;

       d_complex *tmpc = c[ii];
       MPI_Sendrecv((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
               rankL,sendtag,
               (void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,MPI_DOUBLE,
               rankR,recvtag,
               MPI_COMM_WORLD, &status);

       sendtag = ii+3;
       recvtag = ii+3;

       MPI_Sendrecv((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
               2*slab_sizeh,MPI_DOUBLE,
               rankR,sendtag,
               (void*) &(c[ii][offset_size-slab_sizeh]),
               2*slab_sizeh,MPI_DOUBLE,
               rankL,recvtag,
               MPI_COMM_WORLD, &status);
   }

}

void communicate_fermion_borders(vec3_soa *lnh_fermion) // WRAPPER
{

    // NOTICE: GEOMETRY MUST BE SET UP BEFORE!!
    MPI_Barrier(MPI_COMM_WORLD);
    sendrecv_vec3soa_borders_1Dcut(lnh_fermion,
            devinfo.myrank_L, devinfo.myrank_R, 
            FERMION_HALO);
    MPI_Barrier(MPI_COMM_WORLD);
}



void communicate_fermion_borders_hostonly(vec3_soa *lnh_fermion) // WRAPPER
{

    // NOTICE: GEOMETRY MUST BE SET UP BEFORE!!
    MPI_Barrier(MPI_COMM_WORLD);

    sendrecv_vec3soa_borders_1Dcut_hostonly(lnh_fermion,
            devinfo.myrank_L, devinfo.myrank_R, 
            FERMION_HALO);
    MPI_Barrier(MPI_COMM_WORLD);
}



#if defined(USE_MPI_CUDA_AWARE) || defined(__GNUC__)
/*
 This can work only if cuda aware MPI is used.
 If instead you wanted to use the update
 directive, this cannot work with MPI_Irecv
 because the directive should wait for the 
 completion the recv, but Irecv is not bloking!!
*/                        
void sendrecv_vec3soa_borders_1Dcut_async(vec3_soa *lnh_fermion, 
        int rankL, int rankR, 
        int thickness,
        MPI_Request* send_border_requests, 
        MPI_Request* recv_border_requests)
{
   if(verbosity_lv > 5) printf("MPI%02d - sendrecv_vec3soa_borders_1Dcut_async() \n",
           devinfo.myrank);

   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
   // NOTICE send_border_requests recv_border_requests are both 
   // 6-elements long
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");

   //SEE PREAMBLE FOR sendrecv_vec3soa_borders_1Dcut()
   //must be done for the three components of the fermion.

  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2 )*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH

#pragma acc host_data use_device(lnh_fermion)
  {
      d_complex *c[3] ;
      c[0] = lnh_fermion->c0;
      c[1] = lnh_fermion->c1;
      c[2] = lnh_fermion->c2;

      int ii;
      for(int ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
          MPI_Isend((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[ii]));
          MPI_Irecv((void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,
                  MPI_DOUBLE,
                  rankR,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[ii]));


          sendtag = 3+ii;
          recvtag = 3+ii;

          MPI_Isend((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[3+ii]));
          MPI_Irecv( (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[3+ii]));

      }
  }
}

void communicate_fermion_borders_async(vec3_soa *lnh_fermion, 
        MPI_Request* send_border_requests, 
        MPI_Request* recv_border_requests){ //WRAPPER
   
    // NOTICE: GEOMETRY MUST BE SET UP BEFORE!!
    // NOTICE send_border_requests recv_border_requests are both 
    // 3*2-elements long
    // 3 colours
    // 2 ends
    sendrecv_vec3soa_borders_1Dcut_async(lnh_fermion,
            devinfo.myrank_L, devinfo.myrank_R,
            FERMION_HALO,
            send_border_requests,
            recv_border_requests );
}

void communicate_su3_borders_async(su3_soa* lnh_conf, int thickness,
        MPI_Request* send_border_requests, 
        MPI_Request* recv_border_requests)
{

    // NOTICE send_border_requests recv_border_requests are both 
    // 3*2*8*2 = 96 -elements long
    // 3 colours (columns of the matrix)
    // 2 ends
    // 8 = 2 even/odd * 4 link directions
    // 2 rows of the matrix

    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_vec3soa_borders_1Dcut_async(&(lnh_conf[c].r0),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness,
            &send_border_requests[3*2*c],
            &recv_border_requests[3*2*c]);
    
        sendrecv_vec3soa_borders_1Dcut_async(&(lnh_conf[c].r1),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness,
            &send_border_requests[48+3*2*c],
            &recv_border_requests[48+3*2*c]);
    
    }
}

#endif //#ifdef USE_MPI_CUDA_AWARE, or gcc


// GAUGE COMMS

// borders
void communicate_su3_borders(su3_soa* lnh_conf, int thickness)
{

    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_vec3soa_borders_1Dcut(&(lnh_conf[c].r0),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
        sendrecv_vec3soa_borders_1Dcut(&(lnh_conf[c].r1),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
    }

}
void communicate_su3_borders_hostonly(su3_soa* lnh_conf, int thickness)
{

    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_vec3soa_borders_1Dcut_hostonly(&(lnh_conf[c].r0),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
        sendrecv_vec3soa_borders_1Dcut_hostonly(&(lnh_conf[c].r1),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
    }

}

void communicate_gl3_borders(su3_soa* lnh_conf,int thickness)
{

    // NOTICE: GEOMETRY MUST BE SET UP BEFORE!!
    // note for async:
    // NOTICE send_border_requests recv_border_requests are both 
    // 12*8-elements long

    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_vec3soa_borders_1Dcut(&(lnh_conf[c].r0),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
        sendrecv_vec3soa_borders_1Dcut(&(lnh_conf[c].r1),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);
        sendrecv_vec3soa_borders_1Dcut(&(lnh_conf[c].r2),
            devinfo.myrank_L, devinfo.myrank_R,
            thickness);

    }

}



void sendrecv_thmat_soa_borders_1Dcut(thmat_soa *lnh_momenta,
        int rankL, int rankR, int thickness)
{

   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 4) printf("MPI%02d - sendrecv_thmat_soa_borders_1Dcut() \n",
           devinfo.myrank);

  // PREAMBLE : see  void sendrecv_vec3soa_borders_1Dcut()
  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2)*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH
  MPI_Status status;
#ifdef USE_MPI_CUDA_AWARE
#pragma acc host_data use_device(lnh_momenta)
  {
#endif
      d_complex *c[3] ;
      c[0] = lnh_momenta->c01;
      c[1] = lnh_momenta->c12;
      c[2] = lnh_momenta->c02;
      double  * cd[2];
      cd[0] = lnh_momenta->rc00;
      cd[1] = lnh_momenta->rc11;


      int ii;
      for(ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[offset_size:slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,
                  (void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankR,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[(sizeh-offset_size):slab_sizeh])
#endif

          sendtag = ii+3;
          recvtag = ii+3;

#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[(sizeh-offset_size-slab_sizeh):slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,
                  (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[offset_size-slab_sizeh:slab_sizeh])
#endif
      }


      for(ii =0; ii<2; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = 6+ii;
          int recvtag = 6+ii;

          double *tmpc = cd[ii];
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[offset_size:slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(cd[ii][offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,
                  (void*) &(cd[ii][sizeh-offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankR,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[(sizeh-offset_size):slab_sizeh])
#endif

          sendtag = 6+ii+2;
          recvtag = 6+ii+2;

#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[(sizeh-offset_size-slab_sizeh):slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(cd[ii][sizeh-offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,
                  (void*) &(cd[ii][offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[offset_size-slab_sizeh:slab_sizeh])
#endif

      }
#ifdef USE_MPI_CUDA_AWARE
  }
#endif
 }
void sendrecv_thmat_soa_borders_1Dcut_async(thmat_soa *lnh_momenta, 
        int rankL, int rankR, int thickness,
        MPI_Request* send_border_requests, 
        MPI_Request* recv_border_requests)
{
   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
   // NOTICE send_border_requests recv_border_requests are both 
   // 10-elements long
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 4) printf("MPI%02d - sendrecv_thmat_soa_borders_1Dcut_async()\n",
           devinfo.myrank);


   //SEE PREAMBLE FOR sendrecv_vec3soa_borders_1Dcut()
   //must be done for the three components of the fermion.

  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2 )*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH

#pragma acc host_data use_device(lnh_momenta)
  {
      d_complex *c[3] ;
      c[0] = lnh_momenta->c01;
      c[1] = lnh_momenta->c12;
      c[2] = lnh_momenta->c02;
      double  * cd[2];
      cd[0] = lnh_momenta->rc00;
      cd[1] = lnh_momenta->rc11;

      int ii;
      for(int ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
          MPI_Isend((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[ii]));
          MPI_Irecv((void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,
                  MPI_DOUBLE,
                  rankR,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[ii]));


          sendtag = 3+ii;
          recvtag = 3+ii;

          MPI_Isend((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[3+ii]));
          MPI_Irecv( (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[3+ii]));

      }
      for(int ii =0; ii<2; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = 6+ii;
          int recvtag = 6+ii;

          double *tmpc = cd[ii];
          MPI_Isend((void*) &(cd[ii][offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[6+ii]));
          MPI_Irecv((void*) &(cd[ii][sizeh-offset_size]),slab_sizeh,
                  MPI_DOUBLE,
                  rankR,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[6+ii]));


          sendtag = 8+ii;
          recvtag = 8+ii;

          MPI_Isend((void*) &(cd[ii][sizeh-offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[8+ii]));
          MPI_Irecv( (void*) &(cd[ii][offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[8+ii]));

      }
  }
}



void sendrecv_tamat_soa_borders_1Dcut(tamat_soa *lnh_ipdot,
        int rankL, int rankR, int thickness)
{
   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 4) printf("MPI%02d - sendrecv_tamat_soa_borders_1Dcut() \n",
           devinfo.myrank);


  // PREAMBLE : see  void sendrecv_vec3soa_borders_1Dcut()
  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2)*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH
  MPI_Status status;
#ifdef USE_MPI_CUDA_AWARE
#pragma acc host_data use_device(lnh_ipdot)
  {
#endif
      d_complex *c[3] ;
      c[0] = lnh_ipdot->c01;
      c[1] = lnh_ipdot->c12;
      c[2] = lnh_ipdot->c02;
      double  * cd[2];
      cd[0] = lnh_ipdot->ic00;
      cd[1] = lnh_ipdot->ic11;

      int ii;
      for(int ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[offset_size:slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,
                  (void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankR,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[(sizeh-offset_size):slab_sizeh])
#endif

          sendtag = ii+3;
          recvtag = ii+3;

#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[(sizeh-offset_size-slab_sizeh):slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,
                  (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[offset_size-slab_sizeh:slab_sizeh])
#endif
      }
      for(int ii =0; ii<2; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = 6+ii;
          int recvtag = 6+ii;

          double *tmpc = cd[ii];
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[offset_size:slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(cd[ii][offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,
                  (void*) &(cd[ii][sizeh-offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankR,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[(sizeh-offset_size):slab_sizeh])
#endif

          sendtag = 6+ii+2;
          recvtag = 6+ii+2;

#ifndef USE_MPI_CUDA_AWARE
#pragma acc update self(tmpc[(sizeh-offset_size-slab_sizeh):slab_sizeh])
#endif
          MPI_Sendrecv((void*) &(cd[ii][sizeh-offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,
                  (void*) &(cd[ii][offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,
                  MPI_COMM_WORLD, &status);
#ifndef USE_MPI_CUDA_AWARE
#pragma acc update device(tmpc[offset_size-slab_sizeh:slab_sizeh])
#endif
      }
#ifdef USE_MPI_CUDA_AWARE
  }
#endif
}
void sendrecv_tamat_soa_borders_1Dcut_async(tamat_soa *lnh_ipdot, 
        int rankL, int rankR, int thickness,
        MPI_Request* send_border_requests, 
        MPI_Request* recv_border_requests)
{
   // NOTICE YOU HAVE TO SET MYRANK CORRECTLY TO USE THIS FUNCTION
   // NOTICE send_border_requests recv_border_requests are both 
   // 10-elements long
  
  if(NRANKS_D0 != 1 || NRANKS_D1 != 1 || NRANKS_D2 != 1)
     printf("THIS SETUP IS NOT SALAMINO-LIKE!!!\n communication of fermion borders will FAIL!!\n");
   if(verbosity_lv > 4) printf("MPI%02d - sendrecv_tamat_soa_borders_1Dcut_async() \n",
           devinfo.myrank);


   //SEE PREAMBLE FOR sendrecv_vec3soa_borders_1Dcut()
   //must be done for the three components of the fermion.

  // no. of 'fermion' point in each slab)
  int slab_sizeh = (LNH_N0H * LNH_N1 * LNH_N2 )*thickness;
  int offset_size =  (LNH_N0H * LNH_N1 * LNH_N2) * HALO_WIDTH;
  // NOTICE THERE IS LNH_NXH

#pragma acc host_data use_device(lnh_ipdot)
  {
      d_complex *c[3] ;
      c[0] = lnh_ipdot->c01;
      c[1] = lnh_ipdot->c12;
      c[2] = lnh_ipdot->c02;
      double  * cd[2];
      cd[0] = lnh_ipdot->ic00;
      cd[1] = lnh_ipdot->ic11;

      int ii;
      for(int ii =0; ii<3; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = ii;
          int recvtag = ii;

          d_complex *tmpc = c[ii];
          MPI_Isend((void*) &(c[ii][offset_size]),2*slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[ii]));
          MPI_Irecv((void*) &(c[ii][sizeh-offset_size]),2*slab_sizeh,
                  MPI_DOUBLE,
                  rankR,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[ii]));


          sendtag = 3+ii;
          recvtag = 3+ii;

          MPI_Isend((void*) &(c[ii][sizeh-offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[3+ii]));
          MPI_Irecv( (void*) &(c[ii][offset_size-slab_sizeh]),
                  2*slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[3+ii]));

      }
      for(int ii =0; ii<2; ii++){

          // ASK FOR BACKS FIRST, THEN FACES
          int sendtag = 6+ii;
          int recvtag = 6+ii;

          double *tmpc = cd[ii];
          MPI_Isend((void*) &(cd[ii][offset_size]),slab_sizeh,MPI_DOUBLE,
                  rankL,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[6+ii]));
          MPI_Irecv((void*) &(cd[ii][sizeh-offset_size]),slab_sizeh,
                  MPI_DOUBLE,
                  rankR,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[6+ii]));


          sendtag = 8+ii;
          recvtag = 8+ii;

          MPI_Isend((void*) &(cd[ii][sizeh-offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankR,sendtag,MPI_COMM_WORLD,
                  &(send_border_requests[8+ii]));
          MPI_Irecv( (void*) &(cd[ii][offset_size-slab_sizeh]),
                  slab_sizeh,MPI_DOUBLE,
                  rankL,recvtag,MPI_COMM_WORLD,
                  &(recv_border_requests[8+ii]));

      }
  }
}

// used just at the beginning of MD trajectory (only GAUGE_HALO thick)
void communicate_thmat_soa_borders(thmat_soa* lnh_momenta,int thickness)
{
    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_thmat_soa_borders_1Dcut(&(lnh_momenta[c]),
            devinfo.myrank_L, devinfo.myrank_R,thickness );
    }
}
// force communication (only GAUGE_HALO thick)
void communicate_tamat_soa_borders(tamat_soa* lnh_ipdot,int thickness)
{


    for(int c = 0 ; c < 8 ; c++){ // Remember lnh_conf has 8 components
        sendrecv_tamat_soa_borders_1Dcut(&(lnh_ipdot[c]),
            devinfo.myrank_L, devinfo.myrank_R, thickness);
    }
}


// chunks, conf
void send_lnh_subconf_to_rank(global_su3_soa *gl_soa_conf, 
        int target_rank)
{
    // USE ONLY FROM MASTER RANK
    //target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);
    // building sublattice duplicate, target_conf

    int allocation_check;  
    su3_soa* target_su3_soa;
    allocation_check = posix_memalign((void**) &target_su3_soa, ALIGN,
            8*sizeof(su3_soa)); 
    ALLOCCHECK(allocation_check, target_su3_soa);

   if(verbosity_lv > 3) printf("MPI%02d - send_lnh_subconf_to_rank()\n",
           devinfo.myrank);

    int tg_lnh_0,tg_lnh_1,tg_lnh_2,tg_lnh_3,dir; //target-lnh coordinates
    // and link direction
    // Copying all relevant links into the sublattice
    for(dir =0; dir < 4; dir++)
        for(tg_lnh_3=0;tg_lnh_3 < LNH_N3; tg_lnh_3++)
        for(tg_lnh_2=0;tg_lnh_2 < LNH_N2; tg_lnh_2++)
        for(tg_lnh_1=0;tg_lnh_1 < LNH_N1; tg_lnh_1++)
        for(tg_lnh_0=0;tg_lnh_0 < LNH_N0; tg_lnh_0++){

                        //        int gtsp; // global target site parity
                        int tsprlo ; // target site parity respect (to his) local origin;

                        int target_gl_snum = target_lnh_to_gl_snum(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3, target_gl_loc_origin_from_rank);
                        int target_lnh_snum = snum_acc(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3);

                        tsprlo = (D0_HALO+D1_HALO+D2_HALO+D3_HALO+ tg_lnh_3+tg_lnh_2+tg_lnh_1+tg_lnh_0)%2;
												//      gtsp = (target_loc_origin_parity + tsprlo )%2;

                        //      printf("%d %d  %d  %d  %d  %d  %d ",dir, tg_lnh_t, tg_lnh_z, tg_lnh_y, tg_lnh_x, target_gl_snum, tsprlo);
                        single_su3 aux;
                        single_su3_from_global_su3_soa(&(gl_soa_conf[2*dir+tsprlo]),
                                target_gl_snum,&aux);
                        //    printf("ciao \n");
                        //        printf("ciao ");
                        //      print_su3(aux);
                        single_su3_into_su3_soa(&(target_su3_soa[2*dir+tsprlo]),
                                target_lnh_snum, &aux);
                        //        printf("ciao \n");

                    }



    //sending the subconfiguration
    //
    // OLD LINE
    //MPI_Send(target_su3_soa, 2*4*(6*3)*LNH_SIZEH,MPI_DOUBLE, target_rank , target_rank, MPI_COMM_WORLD); // tag = target_rank
    
		MPI_Send(target_su3_soa, 8*sizeof(su3_soa), MPI_CHAR, target_rank, target_rank, MPI_COMM_WORLD); // tag = target_rank

		// In case we remove the third line, this is possibly the thing to do
    //MPI_Send(target_su3_soa, 2*4*(6*2)*LNH_SIZEH,MPI_DOUBLE, rank, 0, MPI_COMM_WORLD);
    //Or maybe do multiple sends in a more complicated way
    // ^^ CHECK
    FREECHECK(target_su3_soa);
}
void recv_loc_subconf_from_rank(global_su3_soa *gl_soa_conf, 
        int target_rank, int tag)
{
    // USE ONLY FROM MASTER RANK
    //target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);
    /*
       int target_loc_origin_parity = (target_gl_loc_origin_from_rank.x +
       target_gl_loc_origin_from_rank.y +
       target_gl_loc_origin_from_rank.z +
       target_gl_loc_origin_from_rank.t)%2;// with the current setup, this should always be 0

       if(target_loc_origin_parity) printf("Problems\n");
       */
    // building sublattice duplicate, target_conf

   if(verbosity_lv > 3) printf("MPI%02d - recv_loc_subconf_from_rank(), tag %d\n",
           devinfo.myrank,tag);

    int allocation_check; 
    su3_soa* target_su3_soa;
    allocation_check = posix_memalign((void**) &target_su3_soa, ALIGN,
            8*sizeof(su3_soa)); 
    ALLOCCHECK(allocation_check, target_su3_soa);

    // OLD LINE
		//MPI_Recv(target_su3_soa, 2*4*(6*3)*LNH_SIZEH,MPI_DOUBLE,target_rank,tag,MPI_COMM_WORLD, MPI_STATUS_IGNORE);


		MPI_Recv(target_su3_soa, 8*sizeof(su3_soa), MPI_CHAR, target_rank,tag,MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    recv_loc_subconf_from_buffer(gl_soa_conf,target_su3_soa,target_rank);

    FREECHECK(target_su3_soa);
}
void send_lnh_subconf_to_master(su3_soa *lnh_soa_conf, int tag)
{
   //sending the subconfiguration
      if(verbosity_lv > 3) printf("MPI%02d - send_lnh_subconf_to_master(), tag %d\n",
           devinfo.myrank,tag);
		// OLD LINE
    //MPI_Send(lnh_soa_conf, 2*4*(6*3)*LNH_SIZEH,MPI_DOUBLE, 0, tag , MPI_COMM_WORLD);
		MPI_Send(lnh_soa_conf, 8*sizeof(su3_soa), MPI_CHAR, 0, tag, MPI_COMM_WORLD);
}
void receive_lnh_subconf_from_master(su3_soa* lnh_su3_conf)
{
		// OLD LINE
    //MPI_Recv(lnh_su3_conf, 2*4*(6*3)*LNH_SIZEH,MPI_DOUBLE,0, devinfo.myrank,MPI_COMM_WORLD, MPI_STATUS_IGNORE); // tag = myrank

    MPI_Recv(lnh_su3_conf, 8*sizeof(su3_soa), MPI_CHAR, 0, devinfo.myrank,MPI_COMM_WORLD, MPI_STATUS_IGNORE); // tag = myrank

    // In case we remove the third line possibly 
    // we have to do something different
    // ^^ CHECK
}

// chunks, fermions
void send_lnh_subfermion_to_rank(global_vec3_soa *gl_soa_ferm, 
        int target_rank)
{
    vec3_soa* lnh_ferm;
    int allocation_check = posix_memalign((void**) &lnh_ferm, ALIGN,
            sizeof(vec3_soa)); 
    ALLOCCHECK(allocation_check, lnh_ferm);
    send_lnh_subfermion_to_buffer(gl_soa_ferm,lnh_ferm,target_rank);
    MPI_Send(lnh_ferm, 6*LNH_SIZEH,MPI_DOUBLE, target_rank , target_rank, MPI_COMM_WORLD); // tag = target_rank

    FREECHECK(lnh_ferm);
}
void recv_loc_subfermion_from_rank(global_vec3_soa *gl_soa_ferm,
        int target_rank)
{
    vec3_soa* lnh_ferm;
    int allocation_check = posix_memalign((void**) &lnh_ferm, ALIGN,
            sizeof(vec3_soa)); 
    ALLOCCHECK(allocation_check, lnh_ferm);

    MPI_Recv(lnh_ferm, 6*LNH_SIZEH,MPI_DOUBLE,target_rank,target_rank,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    recv_loc_subfermion_from_buffer(gl_soa_ferm, lnh_ferm, target_rank);


    FREECHECK(lnh_ferm);

}

void send_lnh_subfermion_to_master(vec3_soa *lnh_ferm, int tag)
{
    MPI_Send(lnh_ferm, 6*LNH_SIZEH,MPI_DOUBLE,0,tag           ,
            MPI_COMM_WORLD); // tag = myrank
}
void receive_lnh_subfermion_from_master(vec3_soa* lnh_ferm)
{
    MPI_Recv(lnh_ferm, 6*LNH_SIZEH,MPI_DOUBLE,0,devinfo.myrank,
            MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

#endif
// only for the master rank
void send_lnh_subconf_to_buffer(global_su3_soa *gl_soa_conf, 
        su3_soa *lnh_conf, int target_rank)
{
    // USE ONLY FROM MASTER RANK

if(verbosity_lv > 3) printf("MPI%02d -send_lnh_subconf_to_buffer()\n", devinfo.myrank);


    //target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);
    // building sublattice duplicate, target_conf
    su3_soa* target_su3_soa = lnh_conf;

    int tg_lnh_0,tg_lnh_1,tg_lnh_2,tg_lnh_3,dir; //target-lnh coordinates
    // and link direction
    // Copying all relevant links into the sublattice
    for(dir =0; dir < 4; dir++)
        for(tg_lnh_3=0;tg_lnh_3 < LNH_N3; tg_lnh_3++)
        for(tg_lnh_2=0;tg_lnh_2 < LNH_N2; tg_lnh_2++)
        for(tg_lnh_1=0;tg_lnh_1 < LNH_N1; tg_lnh_1++)
        for(tg_lnh_0=0;tg_lnh_0 < LNH_N0; tg_lnh_0++){

                        //        int gtsp; // global target site parity
                        int tsprlo ; // target site parity respect (to his) local origin;

                        int target_gl_snum = target_lnh_to_gl_snum(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3, target_gl_loc_origin_from_rank);
                        int target_lnh_snum = snum_acc(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3);

                        tsprlo = (D0_HALO+D1_HALO+D2_HALO+D3_HALO+ tg_lnh_3+tg_lnh_2+tg_lnh_1+tg_lnh_0)%2;
												//      gtsp = (target_loc_origin_parity + tsprlo )%2;

                        //      printf("%d %d  %d  %d  %d  %d  %d ",dir, tg_lnh_t, tg_lnh_z, tg_lnh_y, tg_lnh_x, target_gl_snum, tsprlo);
                        single_su3 aux;
                        single_su3_from_global_su3_soa(&(gl_soa_conf[2*dir+tsprlo]),
                                target_gl_snum,&aux);
                        //    printf("ciao \n");
                        //        printf("ciao ");
                        //      print_su3(aux);
                        single_su3_into_su3_soa(&(target_su3_soa[2*dir+tsprlo]),
                                target_lnh_snum, &aux);
                        //        printf("ciao \n");

                    }

}
void recv_loc_subconf_from_buffer(global_su3_soa *gl_soa_conf, 
        su3_soa* lnh_conf, int target_rank)
{


if(verbosity_lv > 3) printf("MPI%02d - recv_loc_subconf_from_buffer()", devinfo.myrank);
    // USE ONLY FROM MASTER RANK
    //target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);
    // building sublattice duplicate, target_conf
    su3_soa* target_su3_soa = lnh_conf; 

    int tg_loc_0,tg_loc_1,tg_loc_2,tg_loc_3,dir; //target-loc coordinates
    // and link direction
    // Copying all relevant links from the sublattice to the global lattice
    for(dir =0; dir < 4; dir++)
        for(tg_loc_3=0;tg_loc_3<LOC_N3; tg_loc_3++)
        for(tg_loc_2=0;tg_loc_2<LOC_N2; tg_loc_2++)
        for(tg_loc_1=0;tg_loc_1<LOC_N1; tg_loc_1++)
        for(tg_loc_0=0;tg_loc_0<LOC_N0; tg_loc_0++){
    
                        int tg_lnh_0 = tg_loc_0 + D0_HALO;
                        int tg_lnh_1 = tg_loc_1 + D1_HALO;
                        int tg_lnh_2 = tg_loc_2 + D2_HALO;
                        int tg_lnh_3 = tg_loc_3 + D3_HALO;

                        //        int gtsp; // global target site parity
                        int tsprlo ; // target site parity respect (to his) local origin;

                        int target_gl_snum = target_lnh_to_gl_snum(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3, target_gl_loc_origin_from_rank);
                        int target_lnh_snum = snum_acc(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3);

                        tsprlo = (D0_HALO+D1_HALO+D2_HALO+D3_HALO+ tg_lnh_3+tg_lnh_2+tg_lnh_1+tg_lnh_0)%2;

                        single_su3 aux; 
                        single_su3_from_su3_soa(&(lnh_conf[2*dir+tsprlo]),
                                target_lnh_snum, &aux);
                        single_su3_into_global_su3_soa(&(gl_soa_conf[2*dir+tsprlo]),
                                target_gl_snum, &aux);

                    }

}

void send_lnh_subfermion_to_buffer(global_vec3_soa *gl_soa_ferm,
        vec3_soa *lnh_ferm, int target_rank)
{
    // USE ONLY FROM MASTER RANK

if(verbosity_lv > 3) printf("MPI%02d -send_lnh_subferm_to_buffer()", devinfo.myrank);
    
//target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);
    // building sublattice duplicate, target_ferm

    int tg_lnh_0h,tg_lnh_1,tg_lnh_2,tg_lnh_3; //target-lnh coordinates
    // Copying all relevant sites into the sublattice
        for(tg_lnh_3=0;tg_lnh_3 < LNH_N3; tg_lnh_3++)
        for(tg_lnh_2=0;tg_lnh_2 < LNH_N2; tg_lnh_2++)
        for(tg_lnh_1=0;tg_lnh_1 < LNH_N1; tg_lnh_1++)
        for(tg_lnh_0h=0;tg_lnh_0h < LNH_N0H; tg_lnh_0h++){


            int target_gl_snum = target_lnh_to_gl_snum(tg_lnh_0h*2, tg_lnh_1, tg_lnh_2, tg_lnh_3, target_gl_loc_origin_from_rank);
            int target_lnh_snum = snum_acc(tg_lnh_0h*2, tg_lnh_1, tg_lnh_2, tg_lnh_3);
            //printf("%d %d %d %d  %d  %d\n",tg_lnh_0h,tg_lnh_1,           // DEBUG
            //        tg_lnh_2, tg_lnh_3, target_gl_snum,target_lnh_snum); // DEBUG

            lnh_ferm->c0[target_lnh_snum] = gl_soa_ferm->c0[target_gl_snum];
            lnh_ferm->c1[target_lnh_snum] = gl_soa_ferm->c1[target_gl_snum];
            lnh_ferm->c2[target_lnh_snum] = gl_soa_ferm->c2[target_gl_snum];

        }
}
void recv_loc_subfermion_from_buffer(global_vec3_soa *gl_soa_ferm,
        vec3_soa* lnh_ferm, int target_rank)
{

if(verbosity_lv > 3) printf("MPI%02d - recv_loc_subferm_from_buffer()", devinfo.myrank);
    // USE ONLY FROM MASTER RANK
    //target sublattice information
    vec4int target_gl_loc_origin_from_rank = gl_loc_origin_from_rank(target_rank);

    int tg_loc_0h,tg_loc_1,tg_loc_2,tg_loc_3; //target-loc coordinates
    // and link direction
    //Copying all relevant links from the sublattice to the global lattice
    for(tg_loc_3=0;tg_loc_3<LOC_N3; tg_loc_3++)
        for(tg_loc_2=0;tg_loc_2<LOC_N2; tg_loc_2++)
        for(tg_loc_1=0;tg_loc_1<LOC_N1; tg_loc_1++)
        for(tg_loc_0h=0;tg_loc_0h<LOC_N0H; tg_loc_0h++){
    
                        int tg_lnh_0 = 2*tg_loc_0h + D0_HALO;
                        int tg_lnh_1 = tg_loc_1 + D1_HALO;
                        int tg_lnh_2 = tg_loc_2 + D2_HALO;
                        int tg_lnh_3 = tg_loc_3 + D3_HALO;


                        int target_gl_snum = target_lnh_to_gl_snum(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3, target_gl_loc_origin_from_rank);
                        int target_lnh_snum = snum_acc(tg_lnh_0, tg_lnh_1, tg_lnh_2, tg_lnh_3);
                        gl_soa_ferm->c0[target_gl_snum] = lnh_ferm->c0[target_lnh_snum];
                        gl_soa_ferm->c1[target_gl_snum] = lnh_ferm->c1[target_lnh_snum];
                        gl_soa_ferm->c2[target_gl_snum] = lnh_ferm->c2[target_lnh_snum];

                    }

}

#endif
