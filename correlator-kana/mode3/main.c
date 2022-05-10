#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <mpi.h>
#include "../include/jsonReader.h"
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "mode3.h"

#define CTRL_FILE 1
#define FIRST_SIGNAL 2

#define CORES_PER_PC 16

void gnuplot(char* signalOne, char* signalTwo);

// $ mpiexec --hostfile <host_file> -n <combination_count> m3 <ctrl> <s1> <s2> <s1> <s3> ..
int main(int argc, char **argv) {
    if (argc < 4) {
        panic("please specify the experiment name");
    }

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int pairCount = (argc - FIRST_SIGNAL) / 2;
    int procPerPair = size / pairCount;

    int currPair = rank / procPerPair;
    int currChan = (rank % procPerPair) + 1;

    char* ctrlFile = basename(argv[CTRL_FILE]);
    char* directory = getFileDirectory(argv[CTRL_FILE]);
    char* signalOne = argv[(currPair * 2) + FIRST_SIGNAL];
    char* signalTwo = argv[(currPair * 2) + FIRST_SIGNAL + 1];
    chdir(directory);

    // make directories
    mkdir(RESULT_DIR, 0777);
    mkdir(M3_SPECTRA_DIR, 0777);

    printf("\033[36m%s >> %s-%s\033[0m\n", "Mode 3 correlation", signalOne, signalTwo);
    printf("\033[36m=============================\033[0m\n");

    // read control file
    JSON_Tree* params = malloc(sizeof(JSON_Tree));
    // if (params == NULL) ALLOC_ERROR;
    if (JSON_read(params, ctrlFile) == 0) {
        panic("cannot read control file");
    }

    long uBegin = JSON_getInt(params->tree, "root:m3_delay_begin");
    long uEnd = JSON_getInt(params->tree, "root:m3_delay_end");

    for(long u = uBegin; u <= uEnd; ++u){
        processData(params, u, signalOne, signalTwo, currChan);
    }

    //gnuplot(signalOne, signalTwo);

    free(params->tree);
    MPI_Finalize();
    return 0;
}

void gnuplot(char* signalOne, char* signalTwo) {
    char* psName = malloc(strlen(M3_SPECTRA_DIR) + 100);
    // if(!psName) ALLOC_ERROR;
    sprintf(psName,"%s%s-%s.spectra", M3_SPECTRA_DIR, signalOne, signalTwo);

    char* psName2 = malloc(strlen(M3_SPECTRA_DIR) + 100);
    // if(!psName) ALLOC_ERROR;
    sprintf(psName2,"%s%s-%s", M3_SPECTRA_DIR, signalOne, signalTwo);

    FILE* pipe3d = popen("gnuplot","w");
    fprintf(pipe3d, "set term postscript eps enhanced color\n");
    fprintf(pipe3d, "set output '%s1.eps'\n", psName2);
    fprintf(pipe3d, "set xlabel 'Hz'\n");
    fprintf(pipe3d, "set ylabel 'Delay'\n");
    fprintf(pipe3d, "set zlabel 'Power'\n");

    fprintf(pipe3d, "set contour base\n set hidden3d offset 1\n");
    fprintf(pipe3d, "set grid layerdefault   linetype -1 linecolor rgb 'gray'  linewidth 0.200,  linetype -1 linecolor rgb 'gray' linewidth 0.200\n");

    fprintf(pipe3d, "set xrange [-10:10]\n");
    //fprintf(pipe3d, "set yrange [-80:80]\n");

    //fprintf(pipe3d, "set zrange [0.7e6:1.1e6]\n");

    //fprintf(pipe3d, "set pm3d at sb\n");
    fprintf(pipe3d, "set palette rgbformulae 22,13,-31\n");
    fprintf(pipe3d, "splot '%s' u 2:1:3 with pm3d\n", psName);

    //fprintf(pipe3d, "set dgrid3d 30,30\n");
    //fprintf(pipe3d, "set output '%s1.eps'\n", psName2);
    //fprintf(pipe3d, "set hidden3d\n");
    //fprintf(pipe3d, "splot 'dataPlot.dat' u 2:1:3 with lines\n");
    //fprintf(pipe3d, "set palette rgbformulae 22,13,-31\n");

    //fprintf(pipe3d, "splot 'dataPlot.dat' with pm3d\n");
    //fprintf(pipe3d, "set xrange [-8:8]\n");
    //fprintf(pipe3d, "set yrange [0:100]\n");
    fprintf(pipe3d, "set pm3d map\n");
    fprintf(pipe3d, "set output '%s2.eps'\n", psName2);
    fprintf(pipe3d, "splot '%s' u 2:1:3 with pm3d\n", psName);

    fflush(pipe3d);
    pclose(pipe3d);
}