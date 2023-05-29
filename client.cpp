#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <cstring>
#include <iostream>

// DEFINE THE SERVER NAME AN DEFAULT VALUES FOR THE MESSAGE QUEUE
#define SERVER_QUEUE_NAME   "/Brandonserver"
#define QUEUE_PERMISSIONS 0660  // like chmod values, user and owner can read and write to queue
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10   // leave some extra space after message

using namespace std;

int main (){
    mqd_t qd_server, qd_client;   // queue descriptors
    
    struct { // this is a struct for sending the client id and the temperature to the server
		int temperature;
		char client_id[64]; 
	} clientsmessage;

    struct { //this is a struct for taking in the centraltemp and the stability from the server
        int centraltemp = 60;
        bool stability = false;
    } retrievetemp;
    
    // create the client queue for receiving messages from server
	// use the client PID to help differentiate it from other queues with similar names
	// the queue name must be a null-terminated c-string.
	// strcpy makes that happen
//	string  str_client_queue_name = "/client-" + to_string(getpid ()) + "\\0'";
	string  str_client_queue_name = "/Brandonsclient-" + to_string(getpid());
	strcpy(clientsmessage.client_id, str_client_queue_name.c_str());                   // this stores the client name into the clientsmessage.client_id which was created in the struct for use later
    
	// Build message queue attribute structure passed to the mq open
    struct mq_attr attr;

		attr.mq_flags = 0;
		attr.mq_maxmsg = MAX_MESSAGES;
		attr.mq_msgsize = MAX_MSG_SIZE;
		attr.mq_curmsgs = 0;
    
	// Create and open client message queue
    if ((qd_client = mq_open (clientsmessage.client_id, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        cerr<< clientsmessage.client_id <<": mq_open (client)" ;
        exit (1);
    }
	
	// Open server message queue. The name is shared. It is opend write and read only
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        cerr<< clientsmessage.client_id << ": mq_open (server)";
        exit (1);
    }

    cout << clientsmessage.client_id << ": Enter initial client temperature: " << endl; //initial client prompt to ask for a temperature
    cin >> clientsmessage.temperature;                                                  //takes in the temperature as a user input and stores it inside the message
        // Send message to server
		//  Data sent is the client's message queue name
    cout << clientsmessage.client_id << ": Sending the client temperature: " << clientsmessage.temperature << " degrees to the server." << endl; 
    if (mq_send (qd_server, (char*)&clientsmessage , sizeof(clientsmessage), 0) == -1) {              //sends the message containing the temperature and the client name to the server
        cerr<< clientsmessage.client_id << ": Not able to send message to server";             //error message in case something goes wrong
    }

        // Receive response from server
		// Message received is the token
    if (mq_receive (qd_client, (char *)&retrievetemp, MSG_BUFFER_SIZE, NULL) == -1) {  //recieves central temp and stability status from the server
        cerr<< clientsmessage.client_id  << ": mq_receive";
        exit (1);
    }
        
    while(retrievetemp.stability != true){          //this loops until stability is achieved
        cout << clientsmessage.client_id << ": Central temperature received from server: " << retrievetemp.centraltemp << endl;  //prints out what was recieved from the server
        clientsmessage.temperature = ((clientsmessage.temperature * 3) + (2 * retrievetemp.centraltemp))/5;                             //calculation for the new temperature
        cout << clientsmessage.client_id << ": New client temperature: " << clientsmessage.temperature << " degrees." << endl; //prints out the new temperature being sent to the server
        cout << clientsmessage.client_id << ": Sending new client temperature back to server. " << endl;
        if (mq_send (qd_server, (char*)&clientsmessage, sizeof(clientsmessage), 0) == -1) {                         //sends the new temperature to the server as well as the client id
            cerr<< clientsmessage.client_id  << ": Not able to send message to server";
        }

        // Receive response from server
		// Message received is the token
       	if (mq_receive (qd_client, (char *)&retrievetemp, MSG_BUFFER_SIZE, NULL) == -1) {           //recieves the new central temp and stability status from the server
            cerr<< clientsmessage.client_id  << ": mq_receive";
            exit(1);
        	}
        }
    cout << clientsmessage.client_id << ": Stability has been Achieved. " << endl;
    cout << clientsmessage.client_id << ": Temperature of Stability is:  " << retrievetemp.centraltemp << " degrees." << endl;  //this prints out the temperature that stability was achieved at

	// Close this message queue
    if (mq_close (qd_client) == -1) {
        cerr<< clientsmessage.client_id << ": mq_close";
        exit(1);
    }

	// Unlink this message queue
    if (mq_unlink (clientsmessage.client_id ) == -1) {
        cerr<< clientsmessage.client_id  << ": mq_unlink";
        exit(1);
    }
    cout << clientsmessage.client_id  << ": Shutting Down. Goodbye! c:\n";
    exit(0);
}
