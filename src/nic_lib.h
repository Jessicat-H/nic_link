#ifndef NIC_PHYS
#define NIC_PHYS

// message callback function
typedef void (*call_back) (char*);

void sendMessage(int pi, int port, char* str);

//Initialize the pigpio connection and the GPIO modes
void nic_lib_init(call_back userCallback);

#endif
