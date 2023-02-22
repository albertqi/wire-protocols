# CS 262: Wire Protocols

## Build Information

This project uses CMake to generate build files. All you need is cmake, make (or your build system of choice), and a working c++ compiler.

Execute the following commands to compile the system:

```
mkdir build
cd build
cmake ../
make server # To compile the server
make client # To compile the client
make test   # To compile the unit tests
```

Execute the following commands to run the files:

```
./server # To run the server
./client # To run the client
./test   # To run the unit tests
```

The following commands are available to the client:

```
list [substr] # Lists all users that contain [substr]
create (user) # Creates account with username (user), which must be unique.
login (user)  # Logs in as (user), if exists.
send (user)   # Allows current user to send message to (user)
delete        # Deletes current user
exit          # Exits client
```