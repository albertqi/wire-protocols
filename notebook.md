# Design Ideas

Our initial, high-level idea was as follows:
1. Upon starting up, the user types in a username. This will either create an account or login with the server. We later changed this to have register and login be separate commands.
2. When the user types `list`, the server will dump its stored account list.
3. When the user is logged in and types `send`, the user can choose to whom they want to send a message. Then, they will type their message, which will ultimately get stored on the server in a queue. We should probably keep track of both the sender and the receiver for every message.
4. When the user is logged in and types `delete`, their account will be deleted. This will not remove any queued messages to or from the user.

How do we handle multiple clients? There are a few options:
1. Serially handle clients (i.e., only one client is allowed to be logged in at a time). This is probably not the greatest idea.
2. Multiplex incoming connections. This is probably easier to implement, but slower.
3. Spawn a thread for each new incoming connection. This will result in a faster and better user experience, but we will have to deal with locking and synchronization.

We chose to handle multiple clients by spawning a new thread for each connection since it would allow for the best user experience.

How do we handle undefined sized messages?
1. We first send the size of the message, which is defined in the header. Then, we are able to read however many bytes are required for the message.

How do the client and server know what type of data is being received?
1. We chose to add operation codes (hereinafter called "op codes"). Each message would have an op code to define what kind of operation was being performed.

We had separate `User` and `Message` metadatas that we consolodated into one `Metadata` struct.