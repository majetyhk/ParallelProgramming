#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* first grid point */
#define   XI              1.0
/* last grid point */
#define   XF              100.0

/* function declarations */
double     fn(double);
void        print_function_data(int, double*, double*, double*);
int         main(int, char**);

int main (int argc, char *argv[])
{
        int NGRID;
        if(argc > 1)
            NGRID = atoi(argv[1]);
        else 
        {
                printf("Please specify the number of grid points.\n");
                exit(0);
        }
        //loop index
        int         i;

        //domain array and step size
        double       *xc = (double *)malloc(sizeof(double)* (NGRID + 2));
        double      dx;

        //function array and derivative
        //the size will be dependent on the
        //number of processors used
        //to the program
        double     *yc, *dyc;

        //"real" grid indices
        int         imin, imax;  

        imin = 1;
        imax = NGRID;

        //construct grid
        for (i = 1; i <= NGRID ; i++)
        {
                xc[i] = XI + (XF - XI) * (double)(i - 1)/(double)(NGRID - 1);
        }
        //step size and boundary points
        dx = xc[2] - xc[1];
        xc[0] = xc[1] - dx;
        xc[NGRID + 1] = xc[NGRID] + dx;

        //allocate function arrays
        yc  =   (double*) malloc((NGRID + 2) * sizeof(double));
        dyc =   (double*) malloc((NGRID + 2) * sizeof(double));

        //define the function
        for( i = imin; i <= imax; i++ )
        {
                yc[i] = fn(xc[i]);
        }

        //set boundary values
        yc[imin - 1] = 0.0;
        yc[imax + 1] = 0.0;

        //NB: boundary values of the whole domain
        //should be set
        yc[0] = fn(xc[0]);
        yc[imax + 1] = fn(xc[NGRID + 1]);

        //compute the derivative using first-order finite differencing
        //
        //  d           f(x + h) - f(x - h)
        // ---- f(x) ~ --------------------
        //  dx                 2 * dx
        //
        for (i = imin; i <= imax; i++)
        {
                dyc[i] = (yc[i + 1] - yc[i - 1])/(2.0 * dx);
        }

        print_function_data(NGRID, &xc[1], &yc[1], &dyc[1]);


        //free allocated memory 
        free(yc);
        free(dyc);

        return 0;
}

//prints out the function and its derivative to a file
void print_function_data(int np, double *x, double *y, double *dydx)
{
        int   i;

        char filename[1024];
        sprintf(filename, "fn-%d.dat", np);

        FILE *fp = fopen(filename, "w");

        for(i = 0; i < np; i++)
        {
                fprintf(fp, "%f %f %f\n", x[i], y[i], dydx[i]);
        }

        fclose(fp);
}
