#ifndef KANA_SOCKET_H
#define KANA_SOCKET_H

typedef struct {
    unsigned char* data;
    long size;
} Message;
void freeMessage(Message* msg);

int createServer(char* addr);
int connectToServer(char* addr);
void disconnectFromServer(int s);

void sendToSocket(int s, Message* msg);
Message* recieveFromSocket(int s);
void touchSocket(int s);

#endif //KANA_SOCKET_H
