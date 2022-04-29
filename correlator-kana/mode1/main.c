#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include "../include/jsonReader.h"
#include "../include/directories.h"
#include "../include/panic.h"
#include "../include/utils.h"
#include "mode1.h"

#define CTRL_FILE 1
#define FIRST_SIGNAL 2

int  main(int argc, char** argv) {
    if(argc < 3) {
        panic("Please enter the necessary arguments (m1 [controlFile] [signal])");
    }
    int provided;
    int stat = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (stat != MPI_SUCCESS) {
        panic("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, stat);
    }

    int rank = 0;
    int numtasks;
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
    int RANK_OF_NODE;
    MPI_Comm_rank(MPI_COMM_WORLD, &RANK_OF_NODE);

    char* ctrlFile = basename(argv[CTRL_FILE]);
    char* directory = getFileDirectory(argv[CTRL_FILE]);
    char* station = argv[rank + FIRST_SIGNAL];
    chdir(directory);

    // make directories
    mkdir(RESULT_DIR, 0777);
    mkdir(SIGNAL_DIR, 0777);
    mkdir(JSON_DIR, 0777);
    mkdir(M1_SPECTRA_DIR, 0777);

    printf("\033[36m%s >> %s\033[0m\n", "Mode 1 correlation", station);
    printf("\033[36m=============================\033[0m\n");

    // read control file
    JSON_Tree* params = malloc(sizeof(JSON_Tree));
    //if (params == NULL) ALLOC_ERROR;
    if (JSON_read(params, ctrlFile) == 0) {
        panic("cannot read control file");
    }

    if (JSON_getBool(params->tree, "root:realtime") == true) {
        processRealtimeData(params, station);
    } else {
        processDataFolder(params, station);
    }

    free(directory);
    free(params->tree);
    free(params);
    MPI_Barrier( MPI_COMM_WORLD );
    MPI_Finalize();
    return 0;
}