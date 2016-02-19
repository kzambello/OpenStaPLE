#ifndef IO_C_
#define IO_C_

#include "./io.h"
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include "./geometry.h"
#include "./struct_c_def.h"
#include "../Include/common_defines.h"
#include "./single_types.h"
#include <math.h>


void print_su3_soa_ASCII(su3_soa * const conf, const char* nomefile,int conf_id_iter){

    FILE *fp;
    fp = fopen(nomefile,"w");
    if(! fp ){
        printf("ERROR, %s unreadable.\n",nomefile);
        exit(1);
    }

    fprintf(fp,"%d\t%d\t%d\t%d\t%d\n",nx,ny,nz,nt,conf_id_iter);
    for(int q = 0 ; q < 8 ; q++){
        for(int i = 0 ; i < sizeh ; i++){
            // rebuilding the 3rd row
            single_su3 aux;
            single_su3_from_su3_soa(&conf[q],i,&aux);
            rebuild3row(&aux);


            for(int r=0; r < 3 ; r++)
                for(int c=0; c < 3 ; c++) 
                    fprintf(fp, "%.18lf %.18lf\n",
                            creal(aux.comp[r][c]),cimag(aux.comp[r][c])); 
        }
    }
    fclose(fp);
}
int read_su3_soa_ASCII(su3_soa * conf, const char* nomefile,int * conf_id_iter ){


  FILE *fp;
  fp = fopen(nomefile,"r");
  if(!fp){
      printf("Gauge configuration file %s not readable.\n",nomefile);
      *conf_id_iter = -1;
      return 1;
  }
  else{



  int nxt,nyt,nzt,ntt;
  int minus_det_count = 0;
  CHECKREAD(fscanf(fp,"%d\t%d\t%d\t%d\t%d\n",&nxt,&nyt,&nzt,&ntt,conf_id_iter),5);
  if((nx!=nxt)||(ny!=nyt)||(nz!=nzt)||(nz!=nzt)){
    printf(" Errore: DIMENSIONI DELLA CONFIGURAZIONE LETTA DIVERSE DA QUELLE ATTESE\n");
    abort();
  }

  for(int q = 0 ; q < 8 ; q++){
    for(int i = 0 ; i < sizeh ; i++){
      double re,im;
      single_su3 m;double det;
      //      fscanf(fp, "%.18lf\t%.18lf\n",&re,&im);
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[0][0]=conf[q].r0.c0[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[0][1]=conf[q].r0.c1[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[0][2]=conf[q].r0.c2[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[1][0]=conf[q].r1.c0[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[1][1]=conf[q].r1.c1[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[1][2]=conf[q].r1.c2[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[2][0]=conf[q].r2.c0[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[2][1]=conf[q].r2.c1[i] = re + im * I;
      CHECKREAD(fscanf(fp, "%lf %lf\n",&re,&im),2);m.comp[2][2]=conf[q].r2.c2[i] = re + im * I;
      det = detSu3(&m);
      if(fabs(1+det) < 0.005 ){ // DEBUG, the limit should be FAR stricter.
        if(verbosity_lv > 5)  printf("Warning in read_su3_soa_ASCII(), Det M = -1.\n");
        minus_det_count ++;
        conf[q].r0.c0[i] = -conf[q].r0.c0[i];
        conf[q].r0.c1[i] = -conf[q].r0.c1[i];
        conf[q].r0.c2[i] = -conf[q].r0.c2[i];
        conf[q].r1.c0[i] = -conf[q].r1.c0[i];
        conf[q].r1.c1[i] = -conf[q].r1.c1[i];
        conf[q].r1.c2[i] = -conf[q].r1.c2[i];
        conf[q].r2.c0[i] = -conf[q].r2.c0[i];
        conf[q].r2.c1[i] = -conf[q].r2.c1[i];
        conf[q].r2.c2[i] = -conf[q].r2.c2[i];

      }else if (fabs(fabs(det)-1.0)>0.005 ){  // DEBUG, SAME HERE
          printf("Non unitary matrix read, Det M =  %.18lf\n",det);
          double pippo =  fabs(det)-1.0;
          printf("Diff %f,    ", pippo);
          pippo = fabs(pippo);
          printf("Diff %f\n", pippo);
      }
      
    }
  }

  if(minus_det_count !=0 ) {
      printf("Conf read has some matrices with determinant = -1.\n");
      printf("This might be due to the configuration being saved\n");
      printf("is multiplied by the staggered phase.\n");
      printf("In such case, we would expect %d matrices to have determinant = -1\n",
             nx*ny*nz*nt*3/2 );
      printf("The count of matrices with determinant = -1 is %d. \n", minus_det_count);
      




  }
  fclose(fp);
  return 0;
  }
}

void print_su3_soa_ildg_binary(su3_soa * const conf, const char* nomefile,
        int conf_id_iter)
{
    FILE *fp;
    fp = fopen(nomefile,"w");
    if(! fp ){
        printf("ERROR, %s unreadable.\n",nomefile);
        exit(1);
    }

    printf("ILDG PRINT NOT IMPLEMENTED!\n");
    exit(1);
    return ;
}
int read_su3_soa_ildg_binary(su3_soa * conf, const char* nomefile,int * conf_id_iter ){

  FILE *fp;
  fp = fopen(nomefile,"r");
  if(!fp){
      printf("Gauge configuration ILDG file %s not readable.\n",nomefile);
      *conf_id_iter = -1;
      return 1;
  } 
  // get to the payload, somehow

  int x,y,z,t,dir;
  for(t=0;t<nt;t++) for(z=0;z<nz;z++)
      for(y=0;y<ny;y++) for(x=0;x<nx;x++)
      {

          int idxh = snum_acc(x,y,z,t);
          int parity = (x+y+z+t)%2;

          for(dir=0;dir<4;dir++){

              single_su3 m;          
              fread((void*)m.comp,sizeof(double),18,fp);

              single_su3_into_su3_soa(&m,&conf[dir+parity],idxh);

          }

      }



    printf("ILDG READ NOT IMPLEMENTED!\n");
    exit(1);
    return -1;
    
    
}




#endif
