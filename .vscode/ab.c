#include <stdio.h>
#include <stdlib.h>
#include<time.h>
void compute_averages(int X[], int n, double A[]) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j <= i; j++) {
            sum += X[j];
        }
        A[i] = sum / (i + 1);
    }
}

int main() {
    int n;

    printf("Enter the number of elements: ");
    scanf("%d", &n);

    // Dynamically allocate memory for arrays X and A
    int *X = (int *)malloc(n * sizeof(int));
    double *A = (double *)malloc(n * sizeof(double));

    // Input elements for array X
    printf("Enter the elements of the array:\n");
    for (int i = 0; i < n; i++) {
        scanf("%d", &X[i]);
    }

    // Compute the averages
    clock_t start=clock();
    compute_averages(X, n, A);
    clock_t end=clock();
    // Print the averages array
    printf("Averages array:\n");
    for (int i = 0; i < n; i++) {
        printf("%f ", A[i]);
    }
    printf("\n");
    printf("time taken",end-start);
    // Free the allocated memory
    free(X);
    free(A);

    return 0;
}
