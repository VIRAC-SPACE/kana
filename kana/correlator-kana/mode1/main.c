#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <mpi.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include "../jsonReader.h"
#include "../directories.h"
#include "../panic.h"
#include "../utils.h"
#include "mode1.h"

#define CTRL_FILE 1
#define FIRST_SIGNAL 2

int  main(int argc, char** argv) {
	if(argc < 3) {
		panic("Please enter the necessary arguments (m1 [controlFile] [signal])");
	}

	int rank;
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
	if (params == NULL) ALLOC_ERROR;
	if (JSON_read(params, ctrlFile) == 0) {
		panic("cannot read control file");
	}

	if (JSON_getInt(params->tree, "root:realtime") == 1) {
		processRealtimeData(params, station);
	} else {
		processDataFolder(params, station);
	}

	free(directory);
	free(params->tree);
	free(params);
	MPI_Finalize();
	return 0;
}
