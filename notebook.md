# Design Ideas

Our initial, high-level idea was as follows:
1. Upon starting up, the user types in a username. This will either create an account or login with the server. We later changed this to have register and login be separate commands.
2. When the user types `list`, the server will dump its stored account list. We later added substring search functionality.
3. When the user is logged in and types `send`, the user can choose to whom they want to send a message. Then, they will type their message, which will ultimately get stored on the server in a queue. We should probably keep track of both the sender and the receiver for every message.
4. When the user is logged in and types `delete`, their account will be deleted. This will not remove any queued messages to or from the user.

There were a lot of questions that we needed to answer.

How do we handle multiple clients? There are a few options:
1. Serially handle clients (i.e., only one client is allowed to be logged in at a time). This is probably not the greatest idea.
2. Multiplex incoming connections. This is probably easier to implement, but slower.
3. Spawn a thread for each new incoming connection. This will result in a faster and better user experience, but we will have to deal with locking and synchronization.

We chose to handle multiple clients by spawning a new thread for each connection since it would allow for the best user experience.

How do we handle undefined sized messages? We first send the size of the message, which is defined in the header. Then, we are able to read however many bytes are required for the message.

We had separate `User` and `Message` metadatas that we consolodated into one `Metadata` struct.

How do the client and server know what type of data is being received? We chose to add operation codes (hereinafter called "op codes"). Each message would have an op code to define what kind of operation was being performed.

Here, we decided to add callbacks to handle operations in a more isolated manner. This should also somewhat replicate how gRPC works, making it easier to transition later to gRPC.

How can the client receive messages from the server and send messages at the same time? We chose to spawn a new thread that just continually checks for new messages.

We ran into an issue with synchronization, where if the user typed multiple commands fast enough, then the later commands might have run before the previous ones were finished. We chose to fix this issue by adding operation locks (i.e., locking before every operation and releasing the lock at the end).

The transition to gRPC was fairly easy since we just needed to replace parts of our code with gRPC code. This means that the complexity of the code was similar between the two versions.

The buffer size with the wire protocol is the standard socket buffer size, and the buffer size for gRPC is 4 MB.

## Replication

We decided there were four main things to accomplish to achieve two-fault tolerance and persistance:
1. Move in-memory data structures to a persistent database.
2. Forward requests received on the primary to the replicas.
3. Perform leader-election when the primary goes down.
4. Reconnect the client to a replica when the currently connected server goes down.

We decided to use a leader-follower system because it lent itself well to our architecture. The leader could just act like a client to the replicas and forward requests it received, and they would be handled automatically by the existing code.

Fundamentally, there weren't that many changes we needed to make to get these results. The overall architecture and design of the system remained the same. Some major changes are the database setup/code in the server (including re-syncing the databases between system starts), client reconnection, some additional operations to our wire-protocols (as well as handling larger transmission sizes), and some clever handling of network connections.

All of the tests from the previous part also remain passing after some small changes to the setup code.
