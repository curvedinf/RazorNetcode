#ifndef GIT_COMMIT
#define GIT_COMMIT "UNKNOWN"
#endif

#include "networking.h"

unsigned int cur_local_packet_id = 1;

void initializeNetworking() {
	SDLNet_Init();
}

// Returns 0 on success. Otherwise returns the number of the test that failed.
int networking_unitTest() {
	initializeNetworking();
	
	Connection c1, c2;
	
	c1.openSocket(11223);
	if(c1.socket == NULL) return 1;
	
	c2.openSocket(11224);
	if(c2.socket == NULL) return 2;
	
	// Send a small message
	std::string inmsg = "Hello world";
	if(!c2.send("127.0.0.1:11223",inmsg)) return 3;
	
	std::string outhost;
	std::string outmsg;
	
	if(!c1.receive(&outhost, &outmsg)) return 4;
	
	if(outhost != "127.0.0.1:11224") return 5;
	
	if(outmsg != inmsg) return 6;
	
	// Check if duplicate packets are thrown out. Should return false.
	if(c1.receive(&outhost, &outmsg)) return 7;
	
	// Send a large message (lorem ipsum)
	inmsg = R"(
But I must explain to you how all this mistaken idea of denouncing pleasure and praising pain 
was born and I will give you a complete account of the system, and expound the actual teachings 
of the great explorer of the truth, the master-builder of human happiness. No one rejects, 
dislikes, or avoids pleasure itself, because it is pleasure, but because those who do not know 
how to pursue pleasure rationally encounter consequences that are extremely painful. Nor again 
is there anyone who loves or pursues or desires to obtain pain of itself, because it is pain, 
but because occasionally circumstances occur in which toil and pain can procure him some great 
pleasure. To take a trivial example, which of us ever undertakes laborious physical exercise, 
except to obtain some advantage from it? But who has any right to find fault with a man who 
chooses to enjoy a pleasure that has no annoying consequences, or one who avoids a pain that 
produces no resultant pleasure?
)";
	if(!c2.send("127.0.0.1:11223",inmsg)) return 8; // this should send 4 packets
	
	if(!c1.receive(&outhost, &outmsg)) return 9;
	
	if(outhost != "127.0.0.1:11224") return 10;
	
	if(outmsg != inmsg) return 11;
	
	// Check if duplicate packets are thrown out. Should return false.
	if(c1.receive(&outhost, &outmsg)) return 12;
	
	return 0;
}