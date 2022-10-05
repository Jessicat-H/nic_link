#include <pigpiod_if2.h>
#include <stdint.h>
#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include "nic_lib.h"

///////// consts

//define ports. broadcom nums. port 1 is idx 0.
static const int IN_ARRAY[] = {26, 24, 22, 20};
static const int OUT_ARRAY[] = {27, 25, 23, 21};
// the expected delay
const uint32_t EXPECTED_DT = 2000;

///////// library vars

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
uint32_t delay[] = {EXPECTED_DT,EXPECTED_DT,EXPECTED_DT,EXPECTED_DT};
// margin in us
uint32_t marginError[] = {EXPECTED_DT/4,EXPECTED_DT/4,EXPECTED_DT/4,EXPECTED_DT/4};
// byte output buffer
uint8_t output[4][8];
// number of bits 
int bitNum[] = {0,0,0,0};
//character buffers for message
uint8_t charBuffer[4][MESSAGE_SIZE];
//position within the above buffer
int bufferPos[] = {0,0,0,0};
//latest message received
uint8_t latestMessage[MESSAGE_SIZE];


/*
 * Send one character over a specified GPIO pin in Manchester encoding 
 * with a header for synchornization
 * pi - result returned from pigpio_start(NULL, NULL)
 * c - character to transmit
 * dT - Interval that corresponds to a long pulse. Must match between sender and reciever
 * pinOut - GPIO pin to send the message on (Broadcom numbers); if INT32_MAX send to all
 */
int sendChar(int pi, uint8_t c, int dT, int32_t pinOut) {
	int i=0;
	gpioPulse_t pulses[8*2+3];	

	// by default blast to all
	int32_t pins=INT32_MAX;	
	// otherwise, send to individual port
	if(pinOut!=INT32_MAX){
		1<<pinOut;
	}

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
	pulses[i].gpioOn=pins;
	pulses[i].gpioOff=0;
	pulses[i].usDelay=dT/2;
	i++;
	pulses[i].gpioOn=0;
	pulses[i].gpioOff=pins;
	pulses[i].usDelay=dT;
	i++;
	pulses[i].gpioOn=pins;
	pulses[i].gpioOff=0;
	pulses[i].usDelay=dT/2;
	i++;
	for (uint8_t m=0x80;m>0;m>>=1) {
		/*Always encodes:
		 *   .--
		 *   |
		 * --.
		 */
		if (c&m) {
				pulses[i].gpioOn =0;
				pulses[i].gpioOff=pins;
				pulses[i].usDelay=dT/2;
				i++;
				pulses[i].gpioOn =pins;
                                pulses[i].gpioOff=0;
                                pulses[i].usDelay=dT/2;
		}
		/*Always encodes:
                 * --.
                 *   |
                 *   .--
                 */
		else {
				pulses[i].gpioOn =pins;
				pulses[i].gpioOff=0;
				pulses[i].usDelay=dT/2;
				i++;
				pulses[i].gpioOn =0;
				pulses[i].gpioOff=pins;
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

/**
 * Get the latest message received
 * @return The last message to be fully transmitted over the network
 */
uint8_t* receive() {
	return(&latestMessage[0]);
}

/*
	add a byte to a port's byte buffer
	@param port - which port to add a byte to
*/
void addByte(int port){
	unsigned uint8_t byte = 0b00000000;
	for(int i = 0; i < 8; i++) {
		byte += output[port][i] << (7-i);
	}
	charBuffer[port][bufferPos[port]] = byte;
	bufferPos[port]++;
	// check to see if message received
	if(charBuffer[port][0]+1==bufferPos[port]){
		uint8_t message[bufferPos[port]];
		// copy over the charBuffer to our message holder
		for (int i = 0; i < bufferPos[port]; i++) {
			message[i] = charBuffer[port][i];
		}
		//strncpy(message, charBuffer[port],bufferPos[port]);
		// send the message to the callback
		msgCallback(message,port);
		// update latest message
		latestMessage = message;
		//strcpy(latestMessage,message);
		// clear char buffer - theoretically don't need this; uncomment if things break
		// for (int c = 0; c < MESSAGE_SIZE; c++) {
		// 	charBuffer[port][c] = '\0';
		// } 
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

	if (dT>delay[port]-marginError[port] && dT < delay[port]+marginError[port]) {
		//long pulse
		if(!hsOccured[port]) {
			if (level) { //if the level is 1, this is not the header
				// this is the header; get the long pulse time
				hsOccured[port]=1;
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
	lastPulseTick[port] = tick;

	if(bitNum[port] == 8) {
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

/**
 * Send a message to all ports.
 * @param str: the string to transmit
 * @param length: the length of the message
 */
void broadcast(uint8_t* str, uint8_t length) {
	sendMessage(4,str, length);
}

/**
 * Send a message to a port.
 * @param port: the port number; 4 if to all
 * @param str: the string to transmit
 * @param length: the length of the message
 */
void sendMessage(int port, uint8_t* str, uint8_t length) {
	// the gpio pin to output to
	// by default, sends to all
	int32_t gpio=INT32_MAX;
	if(port<4){
		gpio=OUT_ARRAY[port];
	}
	// send size
	sendChar(pi, length, EXPECTED_DT, gpio);
	usleep(EXPECTED_DT*11);
	// send message
	for(int i=0;i<length;i++){
		sendChar(pi, str[i], EXPECTED_DT, gpio);
		usleep(EXPECTED_DT*11);	
	}

}

/**
 * Initialize the pigpio connection and the GPIO modes
 * @param userCallback: callback function that will be called once a message has been transmitted,
 * with the message as a parameter
 */
void nic_lib_init(call_back userCallback) {
	pi = pigpio_start(NULL,NULL);
	msgCallback = userCallback;
	// set modes using a loop
	for (int i = 0; i<4; i++) {
		set_mode(pi, OUT_ARRAY[i], PI_OUTPUT);
		set_mode(pi, IN_ARRAY[i], PI_INPUT);
		// blip a 1 to set up port
		gpio_write(pi, OUT_ARRAY[i],1);
		// add the receive callback to get a change
		callback(pi, IN_ARRAY[i], EITHER_EDGE, &changeDetected);
	}
}



