# Razor C++ State Synchronization (Netcode) Library

Razor synchronizes the state of video game clients with a server over
the internet.

Features include:

* A light-weight, low latency, immediate-mode API for clients and servers
* Compatible with any type of data state from FPS games to strategy to realtime business applications
* Engine-agnostic design that can be integrated into small and large projects
* Data types that automatically serialize and deserialize

# Razor's Objective

The main problem that is difficult to solve in network state
synchronization (NSS) is how to account for the race conditions induced by
the latency inherent in sending packets over the internet?

The second and related problem is after you have received new information
from a remote source that affects your local state, how do you gracefully
update your state to account for the new information?

Razor's objective is to automate the process of keeping remote states
synchronized and gracefully updating the local state when new information
arrives.

# What does Razor NOT do?

Most netcode is integrated deep into a gaming engine because it is difficult
to separate the intricate inner workings of netcode's state manipulation from
the actual data state.

Razor applies the principle of "bring your own state manipulation" so Razor's netcode
may be shared with many types of engines.

Razor assumes your application is capable of the following:

* Serializing a data state (helper data types are included in Razor)
* Deserializing a foreign data state and actualizing it into the current state (helpers included)
* Manipulating the application's current frame number
* Serialization and deserialization of commands

Additionally, you can increase the effectiveness of your application's state manipulation
with the following additional features:

* Ability to rewind and fast-forward through time
* Ability to record and change commands with awareness of authority

# Razor's Approach

Razor uses three separate channels of communication to orchestrate synchronization:

* Data States
* Commands
* Events

*Data States* are raw serializations of the server's state that are periodically shared with
clients. Data States allow clients to absolutely synchronize with the server, except for
the latency the state took to get to the client. Because Data States are relatively large
and processing intensive, they are only shared occassionally and other more optimal approaches
are used to make up the difference. Because states are received late due to network latency,
an optional mitigation is for your application to simulate out to the future
from that state, so your client is running at the timeframe of when commands would reach the
server.

*Commands* are your application's notation for things that users do. Razor provides an 
automatic method of broadcasting them among the clients. Razor provides no guarantees
on delivery of user commands, or whether the same command is received multiple times.
It will be your responsibility to play them correctly in your application's logic, 
to ensure if there are lost packets that the commands are rebroadcast, and to ensure if
a duplicate command is received it is caught.

*Events* are authoritative chronologically-recorded conclusions to application logic.
Events have a distinct difference from commands, because they are only generated by the server,
are guaranteed to be delivered, and guaranteed to have exact synchronicity between one 
event generated on the server to one event received on a client.

# How to run Razor

(Documentation is under development)