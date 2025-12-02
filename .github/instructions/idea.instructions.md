---
applyTo: '**'
---
#1. Project Overview
🎯 Project Title

Who Wants to Be a Millionaire — Multiplayer TCP Socket Game

🎯 Project Goals

Build a complete online multiplayer game system that allows players to:

Register & log in

Add friends & view online status

Chat privately or in rooms

Create or join multiplayer rooms

Play the Millionaire-style quiz game in 2 modes:

Basic Mode (solo)

1vN Mode (competitive)

View profile, statistics, leaderboard

Track game history and performance

All communication uses custom TCP application-layer protocol with binary header + JSON payload.

#2. Game Features
🔐 2.1. Authentication Flow

Client launches & connects to server via TCP.

User chooses Register or Login.

On successful login, server returns:

User profile

Friend list

Statistics

Offline messages

On failure → NOTIFY_ERROR

🏠 2.2. Main Lobby

Includes:

Avatar, username

Friend list (online / offline / in-game)

Main actions:

Create Room

Join Room

Quick Match (Basic Mode)

Leaderboard

Statistics

👥 2.3. Friends System

Search user by ID

Send friend request

Receive friend request

Accept / reject friend request

Real-time friend status update

Invite friend to room

💬 2.4. Chat System

DM chat (1–1)

Room chat

Offline message retrieval

🎮 2.5. Game Modes
A. Basic Mode (Solo)

15 questions:

5 Easy

5 Medium

5 Hard

One wrong answer → eliminated

One lifeline: 50:50

Answer all correctly → win

B. 1vN Competitive Mode

N players in a room

All players answer simultaneously

Faster answer → more points

Wrong → eliminated

Only one remaining → winner

📊 2.6. Stats & Match Result

Server stores:

Total matches

Win rate

Highest score

Total correct answers

Average response time

Match history

Client displays:

Result screen

Leaderboard

Option to replay or return to lobby

#3. Application Protocol
🔧 Message Structure
Header (8 bytes)
Bytes	Meaning
0–1	CommandType
2–3	UserId
4–7	PayloadLength
Payload (JSON)
{
  "t": "REQ / RES / NTF",
  "request_id": "a1b2c3",
  "data": { ... }
}

#4. FULL COMMAND SET

(Taken directly from project document)

## 0x01xx – Authentication
Command	Name	Direction	Description
0x0101	REQ_REGISTER	C→S	Register account
0x0102	RES_REGISTER	S→C	Register response
0x0103	REQ_LOGIN	C→S	Login request
0x0104	RES_LOGIN	S→C	Login response
0x0105	NOTIFY_USER_STATUS	S→C	Update user status (online/offline/in-game)
0x0106	REQ_LOGOUT	C→S	Logout request
0x0107	RES_LOGOUT	S→C	Logout response
## 0x02xx – Friends System
Command	Name	Direction	Description
0x0201	REQ_ADD_FRIEND	C→S	Send friend request
0x0202	RES_ADD_FRIEND	S→C	Friend request result
0x0203	NOTIFY_FRIEND_REQUEST	S→C	Incoming friend request
0x0204	REQ_RESPOND_FRIEND	C→S	Accept/Reject friend request
0x0205	RES_RESPOND_FRIEND	S→C	Response to acceptance
0x0206	REQ_LIST_FRIENDS	C→S	Get friend list
0x0207	RES_LIST_FRIENDS	S→C	Return friend list
0x0208	NOTIFY_FRIEND_STATUS	S→C	Notify friend online/offline status
## 0x03xx – Chat
Command	Name	Direction	Description
0x0301	REQ_SEND_DM	C→S	Send private message
0x0302	RES_SEND_DM	S→C	DM sent success
0x0303	NOTIFY_DM	S→C	Receive private message
0x0304	REQ_SEND_ROOM_CHAT	C→S	Send room message
0x0305	RES_SEND_ROOM_CHAT	S→C	Room chat result
0x0306	NOTIFY_ROOM_CHAT	S→C	Broadcast room message
0x0307	REQ_FETCH_OFFLINE	C→S	Fetch offline messages
0x0308	RES_FETCH_OFFLINE	S→C	Return offline messages
## 0x04xx – Lobby / Room
Command	Name	Direction	Description
0x0401	REQ_CREATE_ROOM	C→S	Create room
0x0402	RES_CREATE_ROOM	S→C	Room creation result
0x0403	REQ_JOIN_ROOM	C→S	Join room
0x0404	RES_JOIN_ROOM	S→C	Join room result
0x0405	NOTIFY_ROOM_UPDATE	S→C	Room member update
0x0406	REQ_INVITE_FRIEND	C→S	Invite friend
0x0407	NOTIFY_ROOM_INVITE	S→C	Room invite received
0x0408	REQ_RESPOND_INVITE	C→S	Accept/Reject invite
0x0409	RES_RESPOND_INVITE	S→C	Invite result
0x040A	REQ_LEAVE_ROOM	C→S	Leave room
0x040B	RES_LEAVE_ROOM	S→C	Leave room result
0x040C	REQ_START_GAME	C→S	Host starts game
0x040D	RES_START_GAME	S→C	Start game result
## 0x05xx – Game: Basic Mode
Command	Name	Direction	Description
0x0501	NOTIFY_GAME_START	S→C	Start game
0x0502	NOTIFY_QUESTION	S→C	New question
0x0503	REQ_SUBMIT_ANSWER	C→S	Submit answer
0x0504	RES_SUBMIT_ANSWER	S→C	Answer received
0x0505	NOTIFY_ANSWER_RESULT	S→C	Correct/Wrong
0x0506	REQ_USE_LIFELINE	C→S	Use lifeline
0x0507	RES_USE_LIFELINE	S→C	Lifeline result
0x0508	NOTIFY_ROUND_END	S→C	End of round
0x0509	NOTIFY_GAME_OVER	S→C	Game end
## 0x06xx – Game: 1vN Mode
Command	Name	Direction	Description
0x0601	NOTIFY_GAME_START_1v100	S→C	Start 1vN
0x0602	NOTIFY_QUESTION_1v100	S→C	Question to all
0x0603	REQ_SUBMIT_ANSWER_1v100	C→S	Submit answer
0x0604	RES_SUBMIT_ANSWER_1v100	S→C	Answer result
0x0605	NOTIFY_ELIMINATION	S→C	Eliminated players
0x0606	NOTIFY_GAME_OVER_1v100	S→C	Game end
## 0x07xx – Stats / Leaderboard
Command	Name	Direction	Description
0x0701	REQ_GET_PROFILE	C→S	Request profile
0x0702	RES_GET_PROFILE	S→C	Profile result
0x0703	REQ_LEADERBOARD	C→S	Request leaderboard
0x0704	RES_LEADERBOARD	S→C	Leaderboard result
0x0705	REQ_MATCH_HISTORY	C→S	Request match history
0x0706	RES_MATCH_HISTORY	S→C	Match history result
## 0x08xx – System / Error / Connection
Command	Name	Direction	Description
0x0801	REQ_PING	C→S	Ping
0x0802	RES_PING	S→C	Ping response
0x0803	NOTIFY_ERROR	S→C	Generic error
0x0804	REQ_RECONNECT	C→S	Reconnect after disconnect
0x0805	RES_RECONNECT	S→C	Reconnect success
#5. High-Level Architecture
🧱 Backend (C)
server/
│── src/
│   ├── main.c
│   ├── server/
│   ├── service/
│   ├── protocol/
│   └── dao/
│── include/
│── build/
│── Makefile

🖥️ Frontend (Qt)

Login / Register Screen

Lobby

Room screen

Game screen

Chat windows

Statistics screen

🗄️ Database (PostgreSQL)

Tables:

users

sessions

friends

rooms

questions

matches

stats

#6. Project Idea Summary (For README)

A real-time multiplayer quiz game inspired by “Who Wants to Be a Millionaire”, built with:

C server using TCP sockets

Qt client UI

PostgreSQL database

Custom binary + JSON protocol

Two game modes

Friend system, messaging, lobby, rooms, ranking

Designed for performance, concurrency, and real-time multiplayer interactions.