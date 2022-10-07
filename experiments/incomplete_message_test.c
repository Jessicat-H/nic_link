#include <pigpiod_if2.h>
#include <unistd.h>
#include <stdint.h>
#include "../src/nic_lib.h"
#include <stdio.h>

static int in_array[]={26,24,22,20};
static int out_array[] = {27,25,23,21};

// don't do anything on receive
void messageReceived(uint8_t* message, int port){
}

// run the multichat program first
// then run this program
int main(void){
	printf("initializing\n");
	
    int pi = pigpio_start(NULL,NULL);
    for(int i=0;i<4;i++){
	set_mode(pi, out_array[i], PI_OUTPUT);
	set_mode(pi, in_array[i], PI_INPUT);
	gpio_write(pi,out_array[i],1);
    }
    //nic_lib_init(messageReceived);
    printf("initialized\n");
    printf("message to send: {length}2-df\n");
    printf("sending first half\n");
    int32_t pins=INT32_MAX;
    const uint32_t expected_dt = 2000;
    // send half of a message: "{length}2-" "df";
    // initialize wave
	wave_clear(pi);
	// 3 pulses for the header, 2 for each bit, length+1 bytes 
	gpioPulse_t pulses[3+(16*3)];
	// setup the header
	int i=0;
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
	pulses[i].usDelay=expected_dt/2;
	i++;
	pulses[i].gpioOn=0;
	pulses[i].gpioOff=pins;
	pulses[i].usDelay=expected_dt;
	i++;
	pulses[i].gpioOn=pins;
	pulses[i].gpioOff=0;
	pulses[i].usDelay=expected_dt/2;
	i++;
	printf("added the header;now adding bytes\n");
    // add the bytes
    	
	for(int z=0;z<3;z++){

		printf("z=%d\n",z);
		// if j=0, add the size
		uint8_t data = 5;
		// otherwise, add the byte
		if(z==1){
			data = '2';
		}else if(z==2){
           		 data = '-';
        	}
		printf("encoding %d \n",data);
		// encode each bit
		for (uint8_t m=0b10000000;m>0;m>>=1) {
			printf("bit: %d\n",data&m);
			/*If 1, encodes:
			*   .--
			*   |
			* --.
			*/
			if (data&m) {
					pulses[i].gpioOn =0;
					pulses[i].gpioOff=pins;
					pulses[i].usDelay=expected_dt/2;
					i++;
					pulses[i].gpioOn =pins;
					pulses[i].gpioOff=0;
					pulses[i].usDelay=expected_dt/2;
			}
			/*If 0, encodes:
					* --.
					*   |
					*   .--
					*/
			else {
					pulses[i].gpioOn =pins;
					pulses[i].gpioOff=0;
					pulses[i].usDelay=expected_dt/2;
					i++;
					pulses[i].gpioOn =0;
					pulses[i].gpioOff=pins;
					pulses[i].usDelay=expected_dt/2;

			}
		
			i++;
		}
	}
	// send wave
	printf("sending wave \n");
	wave_add_generic(pi,i,pulses);
	int wave = wave_create(pi);
	wave_send_once(pi,wave);
	// wait for wave to send; 2 dt's for header, 8 for each byte, add some for error
	usleep(expected_dt*(2+(8*3)+1));
	printf("adding lag\n");
    // add some lag
    usleep(expected_dt*3);

    // send the rest of the message
    printf("sending rest of message\n");
    // initialize wave
	wave_clear(pi);
	// 3 pulses for the header, 2 for each bit, length+1 bytes 
	gpioPulse_t new_pulses[(16*2)];
	// setup the header
	i=0;
    // add the bytes
	for(uint8_t j=0;j<2;j++){
		// if j=0, add the size
		uint8_t data = 5;
		// otherwise, add the byte
		if(j=0){
			data = 'd';
		}else if(j=1){
            data = 'f';
        }
		printf("sending %d \n",data);
		// encode each bit
		for (uint8_t m=0b10000000;m>0;m>>=1) {
			/*If 1, encodes:
			*   .--
			*   |
			* --.
			*/
			if (data&m) {
					new_pulses[i].gpioOn =0;
					new_pulses[i].gpioOff=pins;
					new_pulses[i].usDelay=expected_dt/2;
					i++;
					new_pulses[i].gpioOn =pins;
					new_pulses[i].gpioOff=0;
					new_pulses[i].usDelay=expected_dt/2;
			}
			/*If 0, encodes:
					* --.
					*   |
					*   .--
					*/
			else {
					new_pulses[i].gpioOn =pins;
					new_pulses[i].gpioOff=0;
					new_pulses[i].usDelay=expected_dt/2;
					i++;
					new_pulses[i].gpioOn =0;
					new_pulses[i].gpioOff=pins;
					new_pulses[i].usDelay=expected_dt/2;

			}
		
			i++;
		}
	}
	printf("sending wave\n");
	// send wave
	wave_add_generic(pi,i,new_pulses);
	wave = wave_create(pi);
	wave_send_once(pi,wave);
	// wait for wave to send; 2 dt's for header, 8 for each byte, add some for error
	usleep(expected_dt*((8*2)+1));
    

	printf("sending complete message \n");
    // send a complete message
    uint8_t msg[4];
    msg[0]=1;
    msg[1]=':';
    msg[2]=' ';
    msg[3]='1';
    sendMessage(4,msg,4);
}
