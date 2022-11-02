#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

#define TCP_PORT_NUM_FOR_CLIENT_A  25128
#define TCP_PORT_NUM_FOR_CLIENT_B  26128
#define UDP_PORT_NUM_of_SERVER_A 21128
#define UDP_PORT_NUM_of_SERVER_B 22128
#define UDP_PORT_NUM_of_SERVER_C 23128
#define UDP_PORT_NUM_OF_MAIN_SERVER 24128
#define IP_ADDRESS "127.0.0.1"
#define BACKLOG 10 // maximum number of incoming connections
#define BUFFER_SIZE 100000 // size of all buffer
#define INITIAL_BALANCE 1000 // intitial balance for every user

void initialization_clientA();
void initialization_clientB();
void initialization_main_server();
void initialization_serverA();
void initialization_serverB();
void initialization_serverC();
void set_TCP_socket_for_client(const sockaddr_in &client_info, int &descriptor);
void bind_TCP_socket_for_client(sockaddr_in &client_info,const int descriptor);
void listen_to_client(const int descriptor);
void accept_client(sockaddr_in &client_info, int const descriptor,int &child_socket_descriptor);
void send_to_client(const char* message, const int child_socket_descriptor);
void reveice_from_client(char client, const int buffer_size, const int child_socket_descriptor);

void set_UDP_socket_for_back_server(int &descriptor);
void bind_UDP_socket_for_back_server(const int descriptor);
void send_UDP_to_back_server(const char* message, const int sock_descriptor, sockaddr_in back_server_info);
void receive_UDP_from_back_server(sockaddr_in back_server_info, const int buffer_size, const int descriptor);

void process_for_client(string client_name);
string get_result_from_backend_server(string command, sockaddr_in backend_server, string server_name);
int get_balance(string user, vector<string> records);
vector<string> split_c_string(char *input, const char *delimiter);
bool compare_to (string input_1, string input_2);
void write_record(string new_record ,string file_name);

int tcpsock_descriptor_for_A;
int child_tcpsock_descriptor_for_A;
int tcpsock_descriptor_for_B;
int child_tcpsock_descriptor_for_B;
int udpsock_descriptor;

struct sockaddr_in main_server_info;
struct sockaddr_in clientA_info;
struct sockaddr_in clientB_info;
struct sockaddr_in serverA_info;
struct sockaddr_in serverB_info;
struct sockaddr_in serverC_info;

char recv_buffer_for_clientA[BUFFER_SIZE];
char recv_buffer_for_clientB[BUFFER_SIZE];
char recv_buffer_for_back_server[BUFFER_SIZE];

int transfer_number;



int main(){
    //used to store transfer number, initialized with 0
    transfer_number = 0;

    // bootup message
    cout << "The main server is up and running." << endl;
    
    //setup TCP for clientA
    initialization_clientA();//initialize the information of clientA
    set_TCP_socket_for_client(clientA_info, tcpsock_descriptor_for_A);//create a TCP socket for clientA
    bind_TCP_socket_for_client(clientA_info, tcpsock_descriptor_for_A);//bind the socket with server's IP address and the port number for cilentA

    //setup for clientB
    initialization_clientB();//initialize the information of clientB
    set_TCP_socket_for_client(clientB_info, tcpsock_descriptor_for_B);//create a TCP socket for clientB
    bind_TCP_socket_for_client(clientB_info, tcpsock_descriptor_for_B);//bind the socket with server's IP address and the port number for cilentA

    //setup UDP for backend server
    initialization_main_server();
    initialization_serverA();
    initialization_serverB();
    initialization_serverC();
    set_UDP_socket_for_back_server(udpsock_descriptor);
    bind_UDP_socket_for_back_server(udpsock_descriptor);

    //listen clientA and clientB
    listen_to_client(tcpsock_descriptor_for_A);//listen to clientA
    listen_to_client(tcpsock_descriptor_for_B);//listen to clientB

    while(true){
        //accept connection from clientA
        accept_client(clientA_info,tcpsock_descriptor_for_A, child_tcpsock_descriptor_for_A);
        //reveive message from clientA
        reveice_from_client('A', BUFFER_SIZE, child_tcpsock_descriptor_for_A);
        // //process request from clientA
        process_for_client("clientA");
        close(child_tcpsock_descriptor_for_A);


        //accept connection from clientB
        accept_client(clientB_info,tcpsock_descriptor_for_B, child_tcpsock_descriptor_for_B);
        //reveive message from clientB
        reveice_from_client('B', BUFFER_SIZE, child_tcpsock_descriptor_for_B);
        // //process request from clientB
        process_for_client("clientB");
        close(child_tcpsock_descriptor_for_B);

        //reset all buffers
        memset(recv_buffer_for_clientA, 0, sizeof(recv_buffer_for_clientA));
        memset(recv_buffer_for_clientB, 0, sizeof(recv_buffer_for_clientB));
        memset(recv_buffer_for_back_server, 0, sizeof(recv_buffer_for_back_server));
    }
    close(tcpsock_descriptor_for_A);
    close(tcpsock_descriptor_for_B);
    return 0;
}


//initialize the information of clientA
void initialization_clientA(){
    memset(&clientA_info, 0, sizeof(clientA_info));
    clientA_info.sin_family = AF_INET; //set IPV4
    clientA_info.sin_port = htons(TCP_PORT_NUM_FOR_CLIENT_A); //set port#
    clientA_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // set clientA's IP address
}

//initialize the information of clientB
void initialization_clientB(){
    memset(&clientB_info, 0, sizeof(clientB_info));
    clientB_info.sin_family = AF_INET; //set IPV4
    clientB_info.sin_port = htons(TCP_PORT_NUM_FOR_CLIENT_B); //set port#
    clientB_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // set clientA's IP address
}

//initialize the information of mian server for UDP
void initialization_main_server(){
    memset(&main_server_info, 0, sizeof(main_server_info));
    main_server_info.sin_family = AF_INET; //set IPV4
    main_server_info.sin_port = htons(UDP_PORT_NUM_OF_MAIN_SERVER); //set port#
    main_server_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // mian server's IP address
}

//initialize the information of serverA
void initialization_serverA(){
    memset(&serverA_info, 0, sizeof(serverA_info));
    serverA_info.sin_family = AF_INET; //set IPV4
    serverA_info.sin_port = htons(UDP_PORT_NUM_of_SERVER_A); //set port#
    serverA_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // serverA's IP address
}

//initialize the information of serverB
void initialization_serverB(){
    memset(&serverB_info, 0, sizeof(serverB_info));
    serverB_info.sin_family = AF_INET; //set IPV4
    serverB_info.sin_port = htons(UDP_PORT_NUM_of_SERVER_B); //set port#
    serverB_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // serverB's IP address
}

//initialize the information of serverC
void initialization_serverC(){
    memset(&serverC_info, 0, sizeof(serverC_info));
    serverC_info.sin_family = AF_INET; //set IPV4
    serverC_info.sin_port = htons(UDP_PORT_NUM_of_SERVER_C); //set port#
    serverC_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // serverC's IP address
}


//create UDP socket for back server
//descriptor is an int used to store the socket descriptor.
void set_UDP_socket_for_back_server(int &descriptor){
    descriptor = socket(main_server_info.sin_family, SOCK_DGRAM, 0);
    if(descriptor == -1){
        perror("main server: UDP socket");
        exit(1);
    }
}

//bind the socket with mian server's IP address and the port number for back server
//descriptor the socket descriptor of the socket you want to bind.
void bind_UDP_socket_for_back_server(const int descriptor){
    if (bind(descriptor, (struct sockaddr *) &main_server_info, sizeof(main_server_info)) == -1) {
        perror("main server: fail to bind UDP socket");
        exit(1);
    }
}

//send UDP message to back server
//message is the information you want to send
//sock_descriptor is the socket descriptor of the socket used to send message
//back_server_info is the information of the back server you want to send to
void send_UDP_to_back_server(const char* message, const int sock_descriptor, sockaddr_in back_server_info){
    int len = strlen(message);
    socklen_t back_server_info_size = sizeof(back_server_info);
    int bytes_sent = sendto(sock_descriptor, message, len, 0, (struct sockaddr *) &back_server_info, back_server_info_size);
    if(bytes_sent == -1){
        perror("serverM: fail to send data to back server");
        exit(1);
    }
} 

//receive message from back server
//back_server_info is the information of the back server you want to receive from
//buffer_size is the size of buffer
//sock_descriptor is the socket descriptor of the socket used to receive message
void receive_UDP_from_back_server(sockaddr_in back_server_info, const int buffer_size, const int descriptor){
    socklen_t back_server_info_size = sizeof(back_server_info);
    int bytes_recv = recvfrom(descriptor, recv_buffer_for_back_server, buffer_size - 1, 0, (struct sockaddr *) &back_server_info, &back_server_info_size);
    if(bytes_recv == -1){
        perror("ServerA: fail to receive data from main server");
        exit(1);
    }
    recv_buffer_for_back_server[bytes_recv] = '\0';
}

//create a TCP socket for client
//I got this part of code from Beej's guide
//client_info is the informaion of client you want to create socket for.
//descriptor is an int used to store the socket descriptor.
void set_TCP_socket_for_client(const sockaddr_in &client_info, int &descriptor){
    descriptor = socket(client_info.sin_family, SOCK_STREAM, 0);
    if(descriptor == -1){
        perror("serverM: socket for clientA");
        exit(1);
    }
}

//bind the socket with server's IP address and the port number for cilent
//client_info is the informaion of client you want to bind socket for.
//descriptor the socket descriptor of the socket you want to bind.
void bind_TCP_socket_for_client(sockaddr_in &client_info,const int descriptor){
    if(bind(descriptor,(struct sockaddr *) &client_info, sizeof(clientA_info)) == -1 ){
        close(descriptor);
        perror("serverM: bind for clientA");
        exit(1);
    } 
}


//listen to client
//descriptor the socket descriptor of the socket you use to bind.
void listen_to_client(const int descriptor){
    if((listen(descriptor, BACKLOG)) == -1){
        close(descriptor);
        perror("serverM: listening to clientA faild");
        exit(1);
    }
}

//accept connection from client
void accept_client(sockaddr_in &client_info, int const descriptor,int &child_socket_descriptor){
    socklen_t client_address_size = sizeof(client_info);
    child_socket_descriptor = accept(descriptor,(struct sockaddr *) &client_info, &client_address_size);
    if(child_socket_descriptor == -1){
        perror("serverM: fail to accept clientA");
        close(child_socket_descriptor);
        exit(0);
    }
}

//send message to clinet
//message is the information you want to send
//child_socket_descriptor is the socket descriptor of the socket used to send message
void send_to_client(const char* message, const int child_socket_descriptor){
    int len = strlen(message);
    int bytes_sent = send(child_socket_descriptor, message, len, 0);
    if(bytes_sent == -1){
        perror("send");
        close(child_socket_descriptor);
        exit(0);
    }
}

//receive message from client
//buffer_size is the size of buffer
//child_socket_descriptor is the socket descriptor of the socket used to reveice message
//client is the name of client that you want to reveice from
void reveice_from_client(char client, const int buffer_size, const int child_socket_descriptor){
    if(client == 'A'){
    int bytes_recv = recv(child_socket_descriptor, recv_buffer_for_clientA, buffer_size - 1, 0);
    recv_buffer_for_clientA[bytes_recv] = '\0';
    }
    else if (client == 'B'){
    int bytes_recv = recv(child_socket_descriptor, recv_buffer_for_clientB, buffer_size - 1, 0);
    recv_buffer_for_clientB[bytes_recv] = '\0';
    }
}

//process the request for clients
//client_name is the name of the client we want to precess request for
void process_for_client(string client_name){
//process request from client
    vector<string> splitted_result;
    const char *delimiter = " ";
    int child_tcpsock_descriptor;
    int port_number;
    if(client_name == "clientA"){
        splitted_result = split_c_string(recv_buffer_for_clientA, delimiter);
        memset(recv_buffer_for_clientA, 0, sizeof(recv_buffer_for_clientA));
        child_tcpsock_descriptor = child_tcpsock_descriptor_for_A;
        port_number = TCP_PORT_NUM_FOR_CLIENT_A;
    }
    else if(client_name == "clientB"){
        splitted_result = split_c_string(recv_buffer_for_clientB, delimiter);
        memset(recv_buffer_for_clientB, 0, sizeof(recv_buffer_for_clientB));
        child_tcpsock_descriptor = child_tcpsock_descriptor_for_B;
        port_number = TCP_PORT_NUM_FOR_CLIENT_B;
    }

    int splitted_result_size = splitted_result.size();

    //client request TXLIST
    if((splitted_result_size == 1) && (splitted_result[0] == "TXLIST")){
        cout << "A TXLIST request has been received from " << client_name << " using TCP over port " << port_number <<"." << endl;
        //request for all transfer records
        string message_to_server = "read_all";

        //get records from back end server;
        string result_from_serverA = get_result_from_backend_server(message_to_server,serverA_info, "serverA");
        cout << "The main server received transaction list from ServerA using UDP" << " over port " << UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl; 
        string result_from_serverB = get_result_from_backend_server(message_to_server,serverB_info, "serverB");
        cout << "The main server received transaction list from ServerB using UDP" << " over port " << UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl; 
        string result_from_serverC = get_result_from_backend_server(message_to_server,serverC_info, "serverC");
        cout << "The main server received transaction list from ServerC using UDP" << " over port " << UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl; 

        vector<string> splitted_result_serverA = split_c_string((char*)result_from_serverA.c_str(), ";");
        vector<string> splitted_result_serverB = split_c_string((char*)result_from_serverB.c_str(), ";");
        vector<string> splitted_result_serverC = split_c_string((char*)result_from_serverC.c_str(), ";");

        //update transfer number
        transfer_number = atoi(splitted_result_serverA[0].c_str()) + atoi(splitted_result_serverB[0].c_str()) + atoi(splitted_result_serverC[0].c_str());

        //combile all records in one vector
        vector<string> all_records;
        for(int i = 1; i < splitted_result_serverA.size(); i++){
            all_records.push_back(splitted_result_serverA[i]);
        }
        for(int i = 1; i < splitted_result_serverB.size(); i++){
            all_records.push_back(splitted_result_serverB[i]);
        }
        for(int i = 1; i < splitted_result_serverC.size(); i++){
            all_records.push_back(splitted_result_serverC[i]);
        }
        //sort the records in increasing order based on transfer number 
        sort(all_records.begin(), all_records.end(), compare_to);

        //send message back to client
        remove("alichain.txt");
        for (int i = 0; i < all_records.size(); i++){
            write_record(all_records[i] ,"alichain.txt");
        }
        cout << "The sorted file is up and ready." << endl;
        string message_to_client = "The sorted file is up and ready.";
        send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
    }
    //client request for balance
    else if((splitted_result_size == 1) && (splitted_result[0] != "TXLIST")){
        cout << "The main server received input=" << splitted_result[0] << " from the client using TCP over port " << port_number <<"." << endl;

        //request the user's records
        string command = "read";
        string delimiter = ";";
        string message_to_server = command + delimiter + splitted_result[0];
        string user_name = splitted_result[0];
        string result_from_serverA = get_result_from_backend_server(message_to_server,serverA_info, "serverA");
        cout << "The main server received transactions from ServerA using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result_from_serverB = get_result_from_backend_server(message_to_server,serverB_info, "serverB");
        cout << "The main server received transactions from ServerB using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result_from_serverC = get_result_from_backend_server(message_to_server,serverC_info, "serverC");
        cout << "The main server received transactions from ServerC using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;

        vector<string> splitted_result_serverA = split_c_string((char*)result_from_serverA.c_str(), ";");
        vector<string> splitted_result_serverB = split_c_string((char*)result_from_serverB.c_str(), ";");
        vector<string> splitted_result_serverC = split_c_string((char*)result_from_serverC.c_str(), ";");

        //update transfer number
        transfer_number = atoi(splitted_result_serverA[0].c_str()) + atoi(splitted_result_serverB[0].c_str()) + atoi(splitted_result_serverC[0].c_str());
        string message_to_client = "";
        if ((splitted_result_serverA.size() == 1) && (splitted_result_serverB.size() == 1) && (splitted_result_serverC.size() == 1)){
            message_to_client = "no_exist";
            send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
            cout << user_name << " was not found on database." << endl;
        }
        else{
            int balance = INITIAL_BALANCE + get_balance(user_name, splitted_result_serverA) + get_balance(user_name, splitted_result_serverB) + get_balance(user_name, splitted_result_serverC);
            stringstream int_to_string;
            string num_of_rec_str;
            int_to_string << balance;
            int_to_string >> message_to_client;
            send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
            cout << "The main server sent the current balance to " << client_name << "." << endl;
        }
    }
    //client request for stats
    //not implemented yet
    else if((splitted_result_size == 2) && (splitted_result[1] == "stats")){
        string message_to_client = "This function has not implemented yet.";
        send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
    }
    //client request for transfer
    else if((splitted_result_size == 3)){
        cout << "The main server received from " << splitted_result[0] << " to transfer " << splitted_result[2] << " coins to " << splitted_result[1] << " using TCP over port " << port_number << "." << endl;

        //request user's records
        //need to check if the two people are there
        string command = "read";
        string delimiter = ";";
        string message1_to_server = command + delimiter + splitted_result[0];
        string message2_to_server = command + delimiter + splitted_result[1];
        
        // check sender's balance
        string result1_from_serverA = get_result_from_backend_server(message1_to_server,serverA_info, "serverA");
        cout << "The main server received transactions from ServerA using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result1_from_serverB = get_result_from_backend_server(message1_to_server,serverB_info, "serverB");
        cout << "The main server received transactions from ServerB using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result1_from_serverC = get_result_from_backend_server(message1_to_server,serverC_info, "serverC");
        cout << "The main server received transactions from ServerC using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;

        vector<string> sender_splitted_result_serverA = split_c_string((char*)result1_from_serverA.c_str(), ";");
        vector<string> sender_splitted_result_serverB = split_c_string((char*)result1_from_serverB.c_str(), ";");
        vector<string> sender_splitted_result_serverC = split_c_string((char*)result1_from_serverC.c_str(), ";");

        //check receiver's balance
        string result2_from_serverA = get_result_from_backend_server(message2_to_server,serverA_info, "serverA");
        cout << "The main server received transactions from ServerA using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result2_from_serverB = get_result_from_backend_server(message2_to_server,serverB_info, "serverB");
        cout << "The main server received transactions from ServerB using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
        string result2_from_serverC = get_result_from_backend_server(message2_to_server,serverC_info, "serverC");
        cout << "The main server received transactions from ServerC using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;

        vector<string> receiver_splitted_result_serverA = split_c_string((char*)result2_from_serverA.c_str(), ";");
        vector<string> receiver_splitted_result_serverB = split_c_string((char*)result2_from_serverB.c_str(), ";");
        vector<string> receiver_splitted_result_serverC = split_c_string((char*)result2_from_serverC.c_str(), ";");

        //update transfer number
        transfer_number = atoi(receiver_splitted_result_serverA[0].c_str()) + atoi(receiver_splitted_result_serverB[0].c_str()) + atoi(receiver_splitted_result_serverC[0].c_str());
        string message_to_client = "";

        // both sender and receiver are not in the network
        if ((sender_splitted_result_serverA.size() == 1) && (sender_splitted_result_serverB.size() == 1) && (sender_splitted_result_serverC.size() == 1) &&
            (receiver_splitted_result_serverA.size() == 1) && (receiver_splitted_result_serverB.size() == 1) && (receiver_splitted_result_serverC.size() == 1)){
            message_to_client = "not_there" + delimiter + splitted_result[0] + delimiter + splitted_result[1];
            send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
        }
        //only sender is not in the network
        else if ((sender_splitted_result_serverA.size() == 1) && (sender_splitted_result_serverB.size() == 1) && (sender_splitted_result_serverC.size() == 1) &&
            ((receiver_splitted_result_serverA.size() > 1) || (receiver_splitted_result_serverB.size() > 1) || (receiver_splitted_result_serverC.size() > 1))){
            message_to_client = "not_there" + delimiter + splitted_result[0];
            send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
        }
        //only receiver is not in the network
        else if (((sender_splitted_result_serverA.size() > 1) || (sender_splitted_result_serverB.size() > 1) || (sender_splitted_result_serverC.size() > 1)) &&
            (receiver_splitted_result_serverA.size() == 1) && (receiver_splitted_result_serverB.size() == 1) && (receiver_splitted_result_serverC.size() == 1)){
            message_to_client = "not_there" + delimiter + splitted_result[1];
            send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
        }
        // both sender and receiver are in the network
        else{
            //check sender's balance
            int sender_balance = INITIAL_BALANCE + get_balance(splitted_result[0], sender_splitted_result_serverA) + get_balance(splitted_result[0], sender_splitted_result_serverB) + get_balance(splitted_result[0], sender_splitted_result_serverC);
            if(sender_balance < atoi(splitted_result[2].c_str())){
                stringstream int_to_string;
                string balance_str;
                int_to_string << sender_balance;
                int_to_string >> balance_str;

                message_to_client = "not_enough" + delimiter + balance_str;
                send_to_client(message_to_client.c_str(), child_tcpsock_descriptor);
                cout << "The main server sent the current balance to " << client_name << "." << endl;
            }
            else{
                int new_balance = sender_balance - atoi(splitted_result[2].c_str());
                string new_balance_str = "";
                stringstream int_to_string_1;
                int_to_string_1 << new_balance;
                int_to_string_1 >> new_balance_str;

                transfer_number ++;
                int new_transfer_number =  transfer_number;
                string new_transfer_number_str;
                stringstream int_to_string_2;
                int_to_string_2 << new_transfer_number;
                int_to_string_2 >> new_transfer_number_str;


                //request one of the three backend server to write new record
                string delimiter1 = ";";
                string delimiter2 = " ";
                string command_to_back_server = "write" + delimiter1 + new_transfer_number_str + delimiter2 + splitted_result[0] + delimiter2 + splitted_result[1] + delimiter2 + splitted_result[2];
                //generate a random number from 1 to 3 inclusively
                int pick_back_server = rand() % 3 + 1;
                struct sockaddr_in back_server;
                string server_name;
                if(pick_back_server == 1){
                    back_server = serverA_info;
                    server_name = "serverA";
                }
                else if(pick_back_server == 2){
                    back_server = serverB_info;
                    server_name = "serverB";
                }
                else if(pick_back_server == 3){
                    back_server = serverC_info;
                    server_name = "serverC";
                }
                string result_from_back_server = get_result_from_backend_server(command_to_back_server,back_server, server_name);
                if(result_from_back_server == "write_complete"){
                    cout << "The main server received the feedback from " << server_name << " using UDP over port " <<  UDP_PORT_NUM_OF_MAIN_SERVER << "." << endl;
                    cout << "The main server sent the current balance to " << client_name << "." << endl;
                    send_to_client(new_balance_str.c_str(), child_tcpsock_descriptor);
                }

            }
        }
        cout << "The main server sent the result of transaction to " << client_name << "." << endl;  
    }

}

//used to split the c_string
//input is the c_string we want to split
//delimiter is the delimeter used to split the string 
//retuen a vector which have all strings
//part of code is from https://blog.csdn.net/Mary19920410/article/details/77372828
vector<string> split_c_string(char *input, const char *delimiter){
    vector<string> output;
    char *p = strtok(input, delimiter);
	while (p){
        string s = p;
        output.push_back(s);
        p = strtok(NULL, delimiter);
    }
    return output;
}

// get the user's income from the three records
// records is the transfer history
// result is the user's income get from the records
int get_balance(string user, vector<string> records){
    int result = 0;
    if(records.size() > 1){
        for(int i = 1; i < records.size(); i++){
            vector<string> splitted_record = split_c_string((char*)records[i].c_str(), " ");
            if(splitted_record[1] == user){
                result = result - atoi(splitted_record[3].c_str());
            }
            else if(splitted_record[2] == user){
                result = result + atoi(splitted_record[3].c_str());
            }
        }
    }
    return result;
}

//send request and get result from backend server
//command is the command you send to server
//backend_server is the server you want to request
//server_name is the name of server
string get_result_from_backend_server(string command, sockaddr_in backend_server, string server_name){
    cout << "The main server sent a request to " << server_name << endl;
    send_UDP_to_back_server(command.c_str(), udpsock_descriptor, backend_server);
    receive_UDP_from_back_server(backend_server, BUFFER_SIZE, udpsock_descriptor);
    string result = recv_buffer_for_back_server;
    memset(recv_buffer_for_back_server, 0, sizeof(recv_buffer_for_back_server));
    return result;
}

//a function used as a parameter in sort()
//resturn true if the trasnfer number of input1 is less than that of input2
bool compare_to (string input_1, string input_2){
    vector<string> splitted_input1 = split_c_string((char*) input_1.c_str(), " ");
    vector<string> splitted_input2 = split_c_string((char*) input_2.c_str(), " ");
    return (atoi(splitted_input1[0].c_str()) < atoi(splitted_input2[0].c_str())); 
}  

//write a new record into the file
//new_record is a string containing the new record
//file_name is the name of file you want to write into
void write_record(string new_record ,string file_name){
    ofstream file_weitor (file_name.c_str(), ios::app);
    if (file_weitor.is_open()){
        file_weitor << new_record << "\n";
        file_weitor.close();
    }
}