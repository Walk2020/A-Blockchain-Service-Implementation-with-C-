# A-Blockchain-Service-Implementation-with-C-
This is Blockchain Service Implementation with C++

clinetA and clientB are two client-side programs to send encoded requests to the main server with TCP protocol
including checking balance, checking transaction history, and making transactions.

serverM is  a main-server program to deal with requests translation and validation, data processing, communication with clients by TCP protocol, and communication with backend servers by UDP protocol.

serverA, serverB, and serverC are three backend-server programs to parse and respond to requests from the main server and manage the data storage and modification.

block1.txt, block2.txt, block3.txt are used for serverA, serverB, and serverC separately to store the transaction data.
