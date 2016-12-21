/*
 * Server.h
 *
 *  Created on: 29.11.2016
 *      Author: bs
 */

#ifndef SERVER_H_
#define SERVER_H_
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_LISTEN_QUEUE 10
#define ERROR(VALUE,TEXT) if (VALUE == -1) {\
			printf(TEXT);\
		}

#define PORT 9012
#define IP "127.0.0.1"
#define NAME_SIZE 15
#define COMAND_SIZE 20
#define IP_SIZE 16
#define PROTOCOL_VERSION 1
#define MSG_NAME_SIZE 16

struct CommonHeader {
	uint8_t type;
	uint8_t flag;
	uint8_t version;
	uint8_t lenght;
};

struct LogInOutBody {
	char benutzername[NAME_SIZE];
};

struct LogInOut {
	struct CommonHeader commonHeader;
	struct LogInOutBody logInOutBody;
};

struct Tabelle {
	char benutzername[16];
	int hops;
};

struct ControlInfoBody{
	struct Tabelle tabelle[255];
};

struct ControlInfo {
	struct CommonHeader commonHeader;
	struct ControlInfoBody controlInfoBody;
};

struct MessageBody {
	char quellbenutzername[NAME_SIZE];
	char zielbenutzername[NAME_SIZE];
	char nachricht[255];
};

struct Message {
	struct CommonHeader commonHeader;
	struct MessageBody messageBody;
};

enum typ {
	LOG_IN_OUT = 1,
	CONTROL_INFO = 2,
	MESSAGE = 3,
	CONNECT = 42
};
//FLAGS
enum flag {
	UNDEFINE = 0,
	SYN = 1,
	ACK = 2,
	FIN = 4,
	DUP = 8,
	GET = 16
};

struct ConnectionInfo{
	char name[NAME_SIZE];
	int socketFD;
	int hops;
};

void* Server(void*);
void newConnection(void);
void receivePackages(int);
void passMessage(int,int);
void createHeader(struct CommonHeader*,uint8_t,uint8_t,uint8_t,uint8_t);
void connectToServer(char*);
void sendControlInfo(int,uint8_t);
void notifyAllServers(int);
void deleteEntry(int);
void commands(void);
void putNewServer(int);
int deleteServer(int);
void logInRequest(int,int);
void logOutRequest(int,int);
int sucheSocketFD(char*);
void getControlInfo(int,int);
void verbindungTrennen(int);
void connectToMyself(void);
int createNewBody(int,struct ControlInfoBody*);
int isMyClient(char*);
void zeigeTabelle(void);
int deleteEntriesBelongToDestination(int,struct ControlInfoBody*);
#endif /* SERVER_H_ */
