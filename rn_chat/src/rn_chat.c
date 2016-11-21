/*
 ============================================================================
 Name			: rn_chat.c
 Author			: Anushavan Melkonyan
 Author			: Alexander Hoffmann
 Version		: 0.1
 Description	: Chat program
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(void) {
	puts("Unser Chat Programm"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}
