# Full Name:Zijie Fang
# Student ID:5618388472

## What you have done in the assignment.
### Client:
* TCP connects with the aws
* Send MapID, source and file size
* Get the min distance, Tt, Tp and end-to-end delay

### Aws:
* Start TCP
* TCP connects with the client, get the MapID, source and file size
* Start UDP
* Send the MapID and source to the serverA with UDP
* Get the min distance
* Send the prop speed, trans speed, file size and distance to serverB with UDP
* Get the Tt, Tp and end-to-end delay from serverB
* Send the min distance, Tt, Tp and end-to-end delay to client
* End UDP

### ServerA:
* Parse the map
* Start UDP
* Get the MapID and source from aws
* Use Dijkstra to calculate the min distance
* Send the min distance to aws

### ServerA:
* Start UDP
* Get the prop speed, trans speed, file size and distance from aws
* Calculate the Tt, Tp and end-to-end delay
* Send the min the Tt, Tp and end-to-end delay to aws

## What your code files are and what each one of them does.
Each cpp file represents a part.

serverA.cpp for serverA.
serverB.cpp for serverB.
client.cpp for client.
aww.cpp for aws.
Their functions are written above.

## The format of all the messages exchanged.
I only use char array to send the message. In fact I can send any type of pointer inside the socket. I use char array because the receiver may don't want to know the struct detail of the pointer from the sender. So char array is better for a messages.

```
Format:
* Client to aws: "<MapID> <Source> <fileSize>"
* Client from aws: "<Destination>\t\t<minLength>\t\t<Tt>\t<Tp>\t<Delay>\n" for each destination.(All inside one socket)
* Aws to serverA: "<MapID> <Source> <fileSize>"(In fact the serverA don't need fileSize, just submit for easy)
* Aws from serverA: "<prop>T<trans>D"+"<Destination>\t\t<minLength>\n" for each destination.(All inside one socket)
* Aws to serverB: "<fileSize>P<prop>T<trans>D"+"<Destination>\t\t<minLength>\n" for each destination (P is the delimiter)
* Aws from serverB: "<Destination>\t\t<Tt>\t<Tp>\t<Delay>\n" for each destination(All inside one socket)
```

## Any idiosyncrasy of your project. It should say under what conditions the project fails, if any.
If input format error. It will failed. (Anyway, the project says we don't need to check the input error)

If two clients at the same time. I don't consider any mutex. So no multiple thread.

## Reused Code: Did you use code from anywhere for your project? If not, say so. If so, say what functions and where they're from.
http://beej.us/guide/bgnet/examples/server.c

http://beej.us/guide/bgnet/examples/client.c

http://beej.us/guide/bgnet/examples/listener.c

http://beej.us/guide/bgnet/examples/talker.c
