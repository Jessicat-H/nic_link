#ifndef NIC_LIB
#define NIC_LIB

/**
 * message callback function
 * @param char* the message string
 * @param int the port number
 */ 
typedef void (*call_back) (char*, int);

/**
 * Get the latest message received
 * @return The last message to be fully transmitted over the network
 */
char* receive();

/**
 * Send a message to a port.
 * @param port: the port number
 * @param str: the string to transmit
 */
void sendMessage(int port, char* str);

/**
 * Send a message to all ports.
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
