/*
    AC - OpenMP -- SERIE
    fun_s.c
     rutinas que se utilizan en el modulo gengrupos_s.c
****************************************************************************/
#include <math.h>
#include <float.h> // DBL_MAX
#include <stdlib.h>
#include <stdio.h>
#include "defineg.h"           // definiciones


float sort_and_median(int n, float* disease_data){

    return 0.0f;
}


/**************************************************************************************
   1 - Funcion para calcular la distancia genetica entre dos elementos (distancia euclidea)
       Entrada:  2 elementos con NCAR caracteristicas (por referencia)
       Salida:  distancia (double)
**************************************************************************************/
double geneticdist(float *elem1, float *elem2)
{
    double total = 0.0f;

    for(int i = 0; i < NCAR; i++)
        total += pow((double)(elem2[i] - elem1[i]), 2);

	return sqrt(total);
}

/****************************************************************************************
   2 - Funcion para calcular el grupo (cluster) mas cercano (centroide mas cercano)
   Entrada:  nelem  numero de elementos, int
             elem   elementos, una matriz de tamanno MAXE x NCAR, por referencia
             cent   centroides, una matriz de tamanno NGRUPOS x NCAR, por referencia
   Salida:   samples  grupo mas cercano a cada elemento, vector de tamanno MAXE, por referencia
*****************************************************************************************/
void nearest_cluster(int nelem, float elem[][NCAR], float cent[][NCAR], int *samples)
{
	double dist, mindist;

	for(int i = 0; i < nelem; i++){
        mindist = __DBL_MAX__;
		for(int k = 0; k < nclusters; k++) {
			dist = geneticdist(elem[i], cent[k]);
			if(dist < mindist){
				mindist = dist;
                samples[i] = k;
			}
		}
	}
}


/****************************************************************************************
   3 - Funcion para calcular la calidad de la particion de clusteres.
       Ratio entre a y b. El termino a corresponde a la distancia intra-cluster.
       El termino b corresponde a la distancia inter-cluster.
   Entrada:  samples     elementos, una matriz de tamanno MAXE x NCAR, por referencia
             cluster_data   vector de NCLUSTERS structs (informacion de grupos generados), por ref.
             centroids     centroides, una matriz de tamanno NCLUSTERS x NCAR, por referencia
   Salida:   valor del CVI (double): calidad/ bondad de la particion de clusters
*****************************************************************************************/
double silhouette_simple(float samples[][NCAR], struct lista_grupos *cluster_data, float centroids[][NCAR], float a[]){
    float b[nclusters];
    for(int k = 0; k < nclusters; k++) b[k] = 0.0f;
    for(int k = 0; k < MAX_GRUPOS; k++) a[k] = 0.0f;
    // Me baso en teoría de grafos para obtener el peso total de las distancias
    // Según se va avanzando, únicamente tengo en cuenta las distancias
    // con elementos posicionados en posiciones mayores que la actual.
    for(int k = 0; k < nclusters; k++){
        for(int i = 0; i < cluster_data[k].nelems; i++){
            for(int j = i + 1; j < cluster_data[k].nelems; j++){
                a[k] += (float) geneticdist(samples[cluster_data[k].elem_index[i]],
                                            samples[cluster_data[k].elem_index[j]]);
            }
        }

        // medidas para los n elementos de cada clúster. n(n-1)/2.
        // Equivale al Cálculo de aristas totales para un grafo completo.
        float narista = ((float)(cluster_data[k].nelems * (cluster_data[k].nelems - 1)) / 2);
        a[k] = cluster_data[k].nelems <= 1 ? 0 : a[k] / narista;
    }

    // aproximar b[i] de cada cluster
    for(int k = 0; k < nclusters; k++){
        for(int j = 0; j < nclusters; j++){
            b[k] += (float) geneticdist(centroids[k], centroids[j]); // Cuando k = j -> geneticdist = 0.
        }
        b[k] /=  (float)(nclusters - 1);
    }

    float max, sil = 0.0f;
    for(int k = 0; k < nclusters; k++){
        max = a[k] >= b[k] ? a[k] : b[k];
        if(max != 0.0f)
            sil += (b[k]-a[k])/max;
    }

    printf("\nSILHOUETEE: s=%f, nclusters=%i Calidad de cluster: %f\n", sil, nclusters, (double)(sil / (float)nclusters));
    return (double)(sil / (float)nclusters);
}

/********************************************************************************************
   4 - Funcion para relizar el analisis de enfermedades
   Entrada:  cluster_data   vector de NGRUPOS structs (informacion de grupos generados), por ref.
             enf      enfermedades, una matriz de tamaño MAXE x TENF, por referencia
   Salida:   prob_enf vector de TENF structs (informacion del análisis realizado), por ref.
*****************************************************************************************/
void analisis_enfermedades(struct lista_grupos *cluster_data, float enf[][TENF], struct analisis *data)
{
    // Ahora utilizamos algoritmo de burbuja para el ordenado, pero lo reemplazaremos por
    // Quicksort en breves.

    int cluster_size = 0;
    float medians[TENF][nclusters];

    for(int i = 0; i < TENF; i++)
        for(int j = 0; j < nclusters; j++)
            medians[i][j] = 0.0f;

    for(int k = 0; k < nclusters; k++){
        for(int i = 0; i < TENF; i++){

            cluster_size = cluster_data[k].nelems;
            float disease_data[cluster_size];
            for(int n = 0; n < cluster_size; n++) disease_data[n] = 0.0f;

            for(int j = 0; j < cluster_size; j++)
                disease_data[j] = enf[cluster_data[k].elem_index[j]][i];

            medians[i][k] = sort_and_median(cluster_size, disease_data);
        }
    }

    float median = 0.0f;
    for(int i = 0; i < TENF; i++){
        data[i].mmin = 0;
        data[i].mmax = __DBL_MAX__;
        for(int k = 0; k < nclusters; k++){
            median = medians[i][k];
            if(median < data[i].mmin){
                data[i].mmin = median;
                data[i].gmin = k;
            }
            if(median > data[i].mmax){
                data[i].mmax = median;
                data[i].gmax = k;
            }
        }
    }
}

/***************************************************************************************************
   OTRAS FUNCIONES DE LA APLICACION
****************************************************************************************************/

void inicializar_centroides(float cent[][NCAR]){
    //printf("\n CENTROID INITS \n");
	int i, j;
	srand (147);
	for (i=0; i < nclusters; i++)
		for (j=0; j<NCAR/2; j++){
			cent[i][j] = (rand() % 10000) / 100.0;
			cent[i][j+(NCAR/2)] = cent[i][j];
		}
}

int nuevos_centroides(float elem[][NCAR], float cent[][NCAR], int samples[], int nelem){
    //printf("\n NEW CENCTROIDSS \n");
	int i, j, fin;
	double discent;
	double additions[nclusters][NCAR + 1];
	float newcent[nclusters][NCAR];

	for (i=0; i < nclusters; i++)
		for (j=0; j<NCAR+1; j++)
			additions[i][j] = 0.0;

	// acumular los valores de cada caracteristica (100); numero de elementos al final
	for (i=0; i<nelem; i++){
		for (j=0; j<NCAR; j++) additions[samples[i]][j] += elem[i][j];
		additions[samples[i]][NCAR]++;
	}

	// calcular los nuevos centroides y decidir si el proceso ha finalizado o no (en funcion de DELTA)
	fin = 1;
	for (i=0; i < nclusters; i++){
		if (additions[i][NCAR] > 0) { // ese grupo (cluster) no esta vacio
			// media de cada caracteristica
			for (j=0; j<NCAR; j++)
				newcent[i][j] = (float)(additions[i][j] / additions[i][NCAR]);

			// decidir si el proceso ha finalizado
			discent = geneticdist(&newcent[i][0], &cent[i][0]);
			if (discent > DELTA1)
				fin = 0;  // en alguna centroide hay cambios; continuar

			// copiar los nuevos centroides
			for (j=0; j<NCAR; j++)
				cent[i][j] = newcent[i][j];
		}
	}
	return fin;
}