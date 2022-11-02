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
#include <fstream>
#include <sstream>

using namespace std;

#define PORT_NUMBER_of_SERVER_B 22128
#define IP_ADDRESS "127.0.0.1"
#define PORT_NUMBER_OF_MAIN_SERVER 24128
#define BUFFER_SIZE 100000 // buffer size for all buffer

void set_UDP_socket();
void bind_socket();
void receive_from_main_server();
void send_to_main_server(const char* message);
vector<string> split_c_string(char *input, const char *delimiter);
string find_record(string file_name, string target, int &size, bool find_all);
void write_record(string new_record ,string file_name);

// the buffer used to store message from main server
char buffer_for_main_server[BUFFER_SIZE];
struct sockaddr_in serverB_info;
struct sockaddr_in main_server_info;
int sock_descriptor_serverB;

int main(){
    string file_name = "block2.txt"; // serverB's data file 
    int number_of_records = 0; // number of records in the data file, initialize with 0;


    // bootup message
    cout << "The ServerB is up and running using UDP on port" << PORT_NUMBER_of_SERVER_B << "." << endl;

    //initialize the information of serverB
    memset(&serverB_info, 0, sizeof(serverB_info));
    serverB_info.sin_family = AF_INET; //set IPV4
    serverB_info.sin_port = htons(PORT_NUMBER_of_SERVER_B); //set port#
    serverB_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // serverB's IP address

    //initialize the information of mian server
    memset(&main_server_info, 0, sizeof(main_server_info));
    main_server_info.sin_family = AF_INET; //set IPV4
    main_server_info.sin_port = htons(PORT_NUMBER_OF_MAIN_SERVER); //set port#
    main_server_info.sin_addr.s_addr = inet_addr(IP_ADDRESS); // main server's IP address

    //create a UDP socket
    set_UDP_socket();

    //bind the socket with serverB's IP address and the port number of it
    bind_socket();

    while(true){
        //receive message from main server
        receive_from_main_server();

        cout << "The serverB received a request from the Main Server." << endl;

        //process the command
        vector<string> splitted_command = split_c_string(buffer_for_main_server, ";");
        int size_of_vector = splitted_command.size();
        if(size_of_vector == 1 && splitted_command[0] == "read_all"){
            //get all records
            string result = find_record(file_name, " ", number_of_records, true); //find_all is set true
            //transfer number_of_records to string
            stringstream int_to_string;
            string num_of_rec_str;
            int_to_string << number_of_records;
            int_to_string >> num_of_rec_str;
            result = num_of_rec_str + result;
            //send result to main server
            send_to_main_server(result.c_str());
            cout << "The serverB finished sending the response to the Main Server." << endl;
        }
        else if(size_of_vector == 2 && splitted_command[0] == "read"){
            //get records for the  user 
            //check balance
            string target = splitted_command[1];
            string result = find_record(file_name, target, number_of_records, false); //find_all is set false
            //transfer number_of_records to string
            stringstream int_to_string;
            string num_of_rec_str;
            int_to_string << number_of_records;
            int_to_string >> num_of_rec_str;
            result = num_of_rec_str + result;
            //send result to main server
            send_to_main_server(result.c_str());
            cout << "The serverB finished sending the response to the Main Server." << endl;
        }
        else if(size_of_vector == 2 && splitted_command[0] == "write"){
            string record = splitted_command[1];
            //write this record into the file
            write_record(record,file_name);
            //notice the main server that the new record has been written.
            string result = "write_complete";
            send_to_main_server(result.c_str());
            cout << "The serverB finished writing a new reord from the Main Server." << endl;
        }

        //reset the buffer
        memset(buffer_for_main_server, 0, sizeof(buffer_for_main_server));
    }
    return 0;
}


void set_UDP_socket(){
    sock_descriptor_serverB = socket(serverB_info.sin_family, SOCK_DGRAM, 0);
    if(sock_descriptor_serverB == -1){
        perror("clientB: socket");
        exit(1);
    }
}

void bind_socket(){
    if (bind(sock_descriptor_serverB, (struct sockaddr *) &serverB_info, sizeof(serverB_info)) == -1) {
        perror("ServerB: fail to bind the socket to port 21128");
        exit(1);
    }
}

void receive_from_main_server(){
    socklen_t main_server_info_size = sizeof(main_server_info);
    int bytes_recv = recvfrom(sock_descriptor_serverB, buffer_for_main_server, BUFFER_SIZE - 1, 0, (struct sockaddr *) &main_server_info, &main_server_info_size);
    if(bytes_recv == -1){
        perror("ServerB: fail to receive data from main server");
        close(sock_descriptor_serverB);
        exit(1);
    }
    buffer_for_main_server[bytes_recv] = '\0';
}

void send_to_main_server(const char* message){
    int len = strlen(message);
    socklen_t main_server_info_size = sizeof(main_server_info);
    int bytes_sent = sendto(sock_descriptor_serverB, message, len, 0, (struct sockaddr *) &main_server_info, main_server_info_size);
    if(bytes_sent == -1){
        perror("ServerB: fail to send data to main server");
        close(sock_descriptor_serverB);
        exit(1);
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

//find the record containing target from the txt file
//file_name is the name of data file we want to traverse
//target is the name of user whose recordings we want to get
//size  is an integer records the number of reocrds in the file.
//find_all is a flag, if we want to get all records, set find_all to true, else set find_all to false
string find_record(string file_name, string target, int &size, bool find_all){
    size = 0;
    string help_buffer = "";
    string result = "";
    ifstream file_reader (file_name.c_str());
    while(getline(file_reader,help_buffer)){
        if(help_buffer == ""){
            continue;
        }
        size ++;
        string temp_string = help_buffer;
        if(find_all){
            result = result + ";" + temp_string;
        }
        else{
            vector<string> splitted_string = split_c_string((char*)help_buffer.c_str(), " ");
            if((splitted_string[1] == target) || splitted_string[2] == target){ 
                result = result + ";" + temp_string;
            }
        }
    }
   file_reader.close();
   return result;
}

//write a new record into the file
//new_record is a string containing the new record
//file_name is the name of file you want to write into
void write_record(string new_record ,string file_name){
    ofstream file_weitor (file_name.c_str(), ios::app);
    if (file_weitor.is_open()){
        file_weitor << "\n";
        file_weitor << new_record << "\n";
        file_weitor.close();
    }
}
