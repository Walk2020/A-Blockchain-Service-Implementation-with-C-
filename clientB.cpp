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
#include <cstring>
#include <vector>


using namespace std;


#define TCP_MAIN_SERVER_PORT_NUM 26128 //port number of main server
#define MAIN_SERVER_IP "127.0.0.1" // the IP is local host


// the buffer used to store result
const int result_buffer_size = 100000;
char result_buffer[result_buffer_size];

void send_to_mianserver(const char* message, const int socket_descriptor);
void reveice_from_mainserver(const int socket_descriptor);
vector<string> split_c_string(char *input, const char *delimiter);


int main(int argc, char *argv[]){
    int sock_descriptor;
    struct sockaddr_in server_info;

    //bootup message
    cout << "The client B is up and running." << endl;

    //initialize the information of main server
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET; //set IPV4
    server_info.sin_port = htons(TCP_MAIN_SERVER_PORT_NUM); //set port#
    server_info.sin_addr.s_addr = inet_addr(MAIN_SERVER_IP); // set main server's IP address

    //create a TCP socket of clientB
    //I got this part of code from Beej's guide
    sock_descriptor = socket(server_info.sin_family, SOCK_STREAM, 0);
    if(sock_descriptor == -1){
        perror("clientB: fali to create a socket");
        exit(1);
    }

    //make connection with main server
    //I got the code of detecting error from Beej's guide
    if(connect(sock_descriptor, (struct sockaddr*) &server_info, sizeof(server_info)) == -1){
        close(sock_descriptor);
        perror("clientB: fali to connect");
        exit(1);
    }
    
    //send command to main server and receive the result
    if((argc == 2) && (strcmp(argv[1], "TXLIST") == 0)){
        //get transfer list
        cout << "ClinetB sent a sorted list request to the main server." << endl;

        //send command
        send_to_mianserver(argv[1],sock_descriptor);

        //receive result and process it.
        reveice_from_mainserver(sock_descriptor);
        string result = result_buffer;
        cout << result << endl;
    }
    else if((argc == 2) && (strcmp(argv[1], "TXLIST") != 0)){
        //check wallet
        string username = argv[1];
        cout << username << " send a balance enquiry request to the main server." << endl;

        //send command
        send_to_mianserver(argv[1],sock_descriptor);

        //receive result from main server
        reveice_from_mainserver(sock_descriptor);
        string result = result_buffer;
        if(result == "no_exist"){
            cout << "Unable to proceed with the balance request as "<< username << " is not part of the network." << endl;
        }
        else{
            cout << "The current balance of " << username << " is: " << result <<" alicoins." << endl;
        }
    }
    else if(argc == 4){
        //transfer coins
        string sender = argv[1];
        string receiver = argv[2];
        string transfer_amount = argv[3];
        string command = sender + " " + receiver + " " + transfer_amount;
        cout << sender << " has requested to transfer " << transfer_amount << " coins to " << receiver << "." << endl;
        send_to_mianserver(command.c_str(),sock_descriptor);

        //receive result from main server
        //result will be sender's name if sender is not there, receiver's name if receiver is not there. In other cases, result will be sender's current balance
        reveice_from_mainserver(sock_descriptor);

        //split the result using ' ' as the delimiter.
        const char *delimiter = ";";
        vector<string> splitted_result = split_c_string(result_buffer, delimiter);
        int splitted_result_size = splitted_result.size();
        
        //if succeed, receive the balance before transfer
        if(splitted_result_size == 1){
            cout << sender << " successfully transferred " << transfer_amount << " alicoins to " << receiver << "." << endl;
            cout << "The current balance of " << sender << " is: " << splitted_result[0] << " alicoins." << endl;
        }
        //if receiver or sender is not there, receive "not_there " + name"
        else if((splitted_result_size == 2) && (splitted_result[0] == "not_there")){
            cout << "Unable to proceed with the transaction as " << splitted_result[1] << " is not part of the network." << endl;
        }
        //else if both the sender and receiver are not there, receive "not_there " + "name1 "  + "name2"
        else if((splitted_result_size == 3) && (splitted_result[0] == "not_there")){
            cout << "Unable to proceed with the transaction as " << splitted_result[1] << " and " << splitted_result[2] << " are not part of the network." << endl;
        }
        //else if sender does not have enough coins, receive "not_enought" + balance
        else if((splitted_result_size == 2) && (splitted_result[0] == "not_enough")){
            cout << sender << " was unable to transfer " << transfer_amount << " alicoins to " << receiver << " because of insufficient." << endl;
            cout << "The current balance of " << sender << " is: " << splitted_result[1] << " alicoins." << endl;
        }
    }   
    else if(argc == 3 && (strcmp(argv[2], "stats") == 0)){
        //get statistical result
        //thsi function is not implemented yes
        cout << argv[1] << " sent a statistics enquiry request to the main server." << endl;
        string username = argv[1];
        string stats = argv[2];
        string command = username + " " + stats;
        send_to_mianserver(command.c_str(),sock_descriptor);

        //receive result and process it.
        reveice_from_mainserver(sock_descriptor);
        string result = result_buffer;
        cout << argv[1] << " statistics are the following: " << endl;
        cout << result << endl;
    }
    else{
        cout << "Wrong command!" << endl;
    }

    close(sock_descriptor);
    return 0;
}

//send message to mainserver
//message is the information you want to send
//child_socket_descriptor is the socket descriptor of the socket used to send message
void send_to_mianserver(const char* message, const int socket_descriptor){
    int len = strlen(message);
    int bytes_sent = send(socket_descriptor, message, len, 0);
    if(bytes_sent == -1){
        perror("send");
        close(socket_descriptor);
        exit(0);
    }
}

//receive message from mainserver
//buffer_size is the size of buffer
//child_socket_descriptor is the socket descriptor of the socket used to reveice message
void reveice_from_mainserver(const int socket_descriptor){
    int bytes_recv = recv(socket_descriptor, result_buffer, result_buffer_size - 1, 0);
    result_buffer[bytes_recv] = '\0';
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

