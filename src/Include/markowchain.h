#ifndef _MARKOWCHAIN_H_
#define _MARKOWCHAIN_H_

typedef struct mc_param_t{
    int ntraj                  ;
    int therm_ntraj            ;
    int saveconfinterval       ;
    int saverunningconfinterval;
    double residue_metro;
    double residue_md;
    char store_conf_name[30];
    char save_conf_name[30];
    int seed;
}mc_param;

extern mc_param mkwch_pars;

#endif
