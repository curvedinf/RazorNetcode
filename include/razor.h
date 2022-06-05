#pragma once

#include "networking.h"
#include "serialization.h"
#include "rand.h"
#include "misc.h"

#include <vector>
#include <deque>

//extern std::string local_player_name;

/*
==Protocol==

Daemon - the dedicated server computer
Slave - the client computer

Slave - Daemon - Packet type
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

// Amount of time after connecting to wait before adding the player. This allows local time to be synchronized.
#define RAZOR_SYNC_CREATE_PLAYER_DELAY 500 * NANOS_PER_MILLI

// Amount of time after adding the player to set the team. This allows the player to exist before its team is set.
#define RAZOR_SYNC_SET_TEAM_DELAY 5 * NANOS_PER_MILLI

// Network timeout in game ticks
#define RAZOR_SYNC_TIMEOUT 20000

// Maximum time before sending a keepalive to the daemon or clients
#define RAZOR_SYNC_KEEPALIVE 5000

// The number of past pings to find the maximum from
#define RAZOR_SYNC_PING_LOG_LENGTH 10

// The multiplier on top of ping. This gives a buffer of time for commands to be received at the server
#define RAZOR_SYNC_FUTURE_TIME_PING_MULTIPLIER 1.4

// Fixed amount of future time to add on top of multiplier. This is to account for other player's pings when
// the server sends your command to them.
#define RAZOR_SYNC_FUTURE_TIME_PING_FIXED_FUTURE 30 * NANOS_PER_MILLI

// Maximum value of future time before triggering high-ping self-disconnect
#define RAZOR_SYNC_MAX_FUTURE_TIME_HIGH_PING 1000 * NANOS_PER_MILLI

// Maximum time of high ping before disconnect in ticks
#define RAZOR_SYNC_MAX_HIGH_PING_LENGTH 10000

// Change per frame for future_time to reach target_future_time
#define RAZOR_SYNC_FUTURE_TIME_TRACK_TARGET 0.1

// Size of the working memory for sends
#define RAZOR_SYNC_SEND_BUFFER_SIZE 1024*1024

// Maximum commands per packet
#define RAZOR_SYNC_MAX_COMMANDS_PER_PACKET 5

// Maximum length of a command
#define RAZOR_SYNC_MAX_COMMAND_LENGTH 200

// Maximum ticks in the future for a command
#define RAZOR_SYNC_COMMAND_MAX_FUTURE 2000

// number of nanoseconds to wait before sending another ping request
#define RAZOR_SYNC_PING_DELAY 1000 * NANOS_PER_MILLI

// number of pings to average
#define RAZOR_SYNC_PING_LOG_SIZE 10

// number of game ticks to wait before requesting a new sync
#define RAZOR_SYNC_SYNC_DELAY 250

// number of game ticks to accumulate commands before sending
#define RAZOR_SYNC_COMMAND_DELAY 10

// Destination/Source special case host strings
#define RAZOR_SYNC_LOCAL "LOCAL"
#define RAZOR_SYNC_BROADCAST "BROADCAST"

// == Packet Types ==

// Command packets contain a list of broadcast console commands
#define RAZOR_SYNC_TYPE_COMMAND 1

// Sync packets contain the updated serialized state
#define RAZOR_SYNC_TYPE_SYNC 2

// Pong packets are sent back from the daemon occassionally with the time difference to the slaves
#define RAZOR_SYNC_TYPE_PONG 3

// Request full sync packets request a new full sync
#define RAZOR_SYNC_TYPE_REQUEST_FULL 4

// Disconnect packets are a courtesy to request stop broadcasting
#define RAZOR_SYNC_TYPE_DISCONNECT 5

// Ping packets are sent by slaves to request new pongs
#define RAZOR_SYNC_TYPE_PING 6


class Razor {
public:
	Connection connection;
	
	struct NetworkMessage {
		unsigned char type;
		std::string origin_host_and_port;
		std::string dest_host_and_port; // if == to RAZOR_SYNC_BROADCAST, will be sent to all
		unsigned long long timestamp; // timestamps are absolute to the epoch
		unsigned long long ticknumber; // ticknumbers are relative / dynamic
		std::string message;
	};
	
	struct OutgoingCommand {
		unsigned long long tick_number;
		std::string command;
	};
	
	// queue for messages waiting to be sent
	std::deque<NetworkMessage> send_queue;
	
	// for slaves only, the last RAZOR_SYNC_PING_LOG_LENGTH of pings
	std::deque<long long> ping_log;
	
	// for slaves only, the last RAZOR_SYNC_PING_LOG_LENGTH of time_deltas
	std::deque<long long> time_delta_log;
	
	bool daemon, slaved;
	std::string daemon_host_and_port;
	
	std::deque<OutgoingCommand> outgoing_commands;
	
	// ping in nanoseconds. Generated from a moving average of ping_log.
	long long ping;
	
	// zero time of the daemon
	unsigned long long daemon_zero_time;
	
	// difference between local zero_time and daemon zero_time
	long long time_delta_to_daemon;
	
	// number of nanoseconds into the future the player will play at
	long long future_time;
	
	// the target future time that will be incremented toward TODO
	long long target_future_time;
	
	bool first_ping, first_sync, create_player, set_team;
	unsigned long long create_player_delay;
	unsigned long long set_team_delay;
	
	// used for working memory for building packets
	char* send_buffer;
	char* packed_command_buffer;
	
	// Sync timer
	unsigned long long next_sync_tick, last_sync_tick;
	
	// Ping timer
	unsigned long long next_ping_time;
	
	// Command send delay timer
	unsigned long long next_command_time;
	
	
	Razor() {
		this->daemon = false;
		this->slaved = false;
		this->first_ping = true;
		this->first_sync = true;
		this->create_player = false;
		this->create_player_delay = 0;
		this->set_team = false;
		this->set_team_delay = 0;
		this->daemon_host_and_port = "";
		this->send_buffer = new char[RAZOR_SYNC_SEND_BUFFER_SIZE];
		this->packed_command_buffer = new char[RAZOR_SYNC_MAX_COMMANDS_PER_PACKET * 
													(RAZOR_SYNC_MAX_COMMAND_LENGTH + 8)
													+ 2]; // extra 2 for number of commands
		this->future_time = 0;
		this->target_future_time = 0;
		this->next_sync_tick = 0;
		this->last_sync_tick = 0;
		this->next_ping_time = 0;
		this->next_command_time = 0;
		this->ping = 0;
		this->time_delta_to_daemon = 0;
	};
	
	~Razor() {
	}
	
	void destroy() {
		this->connection.closeSocket(); // force close
		std::cout << "< Closed networking socket." << std::endl;
		delete [] this->send_buffer;
		delete [] this->packed_command_buffer;
	}
	
	unsigned long long calculateLocalTimeDifference() {
		if(this->time_delta_log.size() == 0)
			return 0;
		
		// average the time_delta_log
		double avg_td = 0;
		for(int i=0; i<this->time_delta_log.size(); i++) {
			avg_td += this->time_delta_log[i];
		}
		avg_td /= this->time_delta_log.size();
		
		
		// get the max of the ping_log
		long long max_ping = 0;
		for(int i=0; i<this->ping_log.size(); i++) {
			unsigned long long ping = this->ping_log[i];
			if(ping > max_ping) {
				max_ping = ping;
			}
		}
		
		// add a buffer of future time to account for ping spikes
		unsigned long long future_time = (max_ping/2.0f) // 1-way travel time
			* RAZOR_SYNC_FUTURE_TIME_PING_MULTIPLIER // ping spike headroom
			+ RAZOR_SYNC_FUTURE_TIME_PING_FIXED_FUTURE; // fixed ping spike headroom
		
		//std::cout << "avg_td " << avg_td / NANOS_PER_MILLI;
		//std::cout << " max_ping " << max_ping / NANOS_PER_MILLI;
		//std::cout << " ping_headroom " << ping_headroom / NANOS_PER_MILLI << std::endl;
		
		this->ping = max_ping / NANOS_PER_MILLI;
		this->time_delta_to_daemon = avg_td / NANOS_PER_MILLI;
		this->future_time = future_time / NANOS_PER_MILLI;
		
		return avg_td - future_time;
	}
	
	// returns number of bytes copied
	int serializeMessage(void* data, NetworkMessage* in) {
		int pos = 0;
		pos += copyInC(data, pos, in->type);
		pos += copyInLL(data, pos, in->timestamp);
		pos += copyInLL(data, pos, in->ticknumber);
		pos += copyInString(data, pos, &in->message);
		return pos;
	}
	
	// returns number of bytes copied
	int deserializeMessage(NetworkMessage* out, void* data) {
		int pos = 0;
		pos += copyOutC((char*)&out->type, data, pos);
		pos += copyOutLL((long long*)&out->timestamp, data, pos);
		pos += copyOutLL((long long*)&out->ticknumber, data, pos);
		pos += copyOutString(&out->message, data, pos);
		return pos;
	}
	
	int serializePong(char* data, unsigned long long remote_timestamp, unsigned long long zero_time) {
		int pos = 0;
		pos += copyInLL(data, pos, remote_timestamp);
		pos += copyInLL(data, pos, zero_time);
		return pos;
	}
	
	int deserializePong(char* data, unsigned long long *start_timestamp, unsigned long long *zero_time) {
		int pos = 0;
		pos += copyOutLL((long long*)start_timestamp, data, pos);
		pos += copyOutLL((long long*)zero_time, data, pos);
		return pos;
	}
	
	int serializeCommand(char* data, int pos, unsigned long long tick_number, std::string* command) {
		if(command->length() > RAZOR_SYNC_MAX_COMMAND_LENGTH) {
			std::cout << "< WARNING: Command serialization over length: " << command->length() 
					<< " > " << RAZOR_SYNC_MAX_COMMAND_LENGTH << " [" << *command << "]" << std::endl;
		}
		int len = 0;
		len += copyInLL(data, pos+len, tick_number);
		len += copyInString(data, pos+len, command);
		return len;
	}
	
	int deserializeCommand(char* data, int pos, unsigned long long *tick_number, std::string *command) {
		int len = 0;
		len += copyOutLL((long long*)tick_number, data, pos+len);
		if(len > RAZOR_SYNC_MAX_COMMAND_LENGTH) {
			std::cout << "< WARNING: Command deserialization over length: " << len 
					<< " > " << RAZOR_SYNC_MAX_COMMAND_LENGTH << std::endl;
		}
		len += copyOutString(command, data, pos+len);
		return len;
	}
	
	void queueOutgoingNetworkMessage(std::string dest, unsigned char type, std::string& message) {
		NetworkMessage nm;
		nm.origin_host_and_port = RAZOR_SYNC_LOCAL;
		nm.dest_host_and_port = dest;
		// TODO: setup current frame number
		nm.ticknumber = 0;//this->server->frame_number;
		nm.timestamp = misc::nanoNow();
		nm.type = type;
		nm.message = message;
		this->send_queue.push_back(nm);
	}
	
	void queueOutgoingCommands() {
		std::string dest;
		if(this->daemon) {
			dest = RAZOR_SYNC_BROADCAST;
		} else {
			if(!this->slaved) { // if not a daemon and not slaved, clear the queue and do nothing
				this->clearOutgoingCommands();
				return;
			}
			dest = this->daemon_host_and_port;
		}
		
		int command_counter = 0;
		int pos = 2; // 2 because the number of 
		while(this->outgoing_commands.size() > 0) {
			auto out_command = this->outgoing_commands.front();
			this->outgoing_commands.pop_front();
			
			//std::cout << "< Sending command: " << out_command.command << std::endl;
			
			// discard long commands
			if(out_command.command.size() > RAZOR_SYNC_MAX_COMMAND_LENGTH) {
				std::cout << "< Command exceeds maximum length for network sync: " << out_command.command << std::endl;
				continue;
			}
			pos += this->serializeCommand(packed_command_buffer, pos, out_command.tick_number, &out_command.command);
			if(command_counter == RAZOR_SYNC_MAX_COMMANDS_PER_PACKET) {
				// if the max commands is reached, send a packed packet
				std::string temp;
				copyInS(this->packed_command_buffer, 0, command_counter); // number of commands at beginning
				temp.resize(pos);
				temp.assign(this->packed_command_buffer, pos);
				queueOutgoingNetworkMessage(dest, RAZOR_SYNC_TYPE_COMMAND, temp);
				pos = 2;
				command_counter = 0;
			} else {
				command_counter++;
			}
		}
		
		if(command_counter > 0) {
			// send any remaining commands in another packed packet
			std::string temp;
			copyInS(this->packed_command_buffer, 0, command_counter); // number of commands at beginning
			temp.resize(pos);
			temp.assign(this->packed_command_buffer, pos);
			queueOutgoingNetworkMessage(dest, RAZOR_SYNC_TYPE_COMMAND, temp);
		}
	}
	
	
	// Message send functions
	void sendRequestFullSync() {
		std::string empty;
		this->queueOutgoingNetworkMessage(
				this->daemon_host_and_port, RAZOR_SYNC_TYPE_REQUEST_FULL, empty);
	}
	
	// Pongs also return the requester's timestamp
	void sendPong(std::string dest, unsigned long long remote_timestamp, unsigned long long zero_time) {
		char pong_data[16]; // two 8 byte long longs
		serializePong(pong_data, remote_timestamp, zero_time);
		std::string pong_str;
		pong_str.resize(16);
		pong_str.assign(pong_data, 16);
		this->queueOutgoingNetworkMessage(dest, RAZOR_SYNC_TYPE_PONG, pong_str);
	}
	
	// Ping the server
	void sendPing(std::string dest) {
		std::string ping_str = " ";
		this->queueOutgoingNetworkMessage(dest, RAZOR_SYNC_TYPE_PING, ping_str);
	}
	
	void sendDisconnect(std::string dest) {
		std::string empty;
		this->queueOutgoingNetworkMessage(
			dest, RAZOR_SYNC_TYPE_DISCONNECT, empty);
	}
	
	void sendSync(std::string dest, unsigned long long frame_number) {
		// TODO: Get sync state
		//this->server->serializeState(&state);
		 std::string* state = NULL;
		
		int pos = 0;
		pos += copyInLL(send_buffer, pos, frame_number);
		pos += copyInString(send_buffer, pos, state);
		
		std::string message;
		message.resize(pos);
		message.assign(send_buffer, pos);
		
		this->queueOutgoingNetworkMessage(dest, RAZOR_SYNC_TYPE_SYNC, message);
		
		std::cout << "< Sending full sync to " << dest << std::endl;
	}
	
	void sendCommand(std::string& command, unsigned long long frame_number) {
		OutgoingCommand o;
		o.tick_number = frame_number;
		o.command = command;
		outgoing_commands.push_back(o);
	}
	
	// Common processes
	void connectIfNeeded() {
		if(!this->daemon && !this->slaved) {
			// do not try to become slaved if there is no specified daemon
			if(this->daemon_host_and_port == "")
				return;
			
			this->sendRequestFullSync();
			this->slaved = true;
		}
	}
	
	void receivePong(NetworkMessage* nm, unsigned long long zero_time) {
		if(this->daemon) // deamons ignore pongs -- they have no need to synchronize
					return;
		
		unsigned long long start_timestamp;
		deserializePong((char*)nm->message.c_str(), &start_timestamp, &(this->daemon_zero_time));
		unsigned long long bounce_timestamp = nm->timestamp;
		unsigned long long end_timestamp = misc::nanoNow();
		
		// clean ping log
		while(this->ping_log.size() >= RAZOR_SYNC_PING_LOG_SIZE)
			this->ping_log.pop_front();
		
		// clean time delta log
		while(this->time_delta_log.size() >= RAZOR_SYNC_PING_LOG_SIZE)
			this->time_delta_log.pop_front();
		
		// the ping is the round trip time from slave->daemon->slave
		unsigned long long ping = end_timestamp - start_timestamp;
		this->ping_log.push_back(ping);
		
		// time_delta is estimated difference between the two time systems' 
		// zero_time (from local to daemon).
		// time delta is estimated by using the local half-ping timestamp
		// and the server's "midway" timestamp adjusted
		// they should correspond roughly to the same absolute time, so
		// the difference will be a rough time delta.
		// over time, the log will average out a reasonable time delta
		long long time_delta = (start_timestamp + ping/2 - zero_time) -
									(bounce_timestamp - this->daemon_zero_time);
		this->time_delta_log.push_back(time_delta);
		
		/*std::cout << "ping " << ping / NANOS_PER_MILLI << std::endl;
		std::cout << "bounce_timestamp " << bounce_timestamp / NANOS_PER_MILLI << std::endl;
		std::cout << "daemon_zero_time " << this->daemon_zero_time / NANOS_PER_MILLI << std::endl;
		std::cout << "time_delta " << time_delta / NANOS_PER_MILLI << std::endl;/**/
		
		// if this is the first ping, use the ping's data to initialize the future ticks
		if(this->first_ping) {
			this->future_time = this->calculateLocalTimeDifference();
			this->target_future_time = this->future_time;
			this->first_ping = false;
		}
	}
	
	void receiveCommands(NetworkMessage* nm, unsigned long long frame_number) {
		unsigned short commands_number;
		const char* buffer = nm->message.c_str();
		int pos = 0;
		pos += copyOutS((short*)&commands_number, (void*)buffer, pos);
		if(commands_number > RAZOR_SYNC_MAX_COMMANDS_PER_PACKET) {
			std::cout << "< Received command packet with too many commands (" << commands_number << ")" << std::endl;
			return;
		}
		for(int i=0; i<commands_number; i++) {
			unsigned long long tick_number;
			std::string command;
			pos += deserializeCommand((char*)buffer, pos, &tick_number, &command);
			if(command.size() > RAZOR_SYNC_MAX_COMMAND_LENGTH) {
				std::cout << "< Received command over size limit (" << command.size() << ")" << std::endl;
				return;
			}
			if(this->daemon && tick_number < frame_number) {
				std::cout << "< Received command in the past, discarding (received " << tick_number
							<< " vs now " << frame_number << ")" << std::endl;
				return;
			}
			if(this->daemon && tick_number - frame_number > RAZOR_SYNC_COMMAND_MAX_FUTURE) {
				std::cout << "< Received command too far in the future (" << tick_number << ") for now "
							<< "(" << frame_number << ")" << std::endl;
				return;
			}
			
			// if validated log the command
			auto tick_and_command = std::to_string(tick_number).append(" ").append(command);
			
			if(this->daemon) {
				//std::cout << "< Received command @ " << this->server->frame_number << " : " << tick_and_command << std::endl;
				sendCommand(command, tick_number);
			} else {
				// Note: it is important that this comes before the logcommand below because it must check
				//		to see if this is an existing command.
				// TODO: add command play back
				//this->server->consoleInternal(std::string("replaycommand ").append(tick_and_command), false);
			}
			
			// Note: this must come after the replaycommand above because replay command checks if this command is logged.
			// TODO: add command logging
			//this->server->consoleInternal(std::string("logcommand ").append(tick_and_command), false);
		}
	}
	
	void receiveSync(NetworkMessage* nm) {
		if(this->daemon) { // daemons do not receive syncs
			return;
		} else if(!this->slaved) { // if not a daemon and not slaved, do nothing
			return;
		}
		
		// Set the server's tick time to the sync message's
		// load in the gamedata sync
		// Set the server's local_time_difference to the correct amount of future_ticks
		unsigned long long daemon_tick_number;
		std::string state;
		
		char* data = (char*)nm->message.c_str();
		
		int pos = 0;
		pos += copyOutLL((long long*)&daemon_tick_number, data, pos);
		pos += copyOutString(&state, data, pos);
		
		//std::cout << "< Sync daemon tick: " << daemon_tick_number << ". Local tick: " << this->server->frame_number << std::endl;
		
		unsigned long long local_time_difference = this->future_time;
		
		if(this->first_sync) {
			this->first_sync = false;
			this->create_player = true;
			this->create_player_delay = misc::nanoNow() + RAZOR_SYNC_CREATE_PLAYER_DELAY;
			//std::cout << "Create player delay " << this->create_player_delay << " " << RAZOR_SYNC_CREATE_PLAYER_DELAY << std::endl;
			// for the first sync, do a hard non-background resync. This is necessary to synchronize
			// frame numbers and future time.
			
			// TODO: initial resync
			//this->server->queueResync(&state, daemon_tick_number, local_time_difference);
		} else {
			// I am leaving this non-background resync here for benchmarking purposes
			//this->server->queueResync(&state, daemon_tick_number, local_time_difference);
			
			// Force a "priority" replay. This causes all other replays in queue to be deleted and causes the whole server
			// to be deserialized once updated.
			
			// inter-heurisitc talk is dangerous, but this is necessary because the state might be large.
			
			// TODO: remake state resynchronization
			/*auto ts = (TrackerStates*)this->server->getHeuristic(HEURISTIC_TRACKER_STATES);
			if(ts != NULL && ts->replay_runner != NULL) {
				// replay from the daemon tick's number to our current frame.
				std::cout << "< Queuing background netsync replay. Server0 frame: " << this->server->frame_number <<
					" Daemon frame: " << daemon_tick_number << std::endl;
				ts->replay_runner->startNewReplay(&state, daemon_tick_number, this->server->frame_number, 
					local_time_difference, this->server->zero_time, 
					true); // priority = true
			}/**/
		}
	}
	
	void receiveMessages(unsigned long long frame_number, unsigned long long zero_time) {
		std::string host_and_port, message;
		
		while(this->connection.receive(&host_and_port, &message)) {
			NetworkMessage nm;
			nm.origin_host_and_port = host_and_port;
			nm.dest_host_and_port = RAZOR_SYNC_LOCAL;
			
			try {
				this->deserializeMessage(&nm, (void*)message.c_str());
				
				if(nm.type == RAZOR_SYNC_TYPE_COMMAND) {
					//std::cout << "< Received command" << std::endl;
					this->receiveCommands(&nm, frame_number);
				} else if(nm.type == RAZOR_SYNC_TYPE_SYNC) {
					std::cout << "< Received sync" << std::endl;
					this->receiveSync(&nm);
				} else if(nm.type == RAZOR_SYNC_TYPE_PONG) {
					std::cout << "< Received pong" << std::endl;
					this->receivePong(&nm, zero_time);
				} else if(nm.type == RAZOR_SYNC_TYPE_REQUEST_FULL) {
					if(!this->daemon) // slaves should ignore sync requests
						continue;
					std::cout << "< Received request full" << std::endl;
					this->sendPong(nm.origin_host_and_port, nm.timestamp, zero_time);
					this->sendSync(nm.origin_host_and_port, frame_number);
				} else if(nm.type == RAZOR_SYNC_TYPE_PING) {
					if(!this->daemon) // slaves should ignore ping requests
						continue;
					this->sendPong(nm.origin_host_and_port, nm.timestamp, zero_time);
				} else if(nm.type == RAZOR_SYNC_TYPE_DISCONNECT) {
					// TODO
				} else {
					std::cout << "< Received unknown network sync packet type." << std::endl;
				}
			} catch(...) {
				std::exception_ptr p = std::current_exception();
				std::cout << "< Error processing sync message: " << (p ? p.__cxa_exception_type()->name() : "unknown") << std::endl;
			}
		}
	}
	
	void clearSendQueue() {
		std::deque<NetworkMessage> empty;
		std::swap(this->send_queue, empty);
	}
	
	void clearOutgoingCommands() {
		std::deque<OutgoingCommand> empty;
		std::swap(this->outgoing_commands, empty);
	}
	
	void sendMessages(unsigned long long frame_number) {
		// if a slave with no daemon, do not send.
		if(!this->daemon && !this->slaved) {
			this->clearSendQueue();
			return;
		}
		
		// if a slave that hasn't received its first pong, do not send commands
		if(!this->daemon && this->first_ping) {
			this->clearOutgoingCommands();
		}
		
		if(this->next_command_time < frame_number) {
			this->next_command_time = frame_number + RAZOR_SYNC_COMMAND_DELAY;
			this->queueOutgoingCommands();
		}
		
		while(this->send_queue.size() != 0) {
			auto nm = this->send_queue.front();
			this->send_queue.pop_front();
			int length = this->serializeMessage(this->send_buffer, &nm);
			std::string message_serialized;
			message_serialized.resize(length);
			message_serialized.assign(this->send_buffer, length);
			bool result = false;
			//std::cout << "< Sending message to " << nm.dest_host_and_port << " : " << message_serialized << std::endl;
			if(nm.dest_host_and_port == RAZOR_SYNC_BROADCAST) {
				result = this->connection.sendAll(message_serialized);
			} else {
				result = this->connection.send(nm.dest_host_and_port, message_serialized);
			}
			if(!result) {
				std::cout << "< Failed to send packet" << std::endl;
			}
		}
	}
	
	void updateFutureTime() {
		this->target_future_time = this->calculateLocalTimeDifference();
		this->future_time = this->target_future_time;
		
		//(this->target_future_time - this->future_time) 
		//* RAZOR_SYNC_FUTURE_TIME_TRACK_TARGET + this->future_time;
		//std::cout << "target_future_time " << this->target_future_time / NANOS_PER_MILLI << std::endl;
		//std::cout << "future_time " << this->future_time / NANOS_PER_MILLI  << std::endl;
		
		// TODO: set local time difference
		//this->server->setLocalTimeDifference(this->future_time);
		
		// TODO: RAZOR_SYNC_MAX_FUTURE_TIME_HIGH_PING
	}
	
	void daemonTick(unsigned long long frame_number) {
		if(this->next_sync_tick <= frame_number) {
			this->next_sync_tick = frame_number + RAZOR_SYNC_SYNC_DELAY;
			this->sendSync(RAZOR_SYNC_BROADCAST, frame_number); //this->last_sync_tick); delta syncs need work
			this->last_sync_tick = frame_number;
		}
	}
	
	void slaveTick(unsigned long long frame_number) {
		this->connectIfNeeded();
		this->updateFutureTime();
		
		auto now = misc::nanoNow();
		
		/* TODO: connection event
		if(this->create_player && !this->first_ping) {
			if(now > this->create_player_delay) {
				this->create_player = false;
				if(this->server->getDataInternal(std::string("player-").append(local_player_name)) == NULL) {
					this->server->console(std::string("player add player-").append(local_player_name).append(" unit-").append(local_player_name));
				}
				this->set_team = true;
				this->set_team_delay = now + RAZOR_SYNC_SET_TEAM_DELAY;
			}
		}
		if(this->set_team && now > this->set_team_delay) {
			//TODO: Clean up set_team
			//this->server->console(std::string("scenario setplayerteam ").append(local_player_name).append(" ").append(TEAM_1_NAME));
			this->set_team = false;
		}*/
		if(this->next_ping_time <= now) {
			//std::cout << "here " << now << " " << this->next_ping_time << std::endl;
			this->next_ping_time = now + RAZOR_SYNC_PING_DELAY;
			this->sendPing(this->daemon_host_and_port);
		}
	}
	
	void tick(unsigned long long frame_number, unsigned long long zero_time) {
		this->receiveMessages(frame_number, zero_time);
		
		if(this->daemon) {
			this->daemonTick(frame_number);
		} else {
			this->slaveTick(frame_number);
		}
		
		this->sendMessages(frame_number);
	}
	
	// TODO: Replace console commands with functions
	/*bool console(std::vector<std::string> parts, bool broadcast) {
		if(misc::comGet(parts,0) == "networkport") {
			std::string port_str = misc::comGet(parts,1);
			int port = std::stoi(port_str);
			
			std::string remote;
			if(this->daemon) {
				remote = CONNECTION_ANY;
			} else {
				remote = this->daemon_host_and_port;
			}
			
			if(this->connection.openSocket(port, remote))
				std::cout << "< Listening on port " << port << std::endl;
			else
				std::cout << "< Failed to open listening port " << port << std::endl;
			return true;
		} else if(misc::comGet(parts,0) == "enablecompression") {
			this->connection.enableCompression();
			return true;
		} else if(misc::comGet(parts,0) == "daemon") {
			this->daemon = true;
			std::cout << "< Activating daemon mode" << std::endl;
			return true;
		} else if(misc::comGet(parts,0) == "lognetworking") {
			this->connection.enableLogging();
			std::cout << "< Enabling network logging" << std::endl;
			return true;
		} else if(misc::comGet(parts,0) == "daemonaddr") {
			this->daemon = false;
			this->daemon_host_and_port = misc::comGet(parts,1);
			std::cout << "< Connecting to daemon at " << this->daemon_host_and_port << std::endl;
			return true;
		} else {		
			// capture broadcast commands for synchronization
			// if the command doesn't exist, it is from local, and we should queue it for sending
			if(this->daemon_host_and_port != "" && broadcast) {
				unsigned long long now_tick = this->server->frame_number;
				
				std::stringstream ss;
				for(int i=0; i<parts.size(); i++) {
					ss << parts[i] << " ";
				}
				std::string command = ss.str();
				this->sendCommand(command, now_tick);
			}
		}
		return false;
	}*/
};
