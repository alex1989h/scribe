/*
 * Client.h
 *
 *  Created on: 29.11.2016
 *      Author: bs
 */

#ifndef CLIENT_H_
#define CLIENT_H_
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

#define PORT 9012
#define IP "127.0.0.1"
#define NAME_SIZE 15

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
	char quellbenutzername[16];
	char zielbenutzername[16];
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
#endif /* CLIENT_H_ */
