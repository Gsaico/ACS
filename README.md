
Access Control System
=========================

[TOC]

#Project Summary
The Access Control System (ACS) is a collection of hardware and software components that provides authorised and recorded access to doors and the usage of equipment throughout the GCTechspace facility.


#Software Design
The overall software design of the ACS is split into two (2) sections, Client and Server. A Server node authorises Clients via wireless means and hosts an internal web server that an administrator can access and manage users with. Clients connect to the Server node and register themselves as Client nodes. Clients can request access via Server validation from User driven requests. Clients can also operate in a Server less environment, with limited capability.

## Client node
Each Client node is registered with the Server node on boot and acts as a method of allowing or denying access to doors, lab equipment and other devices such as vending machines.

###Operational modes
These are the modes of operation for a Client Node:

* Normal mode (connected to Server Node)
* Fail-safe mode (disconnected from Server Node)
* User Management mode (add user using master key)

####Normal mode
The Client Node is successfully connected to the Server Node and is able to send and receive messages. The master key can add users to this node, via this node.

####Fail-safe mode
The Client Node is not connected to the Server Node but periodically checks to reconnect. Only keys stored internally (including the master key) will be allowed access to this node. The master key can add users to this node, via this node.

####User Management mode
The Client Node is not connected to the Server Node but periodically checks to reconnect. Only keys stored internally (including the master key) will be allowed access to this node. The master key can add users to this node, whilst in this mode. The master key must be used to add Users to the Client locally. The Client Node will then synchronise any new Users with the Server once it has re-established a connection.

##Server Node
Raspberry Pi / Beaglebone driven Linux system platform.

...Insert content here about server / client communication method...

### Web administration
The Server hosts an internal Web Server that can be used by an Administrator to manage Users.
A typical L.A.M.P setup can be used for this. Laravel as a PHP MVC framework would be the preferred stack to develop the administration console with. A mySQL database is used to store User access and User account details. Client nodes must be authorised by the web admin console. A unique UUID is used to identify Client Nodes when they are making requests to the Server.


##User Stories (User's perspective)
* As a User, I want to enter / exit via the front door with my RF ID access card.

* As a User, I want to enter / exit a room with my RF ID access card.

* As a User, I want to use a piece of lab equipment with my RF ID access card.

* As a User, I want to see my remaining credit on my account using my RF ID access card

* As a User, I want to use credit on my account on the vending machines using my RF ID access card

##User Stories (GCTS' perspective)
* As GCTS, I want to manage Users via a web interface.

* As GCTS, I want to Grant User access to lab equipment via a double tap, master-key / new user RF ID authorised sequence

* As GCTS, I want to log all access attempts

* As GCTS, I want Client Nodes to be connected to the Server Node via wireless

* As GCTS, I want Client Nodes to time sync with the Server Node.






##Use Cases
Here are some of the use cases for the ACS as sequence diagrams:


###General Door Access
```sequence
User->Client Node: Touch RFID card to access point at front door
Note right of Client Node: Client reads RFID UUID
Client Node->Server Node: "Access Attempted" message request
Note right of Server Node: Server records attempt
Note right of Server Node: Server retrieves record
Server Node-->Client Node: "Access Granted" message response
Client Node-->User: Display Message to User
Note right of Client Node: Open door lock
```

###General Lab Equipment Access
```sequence
User->Client Node: Touch RFID card to access point on equipment
Note right of Client Node: Client reads RFID UUID
Client Node->Server Node: "Access Attempted" message request
Note right of Server Node: Server records attempt
Note right of Server Node: Server retrieves record
Server Node-->Client Node: "Access Granted" message response
Client Node-->User: Display Message to User
Note right of Client Node: Allow equipment power on
```

###Grant a User Access to Lab Equipment
```sequence
Master Key->Client Node: Touch RFID card to access point
Note right of Client Node: Read RFID UUID\n Recognises Master Key
Client Node-->Master Key: Display "Master operation mode activated"
Note right of Client Node: Power on equipment\n Begin User registration timeout...
Master Key->Client Node: Touch RFID card to access point
Note right of Client Node: Read RFID UUID\n Recognises Master Key\n Touched within timeout?\n Yes - Power off equipment
Client Node-->Master Key: Display "Touch User to Add Access"
Note right of Client Node: Begin User registration timeout...
User->Client Node: Touch RFID card to access point
Note right of Client Node: Client reads RFID UUID\n Touched within timeout?\n Yes - adds User to internal Database
Client Node-->User: Display "User Access Granted"
```

###Client to Server user database update
```sequence
Note left of Client Node: User Update timeout elapsed?
Note left of Client Node: Yes - Any Users stored internally?
Note left of Client Node: Yes - Begin new User request
Client Node->Server Node: Send "New User" request
Note right of Server Node: Is Client Authorised?\n Yes - add User to database
Server Node-->Client Node: Send "User Added" response
Note left of Client Node: Remove User from internal database
Note left of Client Node: Any more Users to add?\n Yes - repeat above sequence
```

----------
#Hardware Design
## ...Insert content here...
### GCDuino..?
### Isolated switch - solid state relay..?
### Display (OLED)...?
### Wireless comms - nRF / WiFi?
### RF ID touch point details

----------

#Industrial Design
## ...Insert content here...
### Mechanical Drawings / Sketches...
### 3D CAD design...
### 3D printing details go here...

----------

#Contributors
[SkipXtian](https://github.com/SkipXtian)
[Ronnied](https://github.com/ronnied)

#License
[Link to License](LICENSE)
