#pragma once
// This networking library provides extra features on top of UDP
// including multi-packet messages, and multi-send/duplicate elmination.
// It utilizes SDL_net for cross-platform UDP and has a unit test.
// Host addresses and messages are in std::string format for easier
// usability.

#include <SDL2/SDL_net.h>
#include <vector>
#include <string>
#include <exception>
#include <chrono>
#include <deque>
#include <unordered_map>

#include "serialization.h"
#include "misc.h"

#define CONNECTION_ANY "ANY"

#define PACKET_MAX_SIZE 508
#define PACKET_SEGMENT_MAX_SIZE PACKET_MAX_SIZE-12

#define NETWORKING_MAX_DUPLICATE_WAIT 10 * NANOS_PER_SECOND

extern unsigned int cur_local_packet_id;

// Inferno's packet
// Structure:
// - ID 4 bytes
// - NUM_SEGMENTS or MULTIPART 1 byte
// <for each segment>
// - SEGMENT_SIZE 1 byte
// - SEGMENT_DATA up to 256 bytes
class Packet {
public:
	struct Segment {
		unsigned short length;
		void* data;
	};
	
	unsigned int id;
	std::vector<Segment> segments;
	
	Packet() {
	}
	
	void freeSegments() {
		for(int i=0; i<this->segments.size(); i++) {
			delete [] (char*)(this->segments[i].data);
		}
		this->segments.clear();
	}
	
	~Packet() {
		this->freeSegments();
	}
	
	void assignID() {
		this->id = cur_local_packet_id;
		cur_local_packet_id++;
	}
	
	unsigned short length() {
		unsigned short len = 4+1; // id + num_segments
		for(int i=0; i<this->segments.size(); i++) {
			len += this->segments[i].length; // segment data
			len += 2; // segment length
		}
		return len;
	}
	
	unsigned char num_segments() {
		return this->segments.size();
	}
	
	void addSegment(const void* data, unsigned short length) {
		Segment s;
		s.data = new char[length];
		s.length = length;
		memcpy(s.data, data, length);
		this->segments.push_back(s);
	}
	
	UDPpacket* serialize() {
		UDPpacket *udp_packet;
		int len = this->length();
		udp_packet = SDLNet_AllocPacket(len);
		udp_packet->len = len;
		int pos = 0;
		pos += copyInI(udp_packet->data, pos, this->id);
		pos += copyInC(udp_packet->data, pos, this->num_segments());
		for(int i=0; i<this->segments.size(); i++) {
			unsigned short segment_length = this->segments[i].length;
			pos += copyInS(udp_packet->data, pos, segment_length);
			pos += copyInData(udp_packet->data, pos, this->segments[i].data, segment_length);
		}
		return udp_packet;
	}
	
	void deserialize(UDPpacket* udp_packet) {
		this->freeSegments();
		int pos = 0;
		pos += copyOutI((int*)&(this->id), udp_packet->data, pos);
		unsigned char num_segments;
		pos += copyOutC((char*)&num_segments, udp_packet->data, pos);
		for(int i=0; i<num_segments && pos < udp_packet->len; i++) {
			Segment segment;
			pos += copyOutS((short*)&(segment.length), udp_packet->data, pos);
			segment.data = new char[segment.length];
			pos += copyOutData(segment.data, udp_packet->data, pos, segment.length);
			this->segments.push_back(segment);
		}
	}
};


// Handles SDL_net channels, resolving IPs, 
class Connection {
public:
	unsigned short port;
	std::string remote_host_and_port;
	
	UDPsocket socket;
	std::unordered_map<std::string, int> channels;
	unsigned int next_channel;
	
	std::unordered_map<std::string, unsigned long long> received_uids;
	
	struct DirectedMessage {
		std::string host_and_port;
		std::string message;
	};
	std::deque<DirectedMessage> received_messages;
	
	// string is first packet UID of the multipart, vector is message parts in order.
	std::unordered_map<std::string, std::vector<DirectedMessage>> pending_multipart_messages;
	
	// log file for recording packet data for analysis
	std::FILE* log_file;
	
	
	Connection() {
		this->log_file = NULL;
		this->next_channel = 1;
		this->remote_host_and_port = CONNECTION_ANY;
	}
	
	~Connection() {
		this->closeSocket();
		if(this->log_file) {
			std::fclose(this->log_file);
		}
	}
	
	static int hostAndPortToIP(IPaddress *ip, std::string host_and_port) {
		int colon_pos = host_and_port.find(":");
		if(colon_pos == std::string::npos)
			return -1;
		std::string host = host_and_port.substr(0,colon_pos);
		try {
			unsigned short port = std::stoi(host_and_port.substr(colon_pos+1));
			int result = SDLNet_ResolveHost(ip, host.c_str(), port);
			return result;
		} catch (std::exception& e) {
			return -1;
		}
	}
	
	static std::string IPToHostAndPort(IPaddress* ip) {
		std::stringstream ss;
		ss << (ip->host & 0xff);
		ss << "." << (ip->host >> 8 & 0xff);
		ss << "." << (ip->host >> 16 & 0xff);
		ss << "." << (ip->host >> 24 & 0xff);
		ss << ":" << ((ip->port >> 8) | ((ip->port << 8) & 0xff00));
		return ss.str();
	}
	
	static std::string getPacketUIDString(std::string hostAndPort, Packet* p) {
		return hostAndPort.append(":").append(std::to_string(p->id));
	}
	
	static std::string getPacketUIDString(std::string hostAndPort, unsigned int id) {
		return hostAndPort.append(":").append(std::to_string(id));
	}
	
	static void freeUDPPacket(UDPpacket* udp_packet) {
		SDLNet_FreePacket(udp_packet);
	}
	
	static bool multipartIsComplete(std::vector<DirectedMessage>* mp) {
		for(int i=0; i<mp->size(); i++) {
			if((*mp)[i].host_and_port.size() == 0)
				return false;
		}
		return true;
	}
	
	// Must be called before sending/receiving packets
	bool openSocket(unsigned short port, std::string remote=CONNECTION_ANY) {
		this->port = port;
		this->socket = SDLNet_UDP_Open(port);
		this->remote_host_and_port = remote;
		return this->socket != NULL;
	}
	
	void closeSocket() {
		SDLNet_UDP_Close(this->socket);
	}
	
	int getChannel(std::string host_and_port) {
		if(host_and_port == "BROADCAST")
			return -1;
		
		// try to find an existing channel for this host and port
		auto it = this->channels.find(host_and_port);
		if(it != this->channels.end())
			return it->second;
		
		// otherwise create it
		IPaddress ip;
		int result = Connection::hostAndPortToIP(&ip, host_and_port);
		if(result == -1)
			return -1;
		result = SDLNet_UDP_Bind(this->socket, this->next_channel, &(ip));
		if(result == -1)
			return -1;
		this->next_channel++;
		this->channels.insert({host_and_port, result});
		return result;
	}
	
	void unbind(std::string host_and_port) {
		if(this->socket == NULL)
			return;
		
		auto it = this->channels.find(host_and_port);
		if(it == this->channels.end())
			return; // not bound
		SDLNet_UDP_Unbind(this->socket, it->second);
		this->channels.erase(host_and_port);
	}
	
	void unbindAll() {
		if(this->socket == NULL)
			return;
		
		auto it = this->channels.begin();
		while(it != this->channels.end()) {
			SDLNet_UDP_Unbind(this->socket, it->second);
			it++;
		}
		this->channels.clear();
	}
	
	// internal send
	bool sendPacket(int channel, unsigned char multipart_total, unsigned char multipart_index, std::string message_part) {
		Packet p;
		p.assignID();
		
		char multipart_header[3];
		multipart_header[0] = 'M';
		multipart_header[1] = multipart_total;
		multipart_header[2] = multipart_index;
		p.addSegment(multipart_header, 3);
		
		p.addSegment(message_part.c_str(), message_part.size());
		
		UDPpacket* up = p.serialize();
		up->channel = channel;
		
		if(this->log_file) {
			std::fputc('>', this->log_file);
			std::fwrite(up->data, 1, up->len, this->log_file);
			std::fputc('\n', this->log_file);
		}
		
		// send four copies to lower chances of non-delivery
		bool result = true;
		if(SDLNet_UDP_Send(this->socket, channel, up) == 0)
			result = false;
		if(SDLNet_UDP_Send(this->socket, channel, up) == 0)
			result = false;
		/*if(SDLNet_UDP_Send(this->socket, channel, up) == -1)
			result = false;
		if(SDLNet_UDP_Send(this->socket, channel, up) == -1)
			result = false;*/
		
		// std::cout << "< Sent packet. Succeeded: " << result << std::endl; TODO: fix bullet destroy
			
		Connection::freeUDPPacket(up);
		return result;
	}
	
	// returns whether the message was sent
	bool send(std::string host_and_port, std::string message) {
		if(this->socket == NULL)
			return false;
		
		int channel = this->getChannel(host_and_port);
		
		//std::cout << "# Send debug: " << host_and_port << "->" << channel << " " << message << std::endl;
		
		std::vector<std::string> multiparts;
		
		int size = message.size();
		for(int i=0; i<size; i+=PACKET_SEGMENT_MAX_SIZE) {
			int part_length;
			if(size-i < PACKET_SEGMENT_MAX_SIZE) {
				part_length = size-i;
			} else {
				part_length = PACKET_SEGMENT_MAX_SIZE;
			}
			std::string message_part = message.substr(i,part_length);
			multiparts.push_back(message_part);
		}
		
		bool success = true;
		for(int i=0; i<multiparts.size(); i++) {
			success = success && sendPacket(channel, multiparts.size(), i, multiparts[i]);
		}
		
		return success;
	}
	
	bool sendAll(std::string message) {
		bool success = true;
		for(auto p : this->channels) {
			success = success && this->send(p.first, message);
		}
		return success;
	}
	
	// returns if anything was recieved
	bool receive(std::string* host_and_port, std::string* message) {
		if(this->socket == NULL)
			return false;
		
		if(this->received_messages.size() > 0) {
			DirectedMessage m = this->received_messages.front();
			this->received_messages.pop_back();
			*host_and_port = m.host_and_port;
			*message = m.message;
			return true;
		}
				
		// Clean up old received_uids
		auto now = misc::nanoNow();
		for(auto i=this->received_uids.begin(); i!=this->received_uids.end(); ) {
			if(now > i->second) {
				i = this->received_uids.erase(i);
			} else {
				++i;
			}
		}
		
		Packet p;
		UDPpacket* up = SDLNet_AllocPacket(PACKET_MAX_SIZE);
		
		// this loop goes through physical packets, discarding duplicates, and eventually assembling a multipart
		while(true) {
			int result = SDLNet_UDP_Recv(this->socket, up);
			if(result == 0) {
				// No packet received
				SDLNet_FreePacket(up);
				return false;
			}
			if(result == -1) {
				// Error
				std::cout << "< Receive networking error." << std::endl;
				SDLNet_FreePacket(up);
				continue;
			}
			
			// discard packets from non-authorized sources
			std::string host = IPToHostAndPort(&up->address);
			if(this->remote_host_and_port != CONNECTION_ANY &&
				this->remote_host_and_port != host) {
				//std::cout << "< Received packet from source other than remote." << std::endl;
				SDLNet_FreePacket(up);
				continue;
			}
			
			if(this->log_file) {
				std::fputc('<', this->log_file);
				std::fwrite(up->data, 1, up->len, this->log_file);
				std::fputc('\n', this->log_file);
			}			
			
			p.deserialize(up);
			
			// Check if this packet was already received.
			std::string packet_uid = Connection::getPacketUIDString(host, &p);
			auto it = this->received_uids.find(packet_uid);
			if(it != this->received_uids.end()) {
				// This is a duplicate packet. Throw it away.
				continue;
			} else {
				this->received_uids.insert({packet_uid, misc::nanoNow() + NETWORKING_MAX_DUPLICATE_WAIT});
			}
			
			// register this host as a channel if it isn't already.
			this->getChannel(host);
			
			// currently all packets have two segments. this is a sanity check.
			if(p.segments.size() != 2 && p.segments[0].length == 3)
				continue; // drop insane packets
			
			//p.id;
			unsigned char mp_len = ((char*)(p.segments[0].data))[1];
			unsigned char mp_idx = ((char*)(p.segments[0].data))[2];
			if(mp_idx-1 > mp_len) // check sanity
				continue; // drop insane
			unsigned char mp_first_id = p.id - mp_idx;
			
			std::string first_packet_uid = getPacketUIDString(host, mp_first_id);
			auto it2 = this->pending_multipart_messages.find(first_packet_uid);
			if(it2 == this->pending_multipart_messages.end()) {
				std::vector<DirectedMessage> new_multipart;
				new_multipart.resize(mp_len);
				this->pending_multipart_messages.insert({first_packet_uid, new_multipart});
				it2 = this->pending_multipart_messages.find(first_packet_uid);
			}
			std::vector<DirectedMessage>* multiparts = &it2->second;
			
			if(mp_len != multiparts->size()) // check sanity
				continue; // drop insane
			
			// collect the part into the appropriate multipart vector
			DirectedMessage mpp;
			mpp.host_and_port = host;
			mpp.message.resize(p.segments[1].length);
			mpp.message.assign((char*)p.segments[1].data, p.segments[1].length);
			(*multiparts)[mp_idx] = mpp;
			
			// if the multipart is complete, return it
			if(Connection::multipartIsComplete(multiparts)) {
				*host_and_port = host;
				*message = "";
				for(int i=0; i<multiparts->size(); i++) {
					message->append((*multiparts)[i].message);
				}
				SDLNet_FreePacket(up);
				this->pending_multipart_messages.erase(it2);
				return true;
			}
		}
		
		SDLNet_FreePacket(up);
		
		return false;
	}

	void enableLogging() {
		this->log_file = std::fopen("networking.log", "wb");
	}
};

void initializeNetworking();

// Returns 0 on success. Otherwise returns the number of the test that failed.
int networking_unitTest();