#include <pigpiod_if2.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include<string.h>
#include "nic_lib.h"

/*
	call back for receiving a message
	@param message - the message received
*/
void messageReceived(char* message){
	printf("\n");
	printf("%s\n",  message);
	printf("\n");
}

/*
	Listen for and send messages
*/
int main() {
	nic_lib_init(messageReceived);

	char uname[10];
	printf("Enter your username (max 8 chars): ");
	fgets(uname, 10, stdin);
	printf("\n");

	// take out the newline character from username
	for(int i = 0; i<10;i++){
		if(uname[i]=='\n'){
			uname[i]='\0';
			break;
		}
	}

	printf("Enter a message and hit enter to send it. Messages must be under 117 characters; additional characters will be truncated.\n");
	char holder[117];

	while(1){
		// just keep waiting for messages and callbacks
        fgets(holder, 117, stdin);
		// make the message
		char message[128];
		// add the username
		strcpy(message, uname);
		// add the : 
		strncat(message, ": ", 2);
		// take out the newline character from holder
		for(int i = 0; i<117;i++){
			if(holder[i]=='\n'){
				holder[i]='\0';
				break;
			}
		}
		// add the typed message
		strncat(message, holder, 117);

		broadcast(message);
		
    }	

}
