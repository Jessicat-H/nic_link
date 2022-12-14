#ifndef NIC_LIB
#define NIC_LIB

/**
 * message callback function
 * @param uint8_t* the message string
 * @param int the port number
 */ 
typedef void (*call_back) (uint8_t*, int);

/**
 * Get the latest message received
 * @return The last message to be fully transmitted over the network
 */
uint8_t* receive();

/**
 * Send a message to a port.
 * @param port: the port number
 * @param str: the string to transmit
 * @param length: the length of the message
 */
void sendMessage(int port, uint8_t* str, uint8_t length);

/**
 * Send a message to all ports.
 * @param str: the string to transmit
 * @param length: the length of the message
 */
void broadcast(uint8_t* str, uint8_t length);

/**
 * Initialize the pigpio connection and the GPIO modes
 * @param userCallback: callback function that will be called once a message has been transmitted,
 * with the message as a parameter
 */
void nic_lib_init(call_back userCallback);


#endif
