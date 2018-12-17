#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "socket.h"
#include "panic.h"
#include "utils.h"

static Message* recvall(int s, long n);

int createServer(char* addr) {
  char** split = splitString(addr, ':');
  char* address = split[0];
  unsigned short port = atoi(split[1]);

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    panic("Cannot create socket");
  }

  // set timeout to 1 second
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

  // setup server
  struct hostent* host = gethostbyname(address);
  if (host == NULL) {
    panic("Cannot get host by name");
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = *((unsigned long*)host->h_addr_list[0]);

  if (bind(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
    panic("cannot bind server address");
  }

  // wait for client to connect
  listen(s, 1);
  int addlen = sizeof(server);
  int client = accept(s, (struct sockaddr*)&server, (socklen_t*)&addlen);

  return client;
}

int connectToServer(char* addr) {
  char** split = splitString(addr, ':');
  char* address = split[0];
  unsigned short port = atoi(split[1]);

  struct hostent* host = gethostbyname(address);
  if (host == NULL) {
    panic("Cannot get host by name");
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = *((unsigned long*)host->h_addr_list[0]);

  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    panic("Cannot create socket");
  }

  // set timeout to 1 second
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

  if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
    panic("Cannot connect to server");
  }

  free(split[0]);
  free(split[1]);
  free(split);

  return s;
}

void disconnectFromServer(int s) {
  close(s);
}

void sendToSocket(int s, Message* msg) {
  union {
    unsigned char bytes[8];
    long number;
  } longUnion;
  longUnion.number = msg->size;

  if (send(s, &longUnion.bytes[0], 8, 0) < 0)
    panic("Cannot send size to server");
  if (send(s, msg->data, msg->size, 0) < 0)
    panic("Cannot send buffer to server");
}

Message* recieveFromSocket(int s) {
  Message* rawLen = recvall(s, 8);
  if (rawLen == NULL) return NULL;
  long len = *((long*)rawLen->data);
  freeMessage(rawLen);
  return recvall(s, len);
}

void touchSocket(int s) {
  unsigned char buffer[1];
  Message* msg = malloc(sizeof(Message));
  msg->data = buffer;
  msg->size = 1;
  sendToSocket(s, msg);
  free(msg);
}

static Message* recvall(int s, long n) {
  unsigned char* data = malloc(n);
  if (data == NULL) ALLOC_ERROR;

  long filled = 0;
  while (filled < n) {
    unsigned char* packet = malloc(n-filled);
    memset(packet, 0, n-filled);
    long bytesrecvd = recv(s, packet, n-filled, 0);
    if (bytesrecvd < 1) {
      return NULL;
      free(data);
    }; // disconnected
    memcpy(data+filled, packet, bytesrecvd);
    filled += bytesrecvd;
  }
  Message* msg = malloc(sizeof(msg));
  msg->data = data;
  msg->size = n;
  return msg;
}

void freeMessage(Message* msg) {
  free(msg->data);
  free(msg);
}
