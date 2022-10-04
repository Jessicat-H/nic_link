#ifndef NIC_LIB
#define NIC_LIB

/**
 * message callback function
 * @param char* the message string
 */ 
typedef void (*call_back) (char*);

/**
 * Get the latest message received
 * @return The last message to be fully transmitted over the network
 */
char* receive();

/**
 * Send a message to a port.
 * @param pi: the pi int object returned by pigpio
 * @param port: the port number
 * @param str: the string to transmit
 */
void sendMessage(int port, char* str);

/**
 * Send a message to all ports.
 * @param pi: the pi int object returned by pigpio
 * @param str: the string to transmit
 */
void broadcast(char* str);

/**
 * Initialize the pigpio connection and the GPIO modes
 * @param userCallback: callback function that will be called once a message has been transmitted,
 * with the message as a parameter
 */
void nic_lib_init(call_back userCallback);


#endif
