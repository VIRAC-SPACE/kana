#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <libgen.h>
#include <unistd.h>
#include "../include/jsonReader.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "hzCorrect.h"

#define CTRL_FILE 1
#define FIRST_SIGNAL 2

int  main(int argc, char **argv) {
    if(argc < 3) {
        panic("Please enter the necessary arguments (freqCorrect [ctrl] [sig])");
    }

    int rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    char* ctrlFile = basename(argv[CTRL_FILE]);
    char* directory = getFileDirectory(argv[CTRL_FILE]);
    char* signalName = argv[rank + FIRST_SIGNAL];
    chdir(directory);

    printf("\033[36m%s >> %s\033[0m\n", "Frequency correction", signalName);
    printf("\033[36m=============================\033[0m\n");

    // read control file
    JSON_Tree* params = malloc(sizeof(JSON_Tree));
    // if (params == NULL) ALLOC_ERROR;
    if (JSON_read(params, ctrlFile) == 0) {
        panic("cannot read control file");
    }

    processDataFolder(params, signalName);

    free(directory);
    free(params->tree);
    free(params);
    MPI_Finalize();
    return 0;
}