/*
 * lowlevel/lvd/qdc/qdc8.h
 * $ZEL: qdc8.h,v 1.1 2012/09/11 23:39:17 wuestner Exp $
 * created created 2012-Aug-30(?) PK
 */

#ifndef _lvd_qdc8_h_
#define _lvd_qdc8_h_

struct lvd_dev;

plerrcode lvd_qdc_setc_outp(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 outp);
plerrcode lvd_qdc_getc_outp(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* outp);

plerrcode lvd_qdc_setc_festart(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 festart);
plerrcode lvd_qdc_getc_festart(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* festart);

plerrcode lvd_qdc_setc_femin(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 femin);
plerrcode lvd_qdc_getc_femin(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* femin);

plerrcode lvd_qdc_setc_femax(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 femax);
plerrcode lvd_qdc_getc_femax(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* femax);

plerrcode lvd_qdc_setc_feend(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feend);
plerrcode lvd_qdc_getc_feend(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feend);

plerrcode lvd_qdc_setc_fetot(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 fetot);
plerrcode lvd_qdc_getc_fetot(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* fetot);

plerrcode lvd_qdc_setc_fecfd(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 fecfd);
plerrcode lvd_qdc_getc_fecfd(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* fecfd);

plerrcode lvd_qdc_setc_fezero(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 fezero);
plerrcode lvd_qdc_getc_fezero(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* fezero);

plerrcode lvd_qdc_setc_feqae(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feqae);
plerrcode lvd_qdc_getc_feqae(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feqae);

plerrcode lvd_qdc_setc_feqpls(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feqpls);
plerrcode lvd_qdc_getc_feqpls(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feqpls);

plerrcode lvd_qdc_setc_feqclab(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feqclab);
plerrcode lvd_qdc_getc_feqclab(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feqclab);

plerrcode lvd_qdc_setc_feqclae(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feqclae);
plerrcode lvd_qdc_getc_feqclae(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feqclae);

plerrcode lvd_qdc_setc_feiwedge(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feiwedge);
plerrcode lvd_qdc_getc_feiwedge(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feiwedge);

plerrcode lvd_qdc_setc_feiwmax(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feiwmax);
plerrcode lvd_qdc_getc_feiwmax(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feiwmax);

plerrcode lvd_qdc_setc_feiwcenter(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feiwcenter);
plerrcode lvd_qdc_getc_feiwcenter(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feiwcenter);

plerrcode lvd_qdc_setc_feovrun(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 feovrun);
plerrcode lvd_qdc_getc_feovrun(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* feovrun);

plerrcode lvd_qdc_setc_fenoise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 fenoise);
plerrcode lvd_qdc_getc_fenoise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* fenoise);

plerrcode lvd_qdc_setc_chctrl(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 chctrl);
plerrcode lvd_qdc_getc_chctrl(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* chctrl);

plerrcode lvd_qdc_setc_chaena(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 chaena);
plerrcode lvd_qdc_getc_chaena(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* chaena);

plerrcode lvd_qdc_setc_plev(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 plev);
plerrcode lvd_qdc_getc_plev(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* plev);

plerrcode lvd_qdc_setc_polarity(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 polarity);
plerrcode lvd_qdc_getc_polarity(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* polarity);

plerrcode lvd_qdc_setc_gradient(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 gradient);
plerrcode lvd_qdc_getc_gradient(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* gradient);

plerrcode lvd_qdc_setc_noext(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 noext);
plerrcode lvd_qdc_getc_noext(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* noext);

plerrcode lvd_qdc_setc_tot4raw(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 tot4raw);
plerrcode lvd_qdc_getc_tot4raw(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* tot4raw);

plerrcode lvd_qdc_setc_cfqrise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 cfqrise);
plerrcode lvd_qdc_getc_cfqrise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* cfqrise);

plerrcode lvd_qdc_setc_qrise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 qrise);
plerrcode lvd_qdc_getc_qrise(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* qrise);

plerrcode lvd_qdc_setc_cf(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 cf);
plerrcode lvd_qdc_getc_cf(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* cf);

plerrcode lvd_qdc_setc_totlevel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 totlevel);
plerrcode lvd_qdc_getc_totlevel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* totlevel);

plerrcode lvd_qdc_setc_totthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 totthr);
plerrcode lvd_qdc_getc_totthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* totthr);

plerrcode lvd_qdc_setc_totwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 totwidth);
plerrcode lvd_qdc_getc_totwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* totwidth);

plerrcode lvd_qdc_setc_logiclevel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 logiclevel);
plerrcode lvd_qdc_getc_logiclevel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* logiclevel);

plerrcode lvd_qdc_setc_logicthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 logicthr);
plerrcode lvd_qdc_getc_logicthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* logicthr);

plerrcode lvd_qdc_setc_logicwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 logicwidth);
plerrcode lvd_qdc_getc_logicwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* logicwidth);

plerrcode lvd_qdc_setc_iwqthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 iwqthr);
plerrcode lvd_qdc_getc_iwqthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* iwqthr);

plerrcode lvd_qdc_setc_swqthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 swqthr);
plerrcode lvd_qdc_getc_swqthr(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* swqthr);

plerrcode lvd_qdc_setc_swilen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 swilen);
plerrcode lvd_qdc_getc_swilen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* swilen);

plerrcode lvd_qdc_setc_clqstlen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 clqstlen);
plerrcode lvd_qdc_getc_clqstlen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* clqstlen);

plerrcode lvd_qdc_setc_cllen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 cllen);
plerrcode lvd_qdc_getc_cllen(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* cllen);

plerrcode lvd_qdc_setc_clqdel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 clqdel);
plerrcode lvd_qdc_getc_clqdel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* clqdel);

plerrcode lvd_qdc_setc_coincpar(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 coincpar);
plerrcode lvd_qdc_getc_coincpar(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* coincpar);

plerrcode lvd_qdc_setc_coincdel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 coincdel);
plerrcode lvd_qdc_getc_coincdel(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* coincdel);

plerrcode lvd_qdc_setc_coincwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 coincwidth);
plerrcode lvd_qdc_getc_coincwidth(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* coincwidth);

plerrcode lvd_qdc_setc_dttau(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 dttau);
plerrcode lvd_qdc_getc_dttau(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* dttau);

plerrcode lvd_qdc_setc_rawtable(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32 rawtable);
plerrcode lvd_qdc_getc_rawtable(struct lvd_dev* dev, int addr, ems_i32 channel, ems_u32* rawtable);

plerrcode lvd_qdc_setg_swstart(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 swstart);
plerrcode lvd_qdc_getg_swstart(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* swstart);

plerrcode lvd_qdc_setg_swlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 swlen);
plerrcode lvd_qdc_getg_swlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* swlen);

plerrcode lvd_qdc_setg_iwstart(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 iwstart);
plerrcode lvd_qdc_getg_iwstart(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* iwstart);

plerrcode lvd_qdc_setg_iwlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 iwlen);
plerrcode lvd_qdc_getg_iwlen(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* iwlen);

plerrcode lvd_qdc_setg_coinctab(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32 coinctab);
plerrcode lvd_qdc_getg_coinctab(struct lvd_dev* dev, int addr, ems_i32 group, ems_u32* coinctab);
plerrcode lvd_qdc_setm_cr(struct lvd_dev* dev, int addr, ems_u32 cr);
plerrcode lvd_qdc_getm_cr(struct lvd_dev* dev, int addr, ems_u32* cr);
plerrcode lvd_qdc_setm_ena(struct lvd_dev* dev, int addr, ems_u32 ena);
plerrcode lvd_qdc_getm_ena(struct lvd_dev* dev, int addr, ems_u32* ena);
plerrcode lvd_qdc_setm_trgmode(struct lvd_dev* dev, int addr, ems_u32 trgmode);
plerrcode lvd_qdc_getm_trgmode(struct lvd_dev* dev, int addr, ems_u32* trgmode);
plerrcode lvd_qdc_setm_tdcena(struct lvd_dev* dev, int addr, ems_u32 tdcena);
plerrcode lvd_qdc_getm_tdcena(struct lvd_dev* dev, int addr, ems_u32* tdcena);
plerrcode lvd_qdc_setm_fixbaseline(struct lvd_dev* dev, int addr, ems_u32 fixbaseline);
plerrcode lvd_qdc_getm_fixbaseline(struct lvd_dev* dev, int addr, ems_u32* fixbaseline);
plerrcode lvd_qdc_setm_tstsig(struct lvd_dev* dev, int addr, ems_u32 tstsig);
plerrcode lvd_qdc_getm_tstsig(struct lvd_dev* dev, int addr, ems_u32* tstsig);
plerrcode lvd_qdc_setm_adcpwr(struct lvd_dev* dev, int addr, ems_u32 adcpwr);
plerrcode lvd_qdc_getm_adcpwr(struct lvd_dev* dev, int addr, ems_u32* adcpwr);
plerrcode lvd_qdc_setm_f1mode(struct lvd_dev* dev, int addr, ems_u32 f1mode);
plerrcode lvd_qdc_getm_f1mode(struct lvd_dev* dev, int addr, ems_u32* f1mode);
plerrcode lvd_qdc_setm_grpcoinc(struct lvd_dev* dev, int addr, ems_u32 grpcoinc);
plerrcode lvd_qdc_getm_grpcoinc(struct lvd_dev* dev, int addr, ems_u32* grpcoinc);
plerrcode lvd_qdc_setm_scalerrout(struct lvd_dev* dev, int addr, ems_u32 scalerrout);
plerrcode lvd_qdc_getm_scalerrout(struct lvd_dev* dev, int addr, ems_u32* scalerrout);
plerrcode lvd_qdc_setm_coinmintraw(struct lvd_dev* dev, int addr, ems_u32 coinmintraw);
plerrcode lvd_qdc_getm_coinmintraw(struct lvd_dev* dev, int addr, ems_u32* coinmintraw);
plerrcode lvd_qdc_setm_traw(struct lvd_dev* dev, int addr, ems_u32 traw);
plerrcode lvd_qdc_getm_traw(struct lvd_dev* dev, int addr, ems_u32* traw);
plerrcode lvd_qdc_setm_coinmin(struct lvd_dev* dev, int addr, ems_u32 coinmin);
plerrcode lvd_qdc_getm_coinmin(struct lvd_dev* dev, int addr, ems_u32* coinmin);
plerrcode lvd_qdc_setm_tpdac(struct lvd_dev* dev, int addr, ems_u32 tpdac);
plerrcode lvd_qdc_getm_tpdac(struct lvd_dev* dev, int addr, ems_u32* tpdac);
plerrcode lvd_qdc_setm_blcorcycle(struct lvd_dev* dev, int addr, ems_u32 blcorcycle);
plerrcode lvd_qdc_getm_blcorcycle(struct lvd_dev* dev, int addr, ems_u32* blcorcycle);

plerrcode lvd_qdc_getm_baseline(struct lvd_dev* dev, int addr, ems_u32* baseline);

plerrcode lvd_qdc_getm_tcornoise(struct lvd_dev* dev, int addr, ems_u32* tcornoise);

plerrcode lvd_qdc_getm_tcor(struct lvd_dev* dev, int addr, ems_u32* tcor);

plerrcode lvd_qdc_getm_noise(struct lvd_dev* dev, int addr, ems_u32* noise);

plerrcode lvd_qdc_getm_dttauquality(struct lvd_dev* dev, int addr, ems_u32* dttauquality);

plerrcode lvd_qdc_getm_ovr(struct lvd_dev* dev, int addr, ems_u32* ovr);

plerrcode lvd_qdc_setm_ctrl(struct lvd_dev* dev, int addr, ems_u32 ctrl);

#endif
