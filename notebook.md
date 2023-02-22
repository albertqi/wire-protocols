Design Ideas:

Overview:
The idea is that when the client runs, it could ask for an account anem that will get sent to the server. The server will store that information persistantly.

The first thing to do when starting the client is type in a username which will either create an account or login with the server. So dumb client and smart server.

For list accounts just request the server and the server will dump it's account list

Logged in client can choose a user from the list and send a message to them.

Should the receiver a message know who sent it? Yeah probably.

How do we handle multiple clients?
1. Serially handle clients, i.e., only one client is allowed to be logged in at a time. Probably not a good idea.
2. Multiplex incoming connections. Probably easier to implement, but slower.
3. Spawn a thread for each new incoming connection. Have to deal with locking and syncronization. Big pain, but faster and better user experience.

How do we handle undefined sized messages?
1. 

Didn't haves op codes.

Had seperate User and Message Metadatas that we consolodated.