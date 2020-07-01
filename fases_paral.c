#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define TAM_VETOR 1000000    // numero de elementos do vetor
#define DELTA 20         // porcentagem de valores que seram trocados por vez
//#define DEBUG 1

void bs(int n, int * vetor) {
  int c=0, d, troca, trocou =1;

  while (c < (n-1) & trocou ) {
    trocou = 0;
    for (d = 0 ; d < n - c - 1; d++)
      if (vetor[d] > vetor[d+1]) {
        troca = vetor[d];
        vetor[d] = vetor[d+1];
        vetor[d+1] = troca;
        trocou = 1;
      }
      c++;
  }
}

int *interleaving(int vetor[], int tam)
{
	int *vetor_auxiliar;
	int i1, i2, i_aux;

	vetor_auxiliar = (int *)malloc(sizeof(int) * tam);

	i1 = 0;
	i2 = tam / 2;

	for (i_aux = 0; i_aux < tam; i_aux++) {
		if (((vetor[i1] <= vetor[i2]) && (i1 < (tam / 2)))
		    || (i2 == tam))
			vetor_auxiliar[i_aux] = vetor[i1++];
		else
			vetor_auxiliar[i_aux] = vetor[i2++];
	}

	return vetor_auxiliar;
}

int main(int argc, char** argv)
{
    int my_rank;  // Identificador deste processo
    int proc_n;   // Numero de processos disparados pelo usuário na linha de comando (np)
    
    double time;

    int i;
    int n_elementos;
    int * vetor;
    int * vetor_auxiliar;
    int buffer;
    int ordenado;
    int pronto;
    int delta;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);

    n_elementos = 1.0/proc_n * TAM_VETOR;
    delta = DELTA/100.0 * n_elementos;

    #ifdef DEBUG
    printf("n_elementos : %d\n", n_elementos);
    printf("delta : %d\n", delta);
    #endif

    // inicializar o vetor de elementos
    vetor = (int*)malloc((n_elementos + delta) * sizeof(int));
    for(i = 0; i < n_elementos; i++)
    {
        vetor[i] = (TAM_VETOR - i) - (n_elementos * my_rank);
        #ifdef DEBUG
        printf("my_rank : %d , vetor[i] : %d\n", my_rank, vetor[i]);
        #endif
    }

    time = MPI_Wtime();
    while(1)
    {
        // ordenar
        bs(n_elementos, vetor);

        // verificar condição de parada
        if(my_rank != (proc_n - 1))
        {
            MPI_Send(&vetor[n_elementos-1], 1, MPI_INT, (my_rank+1), 0, MPI_COMM_WORLD);
        }

        if(my_rank != 0)
        {
            MPI_Recv (&buffer, 1, MPI_INT, my_rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if(buffer < vetor[0])
                ordenado = 1;
            else
                ordenado = 0;
        }
        else
            ordenado = 1;

        pronto = 1;
        for(i = 0; i < proc_n; i++)
        {
            if(my_rank == i) // minha vez
                buffer = ordenado;

            MPI_Bcast(&buffer, 1, MPI_INT, i, MPI_COMM_WORLD);

            if(buffer == 0) // não esta pronto
            {
                pronto = 0;
                break;
            }
        }

        if(pronto == 1) // se pronto continua 1 então tudo está ordenado
            break;

        // caso ainda nao esteja pronto, trocar valores

        if(my_rank != 0)
        {
            MPI_Send(&vetor[0], delta, MPI_INT, (my_rank-1), 0, MPI_COMM_WORLD);
        }

        if(my_rank != (proc_n - 1))
        {
            MPI_Recv(&vetor[n_elementos], delta, MPI_INT, (my_rank+1), MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            vetor_auxiliar = interleaving(&vetor[n_elementos-delta], delta*2);
            for(i = 0; i < delta*2; i++)
            {
                vetor[n_elementos-delta+i] = vetor_auxiliar[i];
            }

            MPI_Send(&vetor[n_elementos], delta, MPI_INT, (my_rank+1), 0, MPI_COMM_WORLD);
        }

        if(my_rank != 0)
        {
            MPI_Recv(&vetor[0], delta, MPI_INT, (my_rank-1), MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }
    }

    if(my_rank == 0)
        printf("time: %1.2f\n", MPI_Wtime() - time);

    #ifdef DEBUG
    printf("VALORES FINAIS:\n");
    for(i = 0; i < n_elementos; i++)
    {
        printf("rank : %d : vetor[i] : %d\n", my_rank, vetor[i]);
    }
    #endif
        
    MPI_Finalize();
    return 0;
}