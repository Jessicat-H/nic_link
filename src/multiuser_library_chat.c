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
	printf("Message received: %s\n",  message);
	printf("\n");
}


/*
	Listen for and send messages
*/
int main() {
	int pi = pigpio_start(NULL,NULL);

	printf("Enter a message and hit enter to send it. Messages must be under 128 characters.\n");
	char holder[128];
	while(1){
		// just keep waiting for messages and callbacks
        fgets(holder, 128, stdin);
		for(int i =0;i<4;i++){
			sendMessage(pi,i, holder);
		}
    }	

}
