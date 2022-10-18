#include <pigpiod_if2.h>
#include <unistd.h>
#include <stdint.h>
#include "../src/nic_lib.h"
#include <stdio.h>
#include <string.h>

uint8_t msg[] = "The quick brown fox jumped over the lazy dog! 0123456789";
int GoodMsgRcvd = 0;
int BadMsgRcvd = 0;

void messageReceived(uint8_t* message, int port){
    printf("Received: %s \n", &msg[1]);
    // ditch the length and compare the messages
    if (!strcmp(&message[1], msg))
    { // recieved is the same as sent
        GoodMsgRcvd++;
    }
    else
    {
        BadMsgRcvd++;
    }

    printf("Recieved %d good and %d bad\n", GoodMsgRcvd, BadMsgRcvd);
}

// run the multichat program first
// then run this program
int main(void){
	nic_lib_init(messageReceived);
    int numMessages = 0;
    while(1){
        numMessages+=1;
        printf("Sending message #%d\n", numMessages);
        broadcast(msg, 57);
    }
}
