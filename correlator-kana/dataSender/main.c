#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vdifio.h>
#include <libgen.h>
#include <dirent.h>
#include <strings.h>
#include "../include/panic.h"
#include "../include/socket.h"
#include "../include/jsonReader.h"
#include "../include/utils.h"
#include "../include/directories.h"

int isVDIF(JSON_Tree* params, char* station) {
    char* str = malloc(64);
    sprintf(str, "root:stations:%s:format", station);
    char* format = JSON_getString(params->tree, str);

    if (strcmp(format, "VDIF") == 0) {
        return 1;
    } else {
        return 0;
    }
}

long getDataSize(JSON_Tree* params) {
    long discretization = JSON_getInt(params->tree, "root:discretization(Hz)");
    char* lengthStr = JSON_getString(params->tree, "root:rt_batch_length");
    double length = timeToSecs(lengthStr);
    long size = (long)(length * discretization) / 8;
    return size;
}

char *getDataFilePath(char *station, JSON_Tree* params) {
    char* DATA_DIR = JSON_getString(params->tree, "root:data_dir");
    DIR* directory;
    if ((directory = opendir(DATA_DIR)) == NULL) {
        panic("cannot open data dir");
    }

    char* filename = malloc(64);
    // if (filename == NULL) ALLOC_ERROR;

    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
        if (isKanaFile(entry, DATA_DIR, station)) {
            sprintf(filename, "%s", "a");
            break;
        }
    }

    if (closedir(directory) == -1) {
        perror("closedir");
    }

    char* path = malloc(128);
    sprintf(path, "%s%s", DATA_DIR, filename);
    return path;
}

void sendNirfiData(FILE* file, long size, int socket) {
    fseek(file, 60, SEEK_SET);
    unsigned char* buffer = malloc(size);
    // if (buffer == NULL) ALLOC_ERROR;

    while (fread(buffer, 1, size, file) > 0) {
        Message* msg = malloc(sizeof(Message));
        msg->data = buffer;
        msg->size = size;
        sendToSocket(socket, msg);
        Message* ping = recieveFromSocket(socket);
        freeMessage(ping);
        free(msg);
    }
}

void sendVIDFData(FILE* file, long size, int socket) {

}

int main(int argc, char** argv) {
    if (argc < 3) {
        panic("Please enter the necessary arguments (ds [controlFile] [signal])");
    }

    char* ctrlFile = basename(argv[1]);
    char* directory = getFileDirectory(argv[1]);
    char* station = argv[2];
    chdir(directory);

    // read control file
    JSON_Tree* params = malloc(sizeof(JSON_Tree));
    // if (params == NULL) ALLOC_ERROR;
    if (JSON_read(params, ctrlFile) == 0) {
        panic("cannot read control file");
    }

    // read data file
    long size = getDataSize(params);
    char* dataFilePath = getDataFilePath(station, params);

    FILE* dataFile = fopen(dataFilePath, "rb");
    if (dataFile == NULL) {
        panic("couldn't open data file");
    }

    // get address info
    char* address = JSON_getString(params->tree, "root:rt_in_address");
    printf("Bound to %s\n", address);
    int socket = createServer(address);
    printf("Client has connected.\n");

    if (isVDIF(params, station)) {
        sendVIDFData(dataFile, size, socket);
        panic("VDIF sender not implemented.");
    } else {
        sendNirfiData(dataFile, size, socket);
    }

    close(socket);
    free(directory);
    free(params->tree);
    free(params);
    free(dataFile);
    return 0;
}