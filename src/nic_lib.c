#include <pigpiod_if2.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "nic_lib.h"

//define ports. broadcom nums. port 1 is idx 0.
static int in_array[] = {26, 24, 22, 20};
static int out_array[] = {27, 25, 23, 21};
// pigpio pi reference
static int pi;
//function to call after msg recieved
call_back msgCallback;

////////tracking variables

//boolean for if a short pulse has already happened
uint8_t pulseOccured[] = {0,0,0,0};
// whether the callibration header has occurred
uint8_t hsOccured[] = {0,0,0,0};
// last pulse time
int lastPulseTick[] = {0,0,0,0};
// delay in us
uint32_t delay[] = {50000,50000,50000,50000};
// margin in us
uint32_t marginError[] = {5000,5000,5000,5000};
// byte output buffer
uint8_t output[4][8];
// number of bits 
int bitNum[] = {0,0,0,0};
//character buffers for message
char charBuffer[4][128];
//position within the above buffer
int bufferPos[] = {0,0,0,0};
// buffer for completed messages
char messageBuffer[4][32][128];
// keep track of how many messages in each buffer
int messageNum[] = {0,0,0,0};


/*
 * Send one character over a specified GPIO pin in Manchester encoding 
 * with a header for synchornization
 * pi - result returned from pigpio_start(NULL, NULL)
 * c - character to transmit
 * dT - Interval that corresponds to a long pulse. Must match between sender and reciever
 * pinOut - GPIO pin to send the message on (Broadcom numbers)
 */
int sendChar(int pi, char c, int dT, int pinOut) {
	char data = c;
	//100000
	//010000
	//00100
	char first = !(0x80&data);
	int i=0;
	gpioPulse_t pulses[8*2+3];	
	//add handshake
	/* If signal starts high:
	 * ----.    .--data starts
	 *   . |    |
	 *   . |    |
	 *   . .----.
	 * If signal starts low:
	 *     .--.    .--data starts
	 *     |  |    |
	 *     |  |    |
	 * ----.  .----.
	 */
	pulses[i].gpioOn=1<<pinOut;
	pulses[i].gpioOff=0;
	pulses[i].usDelay=dT/2;
	i++;
	pulses[i].gpioOn=0;
	pulses[i].gpioOff=1<<pinOut;
	pulses[i].usDelay=dT;
	i++;
	pulses[i].gpioOn=1<<pinOut;
	pulses[i].gpioOff=0;
	pulses[i].usDelay=dT/2;
	i++;
	for (char m=0x80;m>0;m>>=1) {
		/*Always encodes:
		 *   .--
		 *   |
		 * --.
		 */
		if (data&m) {
				pulses[i].gpioOn =0;
				pulses[i].gpioOff=1<<pinOut;
				pulses[i].usDelay=dT/2;
				i++;
				pulses[i].gpioOn =1<<pinOut;
                                pulses[i].gpioOff=0;
                                pulses[i].usDelay=dT/2;
		}
		/*Always encodes:
                 * --.
                 *   |
                 *   .--
                 */
		else {
				pulses[i].gpioOn =1<<pinOut;
                                pulses[i].gpioOff=0;
                                pulses[i].usDelay=dT/2;
				i++;
				pulses[i].gpioOn =0;
                                pulses[i].gpioOff=1<<pinOut;
                                pulses[i].usDelay=dT/2;

		}
	
		i++;
	}
       	wave_clear(pi);	
	wave_add_generic(pi,i,pulses);
	int wave = wave_create(pi);
	
	wave_send_once(pi,wave);
	return(0);
}

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

/*
	add a byte to a port's byte buffer
	@param port - which port to add a byte to
*/
void addByte(int port){
	unsigned char byte = 0b00000000;
	for(int i = 0; i < 8; i++) {
		byte += output[port][i] << (7-i);
	}
	charBuffer[port][bufferPos[port]] = byte;
	bufferPos[port]++;
	printf("Message so far: %s, need to hit %d chars\n",charBuffer[port],charBuffer[port][0]);
	// check to see if message received
	if(charBuffer[port][0]==bufferPos[port]){
		char message[bufferPos[port]+1];
		for(int i=0;i<bufferPos[port];i++){
			message[i]=charBuffer[port][i+1];
		}
		// add null character
		message[bufferPos[port]+1]='\0';
		msgCallback(message);
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
	printf("DT: %d\n",dT);

	if (dT>delay[port]-marginError[port] && dT < delay[port]+marginError[port]) {
		//long pulse
		printf("Long pulse\n");
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
			printf("short pulse\n");
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
		for (int i=0;i<bitNum[port];i++) {
			printf("Bit %d: %d\n",i,output[port][i]);
		}
		addByte(port);
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
	printf("Sending %d\n",length);
	sendChar(pi, (char)length, delay[port], out_array[port]);
	usleep(delay[port]*11);
	// send message
	int i = 0;
	while(str[i] != '\n') {
		sendChar(pi, str[i], delay[port], out_array[port]);
		usleep(delay[port]*11);	
		i++;
	}

}

//Initialize the pigpio connection and the GPIO modes
void nic_lib_init(call_back userCallback) {
	pi = pigpio_start(NULL,NULL);
	msgCallback = userCallback;
	//set modes using a loop
	for (int i = 0; i<4; i++) {
		set_mode(pi, out_array[i], PI_OUTPUT);
		set_mode(pi, in_array[i], PI_INPUT);
		gpio_write(pi, out_array[i],1);
		callback(pi, in_array[i], EITHER_EDGE, &changeDetected);
	}
}



