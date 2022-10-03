#include <pigpiod_if2.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "send_lib.h"

// message callback function
typedef void (*call_back) (char*);


// the associated pins for each port
static int in_array[] = {26, 24, 22, 20};
static int out_array[] = {27, 25, 23, 21};


/*
	Convert a GPIO read pin to the NIC port number
	@param user_gpio - the pin number to convert
*/
int gpio_to_port(unsigned user_gpio){
	if(user_gpio==26){
		return 0;
	}else if(user_gpio==24){
		return 1;
	}else if(user_gpio==22){
		return 2;
	}else if(user_gpio==20){
		return 3;		
	}
	return 4;
}

//tracking variables

//boolean for if a short pulse has already happened
uint8_t pulseOccured[] = {0,0,0,0};
// whether the callibration header has occurred
uint8_t hsOccured[] = {0,0,0,0};
// last pulse time
int lastPulseTick[] = {0,0,0,0};
// delay in us
uint32_t delay[] = {100000,100000,100000,100000};
// margin in us
uint32_t marginError[] = {5000,5000,5000,5000};
// byte output buffer
uint8_t output[4][8];
// number of bits 
int bitNum[] = {0,0,0,0};
//character buffers for message
char charBuffer[4][128];
//position within the character buffer
int bufferPos[] = {0,0,0,0};

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
	add a byte to a port's byte buffer
	@param port - which port to add a byte to
*/
void addByte(int port, call_back callback){
	unsigned char byte = 0b00000000;
	for(int i = 0; i < 8; i++) {
		byte += output[port][i] << (7-i);
	}
	charBuffer[port][bufferPos[port]] = byte;
	bufferPos[port]++;
	printf("Message so far: %s, need to hit %d chars\n",charBuffer[port],charBuffer[port][0]);
	// check to see if message received
	if(charBuffer[port][0]+1==bufferPos[port]){
		char message[bufferPos[port]+1];
		for(int i=0;i<bufferPos[port];i++){
			message[i]=charBuffer[port][i+1];
		}
		// add null character
		message[bufferPos[port]+1]='\0';
		callback(message);
		// clear char buffer
		for (int c = 0; c < 128; c++) {
			charBuffer[port][c] = '\0';
		}
		bufferPos[port] = 0;
	}
}

/*
	detect a change on a gpio pin; associate it with a port, and carry out Manchester encoding, updating port information as needed
	@param pi - the pigpio pi reference
	@param user_gpio - the gpio pin
	@param level - 0 = change to low (a falling edge) 1 = change to high (a rising edge)
	@param tick - # of microseconds since boot, wraps every 72 minutes
*/
void changeDetected(int pi, unsigned user_gpio, unsigned level, uint32_t tick) {
	int port = gpio_to_port(user_gpio);
	int dT;
	// get the amount of time since last tick
	if (tick > lastPulseTick[port]) {
		dT = tick - lastPulseTick[port];
	}
	else {
		// account for rollover
		dT = (4294967295-lastPulseTick[port]) + tick;
	}
	//printf("DT: %d\n",dT);

	if (dT>delay[port]-marginError[port] && dT < delay[port]+marginError[port]) {
		//long pulse
	//	printf("Long pulse\n");
		if(!hsOccured[port]) {
			if (level) { //if the level is 1, this is not the header
				// this is the header; get the long pulse time
				hsOccured[port]=1;
				printf("Updating delay to: %d\n",dT);
				delay[port] = dT;
			}
		}
		else {
			// add a bit
			output[port][bitNum[port]] = level;
			bitNum[port]++;
		}
	}
	// check margins
	else if (dT>(delay[port]-marginError[port])/2 && dT < (delay[port]+marginError[port])/2) {
		//short pulse
		if(hsOccured[port]){ // ignore if haven't received header yet
		//	printf("short pulse\n");
			if(!pulseOccured[port]) {
				//discard this pulse
				pulseOccured[port] = 1;
			}
			else {
				// add bit
				output[port][bitNum[port]] = level;
				bitNum[port]++;
				pulseOccured[port] = 0;
			}
		}
	}
	//if the delay doesn't look like half a delay OR a whole delay, start the message over
	/*else if(dT > (delay[port] * 2)) {
		printf("%s\n",charBuffer[port]);
		//flush the byte to be able to accept the next byte
		for (int i=0; i<8; i++) {
			output[port][i] = 0;
		}
		printf("New message?\n");
		bitNum[port] = 0;
		hsOccured[port] = 0;
		pulseOccured[port] = 0;
		for (int c = 0; c < 128; c++) {
			charBuffer[port][c] = '\0';
		}
		bufferPos[port] = 0;
	}*/
	lastPulseTick[port] = tick;

	if(bitNum[port] == 8) {
		//for (int i=0;i<bitNum[port];i++) {
		//	printf("Bit %d: %d\n",i,output[port][i]);
		//}
		addByte(port,messageReceived);
		//flush the byte to be able to accept the next byte
		for (int i=0; i<8; i++) {
			output[port][i] = 0;
		}
		bitNum[port] = 0;
		hsOccured[port] = 0;
		pulseOccured[port] = 0;
	}
	

}

void sendMessage(int pi, int port, char* str) {
	// send size
	char length = strlen(str);
	printf("Sending %s\n",str);
	sendChar(pi, (char)length, delay[port], out_array[port]);
	usleep(delay[port]*11);
	// send message
	for(int i=0;i<length;i++){
		sendChar(pi, str[i], delay[port], out_array[port]);
		usleep(delay[port]*11);	
		//i++;
	}

}

/*
	Listen for and send messages
*/
int main() {
	// get username
	char uname[10];
	printf("Enter your username (max 8 chars): ");
	fgets(uname, 10, stdin);
	printf("\n");

	// take out the new line character from username
	for(int i = 0; i<10;i++){
		if(uname[i]=='\n'){
			uname[i]='\0';
			break;
		}
	}

	printf("Username: %s\n", uname);
	// start up gpio
	int pi = pigpio_start(NULL,NULL);

	// add callback to each in pin
	for(int i = 0; i<4; i++){
		gpio_write(pi, out_array[i],1);
		int success = callback(pi, in_array[i], EITHER_EDGE, &changeDetected);
	}

	printf("Enter a message and hit enter to send it. Messages must be under 117 characters.\n");
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
		for(int i =0;i<4;i++){
			sendMessage(pi,i, message);
		}
    }	

}
