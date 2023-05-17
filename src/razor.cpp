#include "razor.h"

namespace razor {
	Razor::Razor() {
		this->daemon = false;
		this->slaved = false;
		this->first_ping = true;
		this->first_sync = true;
		this->create_player = false;
		this->create_player_delay = 0;
		this->set_team = false;
		this->set_team_delay = 0;
		this->daemon_host_and_port = "";
		this->future_time = 0;
		this->next_sync_tick = 0;
		this->last_sync_tick = 0;
		this->next_ping_time = 0;
		this->next_command_time = 0;
		this->ping = 0;
		this->time_delta_to_daemon = 0;
		this->destroyed = false;
		this->send_buffer = new char[SEND_BUFFER_SIZE];
		this->packed_command_buffer = new char[MAX_COMMANDS_PER_PACKET * 
													(MAX_COMMAND_LENGTH + 8)
													+ 2]; // extra 2 for number of commands
	};
	
	Razor::~Razor() {
		this->destroy();
	}
	
	void Razor::destroy() {
		if(!this->destroyed) {
			this->connection.closeSocket(); // force close
			std::cout << "< Closed networking socket." << std::endl;
			delete [] this->send_buffer;
			delete [] this->packed_command_buffer;
		}
	}
	
	nanotimediff Razor::calculateLocalTimeDifference() {
		if(this->time_delta_log.size() == 0)
			return 0;
		
		// average the time_delta_log
		double avg_td = 0;
		for(int i=0; i<this->time_delta_log.size(); i++) {
			avg_td += this->time_delta_log[i];
		}
		avg_td /= this->time_delta_log.size();
		
		
		// get the max of the ping_log
		nanotimediff max_ping = 0;
		for(int i=0; i<this->ping_log.size(); i++) {
			nanotime ping = this->ping_log[i];
			if(ping > max_ping) {
				max_ping = ping;
			}
		}
		
		// add a buffer of future time to account for ping spikes
		nanotime future_time = (max_ping/2.0f) // 1-way travel time
			* FUTURE_TIME_PING_MULTIPLIER // ping spike multiplier
			+ FUTURE_TIME_PING_FIXED; // fixed ping spike headroom
		
		//std::cout << "avg_td " << avg_td / NANOS_PER_MILLI;
		//std::cout << " max_ping " << max_ping / NANOS_PER_MILLI;
		//std::cout << " ping_headroom " << ping_headroom / NANOS_PER_MILLI << std::endl;
		
		this->ping = max_ping / NANOS_PER_MILLI;
		this->time_delta_to_daemon = avg_td / NANOS_PER_MILLI;
		this->future_time = future_time / NANOS_PER_MILLI;
		
		return avg_td - future_time;
	}
	
	// returns number of bytes copied
	int Razor::serializeMessage(void* data, NetworkMessage* in) {
		int pos = 0;
		pos += copyIn(data, pos, in->type);
		pos += copyIn(data, pos, in->timestamp);
		pos += copyIn(data, pos, in->ticknumber);
		pos += copyInString(data, pos, &in->message);
		return pos;
	}
	
	// returns number of bytes copied
	int Razor::deserializeMessage(NetworkMessage* out, void* data) {
		int pos = 0;
		pos += copyOut(&out->type, data, pos);
		pos += copyOut(&out->timestamp, data, pos);
		pos += copyOut(&out->ticknumber, data, pos);
		pos += copyOutString(&out->message, data, pos);
		return pos;
	}
	
	int Razor::serializePong(char* data, nanotime remote_timestamp, nanotime zero_time) {
		int pos = 0;
		pos += copyIn(data, pos, remote_timestamp);
		pos += copyIn(data, pos, zero_time);
		return pos;
	}
	
	int Razor::deserializePong(char* data, nanotime *start_timestamp, nanotime *zero_time) {
		int pos = 0;
		pos += copyOut(start_timestamp, data, pos);
		pos += copyOut(zero_time, data, pos);
		return pos;
	}
	
	int Razor::serializeCommand(char* data, int pos, ticktype tick_number, std::string* command) {
		if(command->length() > MAX_COMMAND_LENGTH) {
			std::cout << "< WARNING: Command serialization over length: " << command->length() 
					<< " > " << MAX_COMMAND_LENGTH << " [" << *command << "]" << std::endl;
		}
		int len = 0;
		len += copyIn(data, pos+len, tick_number);
		len += copyInString(data, pos+len, command);
		return len;
	}
	
	int Razor::deserializeCommand(char* data, int pos, ticktype *tick_number, std::string *command) {
		int len = 0;
		len += copyOut(tick_number, data, pos+len);
		if(len > MAX_COMMAND_LENGTH) {
			std::cout << "< WARNING: Command deserialization over length: " << len 
					<< " > " << MAX_COMMAND_LENGTH << std::endl;
		}
		len += copyOutString(command, data, pos+len);
		return len;
	}
	
	void Razor::queueOutgoingNetworkMessage(const std::string &dest, unsigned char type, const std::string& message) {
		NetworkMessage nm;
		nm.origin_host_and_port = LOCAL;
		nm.dest_host_and_port = dest;
		// TODO: setup current frame number
		nm.ticknumber = 0;//this->server->tick_number;
		nm.timestamp = razor::nanoNow();
		nm.type = type;
		nm.message = message;
		this->send_queue.push_back(nm);
	}
	
	void Razor::queueOutgoingCommands() {
		std::string dest;
		if(this->daemon) {
			dest = BROADCAST;
		} else {
			if(!this->slaved) { // if not a daemon and not slaved, clear the queue and do nothing
				this->clearOutgoingCommands();
				return;
			}
			dest = this->daemon_host_and_port;
		}
		
		unsigned short command_counter = 0;
		int pos = 2; // 2 because the number of 
		while(this->outgoing_commands.size() > 0) {
			auto out_command = this->outgoing_commands.front();
			this->outgoing_commands.pop_front();
			
			//std::cout << "< Sending command: " << out_command.command << std::endl;
			
			// discard long commands
			if(out_command.command.size() > MAX_COMMAND_LENGTH) {
				std::cout << "< Command exceeds maximum length for network sync: " << out_command.command << std::endl;
				continue;
			}
			pos += this->serializeCommand(packed_command_buffer, pos, out_command.tick_number, &out_command.command);
			if(command_counter == MAX_COMMANDS_PER_PACKET) {
				// if the max commands is reached, send a packed packet
				std::string temp;
				copyIn(this->packed_command_buffer, 0, command_counter); // number of commands at beginning
				temp.resize(pos);
				temp.assign(this->packed_command_buffer, pos);
				queueOutgoingNetworkMessage(dest, MESSAGE_COMMAND, temp);
				pos = 2;
				command_counter = 0;
			} else {
				command_counter++;
			}
		}
		
		if(command_counter > 0) {
			// send any remaining commands in another packed packet
			std::string temp;
			copyIn(this->packed_command_buffer, 0, command_counter); // number of commands at beginning
			temp.resize(pos);
			temp.assign(this->packed_command_buffer, pos);
			queueOutgoingNetworkMessage(dest, MESSAGE_COMMAND, temp);
		}
	}
	
	
	// Message send functions
	void Razor::sendRequestFullSync() {
		std::string empty;
		this->queueOutgoingNetworkMessage(
				this->daemon_host_and_port, MESSAGE_REQUEST_FULL, empty);
	}
	
	// Pongs also return the requester's timestamp
	void Razor::sendPong(std::string dest, nanotime remote_timestamp) {
		auto zero_time = this->local_zero_time;
		char pong_data[16]; // two 8 byte long longs
		serializePong(pong_data, remote_timestamp, zero_time);
		std::string pong_str;
		pong_str.resize(16);
		pong_str.assign(pong_data, 16);
		this->queueOutgoingNetworkMessage(dest, MESSAGE_PONG, pong_str);
	}
	
	// Ping the server
	void Razor::sendPing(std::string dest) {
		std::string ping_str = " ";
		this->queueOutgoingNetworkMessage(dest, MESSAGE_PING, ping_str);
	}
	
	void Razor::sendDisconnect(std::string dest) {
		std::string empty;
		this->queueOutgoingNetworkMessage(
			dest, MESSAGE_DISCONNECT, empty);
	}
	
	void Razor::sendSync(std::string dest) {
		auto tick_number = this->local_tick_number;
        
        if(this->get_state_data_func == nullptr)
            throw std::runtime_error("registerGetStateDataFunc must be called before sendSync");
        
		std::string state;
        (*this->get_state_data_func)(&state);
		
		int pos = 0;
		pos += copyIn(send_buffer, pos, tick_number);
		pos += copyInString(send_buffer, pos, &state);
		
		std::string message;
		message.resize(pos);
		message.assign(send_buffer, pos);
		
		this->queueOutgoingNetworkMessage(dest, MESSAGE_SYNC, message);
		
		std::cout << "< Sending full sync to " << dest << std::endl;
	}
	
	void Razor::sendCommand(const std::string& command) {
		auto tick_number = this->local_tick_number;
		OutgoingCommand o;
		o.tick_number = tick_number;
		o.command = command;
		outgoing_commands.push_back(o);
	}
	
	// Common processes
	void Razor::connectIfNeeded() {
		if(!this->daemon && !this->slaved) {
			// do not try to become slaved if there is no specified daemon
			if(this->daemon_host_and_port == "")
				return;
			
			this->sendRequestFullSync();
			this->slaved = true;
		}
	}
	
	void Razor::receivePong(NetworkMessage* nm) {
		auto zero_time = this->local_zero_time;
		
		if(this->daemon) // deamons ignore pongs -- they have no need to synchronize
					return;
		
		unsigned long long start_timestamp;
		deserializePong((char*)nm->message.c_str(), &start_timestamp, &(this->daemon_zero_time));
		unsigned long long bounce_timestamp = nm->timestamp;
		unsigned long long end_timestamp = razor::nanoNow();
		
		// clean ping log
		while(this->ping_log.size() >= PING_LOG_SIZE)
			this->ping_log.pop_front();
		
		// clean time delta log
		while(this->time_delta_log.size() >= PING_LOG_SIZE)
			this->time_delta_log.pop_front();
		
		// the ping is the round trip time from slave->daemon->slave
		nanotime ping = end_timestamp - start_timestamp;
		this->ping_log.push_back(ping);
		
		// time_delta is estimated difference between the two time systems' 
		// zero_time (from local to daemon).
		// time delta is estimated by using the local half-ping timestamp
		// and the server's "midway" timestamp adjusted
		// they should correspond roughly to the same absolute time, so
		// the difference will be a rough time delta.
		// over time, the log will average out a reasonable time delta
		nanotimediff time_delta = (start_timestamp + ping/2 - zero_time) -
									(bounce_timestamp - this->daemon_zero_time);
		this->time_delta_log.push_back(time_delta);
		
		/*std::cout << "ping " << ping / NANOS_PER_MILLI << std::endl;
		std::cout << "bounce_timestamp " << bounce_timestamp / NANOS_PER_MILLI << std::endl;
		std::cout << "daemon_zero_time " << this->daemon_zero_time / NANOS_PER_MILLI << std::endl;
		std::cout << "time_delta " << time_delta / NANOS_PER_MILLI << std::endl;/**/
		
		// if this is the first ping, use the ping's data to initialize the future ticks
		if(this->first_ping) {
			this->future_time = this->calculateLocalTimeDifference();
			this->first_ping = false;
		}
	}
	
	void Razor::receiveCommands(NetworkMessage* nm) {
		auto tick_number = this->local_tick_number;
		unsigned short commands_number;
		const char* buffer = nm->message.c_str();
		int pos = 0;
		pos += copyOut(&commands_number, (void*)buffer, pos);
		if(commands_number > MAX_COMMANDS_PER_PACKET) {
			std::cout << "< Received command packet with too many commands (" << commands_number << ")" << std::endl;
			return;
		}
		for(int i=0; i<commands_number; i++) {
			ticktype tick_number;
			std::string command;
			pos += deserializeCommand((char*)buffer, pos, &tick_number, &command);
			if(command.size() > MAX_COMMAND_LENGTH) {
				std::cout << "< Received command over size limit (" << command.size() << ")" << std::endl;
				return;
			}
			if(this->daemon && tick_number < tick_number) {
				std::cout << "< Received command in the past, discarding (received " << tick_number
							<< " vs now " << tick_number << ")" << std::endl;
				return;
			}
			if(this->daemon && tick_number - tick_number > COMMAND_MAX_FUTURE) {
				std::cout << "< Received command too far in the future (" << tick_number << ") for now "
							<< "(" << tick_number << ")" << std::endl;
				return;
			}
			
			// if validated log the command
			auto tick_and_command = std::to_string(tick_number).append(" ").append(command);
			
			if(this->daemon) {
				//std::cout << "< Received command @ " << this->server->tick_number << " : " << tick_and_command << std::endl;
				sendCommand(command);
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
	
	void Razor::receiveSync(NetworkMessage* nm) {
		if(this->daemon) { // daemons do not receive syncs
			return;
		} else if(!this->slaved) { // if not a daemon and not slaved, do nothing
			return;
		}
		
		// Set the server's tick time to the sync message's
		// load in the gamedata sync
		// Set the server's local_time_difference to the correct amount of future_ticks
		ticktype daemon_tick_number;
		std::string state;
		
		char* data = (char*)nm->message.c_str();
		
		int pos = 0;
		pos += copyOut(&daemon_tick_number, data, pos);
		pos += copyOutString(&state, data, pos);
		
		//std::cout << "< Sync daemon tick: " << daemon_tick_number << ". Local tick: " << this->server->tick_number << std::endl;
		
		nanotimediff local_time_difference = this->future_time;
		
		if(this->first_sync) {
			this->first_sync = false;
			this->create_player = true;
			this->create_player_delay = razor::nanoNow() + CREATE_PLAYER_DELAY;
			//std::cout << "Create player delay " << this->create_player_delay << " " << CREATE_PLAYER_DELAY << std::endl;
			// for the first sync, do a hard non-background resync. This is necessary to synchronize
			// frame numbers and future time.
			
			// TODO: initial resync
			//this->server->queueResync(&state, daemon_tick_number, local_time_difference);
		} else {
			// I am leaving this non-background resync here for benchmarking purposes
			//this->server->queueResync(&state, daemon_tick_number, local_time_difference);
			
			// Force a "priority" replay. This causes all other replays in queue to be deleted and causes the whole server
			// to be deserialized once updated.
			
			// inter-heuristic talk is dangerous, but this is necessary because the state might be large.
			
			// TODO: remake state resynchronization
			/*auto ts = (TrackerStates*)this->server->getHeuristic(HEURISTIC_TRACKER_STATES);
			if(ts != nullptr && ts->replay_runner != nullptr) {
				// replay from the daemon tick's number to our current frame.
				std::cout << "< Queuing background netsync replay. Server0 frame: " << this->server->tick_number <<
					" Daemon frame: " << daemon_tick_number << std::endl;
				ts->replay_runner->startNewReplay(&state, daemon_tick_number, this->server->tick_number, 
					local_time_difference, this->server->zero_time, 
					true); // priority = true
			}/**/
		}
	}
	
	void Razor::receiveMessages() {
		auto tick_number = this->local_tick_number;
		auto zero_time = this->local_zero_time;
		
		std::string host_and_port, message;
		
		while(this->connection.receive(&host_and_port, &message)) {
			NetworkMessage nm;
			nm.origin_host_and_port = host_and_port;
			nm.dest_host_and_port = LOCAL;
			
			try {
				this->deserializeMessage(&nm, (void*)message.c_str());
				
				if(nm.type == MESSAGE_COMMAND) {
					//std::cout << "< Received command" << std::endl;
					this->receiveCommands(&nm);
				} else if(nm.type == MESSAGE_SYNC) {
					std::cout << "< Received sync" << std::endl;
					this->receiveSync(&nm);
				} else if(nm.type == MESSAGE_PONG) {
					std::cout << "< Received pong" << std::endl;
					this->receivePong(&nm);
				} else if(nm.type == MESSAGE_REQUEST_FULL) {
					if(!this->daemon) // slaves should ignore sync requests
						continue;
					std::cout << "< Received request full sync" << std::endl;
					this->sendPong(nm.origin_host_and_port, nm.timestamp);
					this->sendSync(nm.origin_host_and_port);
				} else if(nm.type == MESSAGE_PING) {
					if(!this->daemon) // slaves should ignore ping requests
						continue;
					this->sendPong(nm.origin_host_and_port, nm.timestamp);
				} else if(nm.type == MESSAGE_DISCONNECT) {
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
	
	void Razor::clearSendQueue() {
		std::deque<NetworkMessage> empty;
		std::swap(this->send_queue, empty);
	}
	
	void Razor::clearOutgoingCommands() {
		std::deque<OutgoingCommand> empty;
		std::swap(this->outgoing_commands, empty);
	}
	
	void Razor::sendMessages(ticktype tick_number) {
		// if a slave with no daemon, do not send.
		if(!this->daemon && !this->slaved) {
			this->clearSendQueue();
			return;
		}
		
		// if a slave that hasn't received its first pong, do not send commands
		if(!this->daemon && this->first_ping) {
			this->clearOutgoingCommands();
		}
		
		// wait for the next command interval to send a batch of commands
		if(this->next_command_time < tick_number) {
			this->next_command_time = tick_number + COMMAND_DELAY;
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
			if(nm.dest_host_and_port == BROADCAST) {
				result = this->connection.sendAll(message_serialized);
			} else {
				result = this->connection.send(nm.dest_host_and_port, message_serialized);
			}
			if(!result) {
				std::cout << "< Failed to send packet" << std::endl;
			}
		}
	}
	
	void Razor::updateFutureTime() {
		this->future_time = this->calculateLocalTimeDifference();
		// TODO: set local time difference
		//this->server->setLocalTimeDifference(this->future_time);
	}
	
	void Razor::daemonTick() {
		auto tick_number = this->local_tick_number;
		if(this->next_sync_tick <= tick_number) {
			this->next_sync_tick = tick_number + SYNC_DELAY;
			this->sendSync(BROADCAST); //this->last_sync_tick); delta syncs need work
			this->last_sync_tick = tick_number;
		}
	}
	
	void Razor::slaveTick(ticktype tick_number) {
		this->connectIfNeeded();
		this->updateFutureTime();
		
		auto now = razor::nanoNow();
		
		/* TODO: connection event
		if(this->create_player && !this->first_ping) {
			if(now > this->create_player_delay) {
				this->create_player = false;
				if(this->server->getDataInternal(std::string("player-").append(local_player_name)) == nullptr) {
					this->server->console(std::string("player add player-").append(local_player_name).append(" unit-").append(local_player_name));
				}
				this->set_team = true;
				this->set_team_delay = now + SET_TEAM_DELAY;
			}
		}
		if(this->set_team && now > this->set_team_delay) {
			//TODO: Clean up set_team
			//this->server->console(std::string("scenario setplayerteam ").append(local_player_name).append(" ").append(TEAM_1_NAME));
			this->set_team = false;
		}*/
		if(this->next_ping_time <= now) {
			//std::cout << "here " << now << " " << this->next_ping_time << std::endl;
			this->next_ping_time = now + PING_DELAY;
			this->sendPing(this->daemon_host_and_port);
		}
	}
	
	void Razor::tick(unsigned long long tick_number, unsigned long long zero_time) {
		this->local_tick_number = tick_number;
		this->local_zero_time = zero_time;
		
		this->receiveMessages();
		
		if(this->daemon) {
			this->daemonTick();
		} else {
			this->slaveTick(tick_number);
		}
		
		this->sendMessages(tick_number);
	}
	
	void Razor::setPort(int port) {
		std::string remote;
		if(this->daemon) {
			remote = ANY_ADDRESS;
		} else {
			remote = this->daemon_host_and_port;
		}
		
		if(this->connection.openSocket(port, remote))
			std::cout << "< Listening on port " << port << std::endl;
		else
			std::cout << "< Failed to open listening port " << port << std::endl;
	}
	
	void Razor::setDaemon(bool is_daemon) {
		this->daemon = is_daemon;
		if(is_daemon) {
			std::cout << "< Activating daemon mode" << std::endl;
		} else {
			std::cout << "< Activating slave mode" << std::endl;
		}
	}
	
	void Razor::setDaemonAddress(const std::string &daemon_host_and_port) {
		this->daemon_host_and_port = daemon_host_and_port;
	}
	
	void Razor::setLogNetworking() {
		this->connection.enableLogging();
	}
	
	void Razor::command(const std::string &command_data) {
		this->sendCommand(command_data);
	}
    
    void Razor::registerCallbackGetStateData(void (*get_state_data_func)(std::string*)) {
        this->get_state_data_func = get_state_data_func;
    }
	
    void testGetStateData(std::string*) {
    }
    
	int razorUnitTest() {
		std::cout << "Creating server..." << std::endl;
		auto s = new Razor();
		s->setPort(12320);
		s->setDaemon();
        s->registerCallbackGetStateData(&testGetStateData);
		
		std::cout << "Creating client..." << std::endl;
		auto c = new Razor();
		c->setPort(12321);
		c->setDaemonAddress("127.0.0.1:12320");
        c->registerCallbackGetStateData(&testGetStateData);
		
		// Server warmup frames
		int sbt = 100;
		for(int i=0; i <= sbt; i++) {
			s->tick(i, nanoNow());
		}
		
		nanotimediff error = -1555 * NANOS_PER_MILLI;
		
		// Frame 1
		int frame = 1;
		std::cout << "@ Frame" << frame << std::endl;
		s->tick(sbt+frame, nanoNow());
		// c sends request full sync
		c->tick(frame, nanoNow()+error);
		sleep(100);
		
		// Frame 2
		frame = 2;
		std::cout << "@ Frame" << frame << std::endl;
		c->tick(frame, nanoNow()+error);
		// s receives request full sync, responds with sync and pong
		s->tick(sbt+frame, nanoNow());
		sleep(100);
		
		// Frame 3
		frame = 3;
		std::cout << "@ Frame" << frame << std::endl;
		s->tick(sbt+frame, nanoNow());
		// c receives full sync and pong, updates everything
		c->tick(frame, nanoNow()+error);
		sleep(100);
		
		// Check C's state versus S's
		// Check C's future time
		
		
		delete s;
		delete c;
		
		return 0;
	}
};

