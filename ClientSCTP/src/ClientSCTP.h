/*
 * ClientSCTP.h
 *
 *  Created on: 19 Dec 2016
 *      Author: networker
 */

#ifndef CLIENTSCTP_H_
#define CLIENTSCTP_H_
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
#include <unistd.h>
#include <netinet/sctp.h>

#define PORT 9012
#define IP "127.0.0.1"
#define NAME_SIZE 15
#define IP_SIZE 16
#define PROTOCOL_VERSION 1
#define MSG_NAME_SIZE 16
#define COMAND_SIZE 20

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
	char quellbenutzername[MSG_NAME_SIZE];
	char zielbenutzername[MSG_NAME_SIZE];
	char nachricht[255];
};

struct Message {
	struct CommonHeader commonHeader;
	struct MessageBody messageBody;
};

enum typ {
	LOG_IN_OUT = 1,
	CONTROL_INFO = 2,
	MESSAGE = 3
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

void *Client(void*);
void logIn(char*);
void logOut(char*);
void loadInfo(void);
void closeProgram(char*);
void sendMessage(char*,char*,char*);
void connectToServer(char*);
void command(void);
void createHeader(struct CommonHeader*,uint8_t,uint8_t,uint8_t,uint8_t);
void receiveMessage(int);
void getUserNames(int);
#endif /* CLIENTSCTP_H_ */
