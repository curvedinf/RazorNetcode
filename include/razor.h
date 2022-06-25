#include <vector>
#include <deque>
#include <string>
#include <iostream>

#include "networking.h"

//extern std::string local_player_name;

/*
==Protocol==

Daemon - the dedicated server computer
Slave - the client computer

Slave|Daemon - Packet type
-> REQUEST_FULL *
<- PONG *
<- SYNC 
-> COMMAND
<- COMMAND (to other slaves)
<- SYNC
-> DISCONNECT


==Sync Methodology==

Daemon has an authoritative, ordered command list for each tick.

Slaves operate a number of ticks ahead of the daemon determined by the PONG length plus a set amount into the future.

Slaves send commands to daemon that are queued into the future.

Daemon rebroadcasts future commands to all slaves.

When a command is received by a slave, it is queued into its command list and ticks are
replayed from the oldest command forward to regenerate the future simulation.

If the daemon receives a command that is older than the current tick, it is disgarded

Occassionally the daemon broadcasts a sync of all changes past the last sync

If a slave detects packet loss, it will request a full sync

If the daemon or a slave sends no packets for KEEPALIVE time, it will send a pong to keep the connection alive

If the daemon or a slave detects no packets for TIMEOUT time, it will disconnect
*/

namespace razor {
	typedef unsigned long long int ticktype;
	typedef long long int nanotimediff;
	
	// Amount of time after connecting to wait before adding the player. This allows local time to be synchronized.
	inline constexpr nanotime CREATE_PLAYER_DELAY = 500 * NANOS_PER_MILLI;

	// Amount of time after adding the player to set the team. This allows the player to exist before its team is set.
	inline constexpr nanotime SET_TEAM_DELAY = 5 * NANOS_PER_MILLI;

	// The number of past pings to find the maximum from
	inline constexpr nanotime PING_LOG_LENGTH = 10;

	// The multiplier on top of ping. This gives a buffer of time for commands to be received at the server
	inline constexpr auto FUTURE_TIME_PING_MULTIPLIER = 1.4f;

	// Fixed amount of future time to add on top of multiplier. This is to account for other player's pings when
	// the server sends your command to them.
	inline constexpr nanotime FUTURE_TIME_PING_FIXED = 30 * NANOS_PER_MILLI;

	// Maximum value of future time before triggering high-ping self-disconnect
	inline constexpr nanotime MAX_FUTURE_TIME_HIGH_PING = 1000 * NANOS_PER_MILLI;

	// Size of the working memory for sends
	inline constexpr auto SEND_BUFFER_SIZE = 1024*1024;

	// Maximum commands per packet
	inline constexpr auto MAX_COMMANDS_PER_PACKET = 5;

	// Maximum length of a command
	inline constexpr auto MAX_COMMAND_LENGTH = 200;

	// Maximum ticks in the future for a command
	inline constexpr auto COMMAND_MAX_FUTURE = 2000;

	// number of nanoseconds to wait before sending another ping request
	inline constexpr nanotime PING_DELAY = 1000 * NANOS_PER_MILLI;

	// number of pings to average
	inline constexpr auto PING_LOG_SIZE = 10;

	// number of game ticks to wait before requesting a new sync
	inline constexpr auto SYNC_DELAY = 250;

	// number of game ticks to accumulate commands before sending
	inline constexpr auto COMMAND_DELAY = 10;

	// Destination/Source special case host strings
	inline constexpr auto LOCAL = "LOCAL";
	inline constexpr auto BROADCAST = "BROADCAST";

	// == Message Types ==

	// Command packets contain a list of broadcast console commands
	enum MessageTypes {
		MESSAGE_COMMAND,
		MESSAGE_SYNC,
		MESSAGE_PONG,
		MESSAGE_REQUEST_FULL,
		MESSAGE_DISCONNECT,
		MESSAGE_PING
	};
	
	class Razor {
	public:
		Connection connection;
		
		struct NetworkMessage {
			unsigned char type;
			std::string origin_host_and_port;
			std::string dest_host_and_port; // if == to BROADCAST, will be sent to all
			nanotime timestamp; // timestamps are absolute to the epoch
			ticktype ticknumber; // ticknumbers are relative / dynamic
			std::string message;
		};
		
		struct OutgoingCommand {
			ticktype tick_number;
			std::string command;
		};
		
		// queue for messages waiting to be sent
		std::deque<NetworkMessage> send_queue;
		
		// for slaves only, the last PING_LOG_LENGTH of pings
		std::deque<nanotime> ping_log;
		
		// for slaves only, the last PING_LOG_LENGTH of time_deltas
		std::deque<nanotimediff> time_delta_log;
		
		bool daemon, slaved;
		std::string daemon_host_and_port;
		
		std::deque<OutgoingCommand> outgoing_commands;
		
		// ping in nanoseconds. Generated from a moving average of ping_log.
		nanotime ping;
		
		// zero time of the daemon
		nanotime daemon_zero_time;
		
		// difference between local zero_time and daemon zero_time
		nanotimediff time_delta_to_daemon;
		
		// number of nanoseconds into the future the player will play at
		nanotimediff future_time;
		
		bool first_ping, first_sync, create_player, set_team;
		ticktype create_player_delay;
		ticktype set_team_delay;
		
		// used for working memory for building packets
		char* send_buffer;
		char* packed_command_buffer;
		
		// Sync timer
		ticktype next_sync_tick, last_sync_tick;
		
		// Ping timer
		nanotime next_ping_time;
		
		// Command send delay timer
		nanotime next_command_time;
		
		// The local state's current tick (as of last tick call)
		ticktype local_tick_number;
		
		// The local state's current zero_time (as of last tick call)
		nanotime local_zero_time;
		
		bool destroyed;
		
		Razor();
		
		~Razor();
		
		void destroy();
		
		nanotimediff calculateLocalTimeDifference();
		
		// Serialize/deserialize different message types
		// returns number of bytes copied
		int serializeMessage(void* data, NetworkMessage* in);
		int deserializeMessage(NetworkMessage* out, void* data);
		
		int serializePong(char* data, nanotime remote_timestamp, nanotime zero_time);
		int deserializePong(char* data, nanotime *start_timestamp, nanotime *zero_time);
		
		int serializeCommand(char* data, int pos, ticktype tick_number, std::string* command);
		int deserializeCommand(char* data, int pos, ticktype *tick_number, std::string *command);
		
		// Queuing of new messages, used by sends
		void queueOutgoingNetworkMessage(const std::string &dest, unsigned char type, const std::string& message);
		void queueOutgoingCommands();
		
		// Send message types
		void sendRequestFullSync();
		void sendPong(std::string dest, nanotime remote_timestamp);
		void sendPing(std::string dest);
		void sendDisconnect(std::string dest);
		void sendSync(std::string dest);
		void sendCommand(const std::string& command);
		
		// Receive message types
		void receivePong(NetworkMessage* nm);
		void receiveCommands(NetworkMessage* nm);
		void receiveSync(NetworkMessage* nm);
		
		// Handle sending and receiving of messages
		void sendMessages(ticktype tick_number);
		void receiveMessages();
		
		// Internal processes
		void connectIfNeeded();
		void clearSendQueue();
		void clearOutgoingCommands();
		void updateFutureTime();
		
		// Daemon/slave tick functions
		void daemonTick();
		void slaveTick(ticktype tick_number);
		
		// Public configuration
		void setPort(int port);
		void setDaemon(bool is_daemon=true);
		void setDaemonAddress(const std::string &daemon_host_and_port);
		void setLogNetworking();
		
		// Public live functions
		// Note: tick must be called first each frame
		void tick(ticktype tick_number, nanotime zero_time);
		void command(const std::string &command_data);
	};
};