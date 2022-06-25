#pragma once

#include <vector>
#include <string>
#include <exception>
#include <deque>
#include <unordered_map>
#include <iostream>
#include <SDL2/SDL_net.h>

#include "serialization.h"
#include "misc.h"

namespace razor {
	inline constexpr nanotime MAX_DUPLICATE_WAIT = 10 * NANOS_PER_SECOND;
	inline constexpr auto PACKET_MAX_SIZE = 508;
	inline constexpr auto PACKET_SEGMENT_MAX_SIZE = PACKET_MAX_SIZE-12;
	inline constexpr auto ANY_ADDRESS = "ANY";
	
	// Razor's packet
	// Structure:
	// - ID 4 bytes
	// - NUM_SEGMENTS or MULTIPART 1 byte
	// <for each segment>
	// - SEGMENT_SIZE 1 byte
	// - SEGMENT_DATA up to PACKET_SEGMENT_MAX_SIZE bytes
	class Packet {
	public:
		struct Segment {
			unsigned short length;
			void* data;
		};
		
		unsigned int id;
		std::vector<Segment> segments;
		
		Packet();
		~Packet();
		void freeSegments();
		void assignID();
		unsigned short length();
		unsigned char num_segments();
		void addSegment(const void* data, unsigned short length);
		UDPpacket* serialize();
		void deserialize(UDPpacket* udp_packet);
	};


	// Handles SDL_net channels & resolving IPs
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
		
		Connection();
		~Connection();
		
		static int hostAndPortToIP(IPaddress *ip, const std::string &host_and_port);
		static std::string IPToHostAndPort(IPaddress* ip);
		
		// Must be called before sending/receiving packets
		bool openSocket(unsigned short port, const std::string &remote=ANY_ADDRESS);
		
		void closeSocket();
		
		int getChannel(const std::string &host_and_port);
		void unbind(const std::string &host_and_port);
		void unbindAll();
		
		// returns whether the message was sent
		bool send(const std::string &host_and_port, const std::string &message);
		bool sendAll(const std::string &message);
		
		// returns if anything was recieved
		bool receive(std::string* host_and_port, std::string* message);
		
		void enableLogging();
		
	private:
		bool sendPacket(int channel, unsigned char multipart_total, 
				unsigned char multipart_index, const std::string &message_part);
		static std::string getPacketUIDString(const std::string &hostAndPort, Packet* p);
		static std::string getPacketUIDString(const std::string &hostAndPort, unsigned int id);
		static void freeUDPPacket(UDPpacket* udp_packet);
		static bool multipartIsComplete(std::vector<DirectedMessage>* mp);
	};
	
	void initializeNetworking();
	
	// Returns 0 on success. Otherwise returns the number of the test that failed.
	int networkingUnitTest();
}