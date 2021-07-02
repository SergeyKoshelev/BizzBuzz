#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

struct zone{
double left;
double right;
int count;
};


void* integrate(void* arg)
{
    double* res = (double*) calloc(1, sizeof(double));
    struct zone* arg1 = (struct zone*)arg;
    double width = (arg1->right - arg1->left)/arg1->count;
    //printf("%d\n", arg1->count);

    for (int i = 0; i < arg1->count; i++)
        {

            double x = arg1->left + i*width + width/2;

            //printf("%lf\n", x);

            *res+= 2 * sqrt(1 - x*x) * width;

            //printf("%lf\n", res);
        }

    pthread_exit((void*)res);
}

int main(int argc, char** argv)
{
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    pthread_t* arr_threads = malloc(n*sizeof(pthread_t));
    struct zone* zones = malloc(n*sizeof(struct zone));

    for (int i = 0; i < n; i++)
    {
        (zones+i)->left = -1 + 2*(double)i/n;
        (zones+i)->right= -1 + 2*(double)(i+1)/n;
        (zones+i)->count = m/n;

        //printf("%lf %lf %d\n", (zones+i)->left, (zones+i)->right, (zones+i)->count);

        int check = pthread_create(arr_threads+i,NULL,integrate, zones+i);
        //pthread_join(arr_threads[i], NULL);

        if (check != 0)
            {
                printf ("Can't create thread\n");
                exit(1);
            }
    }

    void* temp = NULL;
    double result = 0;
    for (int i = 0; i < n; i++)
    {
        pthread_join(arr_threads[i], &temp);
        result += * (double*)temp;
        free(temp);
    }


    printf ("MY_PI    %.15lf\n", result);
    printf ("MY_PI - REAL_PI     %.15lf\n", result - M_PI);
    return 0;
}
