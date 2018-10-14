#ifndef __LAKE_H__
#define __LAKE_H__

#define XMIN 0.0
#define XMAX 1.0
#define YMIN 0.0
#define YMAX 1.0

#define MAX_PSZ 10
#define TSCALE 1.0
#define VSQR 0.1

//#define __DEBUG

void run_sim(double *u, double *u0, double *u1, double *pebbles, int n, double h, double end_time);
void init_pebbles(double *p, int pn, int n);
double f(double p, double t);
int tpdt(double *t, double dt, double tf);
void init(double *u, double *pebbles, int n);
void error_u(double *uerr, double *avgerr, double *ua, double *ub, int n);
void print_heatmap(char *filename, double *u, int n, double h);

#endif
