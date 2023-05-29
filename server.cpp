#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

int main () {
    mqd_t qd_server, qd_client;   // queue descriptors
    struct {				  // this is a struct for recieving the client id and the temperature from the clients
        int temperature;
        char client_id [64];
    } clientsmessage;
	// Build message queue attribute structure passed to the mq open
    struct mq_attr attr;

		attr.mq_flags = 0;
		attr.mq_maxmsg = MAX_MESSAGES;
		attr.mq_msgsize = MAX_MSG_SIZE;
		attr.mq_curmsgs = 0;

	// Declare (create) the buffers to hold message received and sent
	
	// Initialize the token to be given to client
    struct { 				////this is a struct for sending the centraltemp and the stability status to the clients
        int centraltemp = 60;
        bool stability = false;
    }   retrievetemp;

	//initial server prompt
	cout << "Server: Hello! Currently waiting for temperatures from clients!\n";
	
    if ((qd_server = mq_open (SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        cerr << "Server: mq_open (server)";
        exit (1);
    }
	
	// Loop forever waiting for a request for a token
    while (1) {
        int arr[4];											//initializes array for holding temperatures from the clients
        int arrayLength = sizeof(arr) / sizeof(arr[0]) - 1;	//gets the array size for the for loops stop condition
        string client_queue[4];								//initializes array for holding the clients name information
        int sum = 0;										//sum variable to hold sum of temperatures

		
		// RECEIVE THE MESSAGE FROM SERVER MAILBOX
        // get the oldest message with highest priority
		// Note: this synchronous receive will wait until a message is in the queue
        for (int i = 0; i <= arrayLength; i++){									//this for loop is to get the server to wait for each of the clients to send a message before continuing
            if (mq_receive (qd_server, (char*)&clientsmessage,MSG_BUFFER_SIZE , NULL) == -1) {		//this recieves the temperature and client name from the client
                cerr << "Server: mq_receive";
                exit (1);
            }
            cout << "Server: Temperature: " << clientsmessage.temperature << " degrees from client: " << clientsmessage.client_id << endl; //prints out the temperature recieved as well as which client it was recieved from
            sum += clientsmessage.temperature;														//adds this temperature to the sum
            arr[i] = clientsmessage.temperature;												//places the temperature recieved into the array
            client_queue[i] = string(clientsmessage.client_id);									//places the clients id in the client queue array
        }
        
        if((arr[0] == arr[1]) && (arr[1] == arr[2]) && (arr[2] == arr[3])){							//checks if all the temperatures in the array are equal
			retrievetemp.stability = true; 															//sets the stability to true if all temperatures are true
			for (int clients = 0; clients <= arrayLength; clients++){								//this is a for loop for sending the clients their last central temp update as well as the stability is true status
				if ((qd_client = mq_open(client_queue[clients].c_str(), O_WRONLY)) == -1){			//opens the message queue for each individual client
					cerr << "Server: Not able to open client queue";
					continue;
				}
				if (mq_send (qd_client, (char*)&retrievetemp, sizeof(retrievetemp), 0) == -1) {		//sends the last central temp and stability status to each individual client
           			cerr << "Server: Not able to send message to client";
            		continue;
				}
			}
            cout << "Server: All temperatures are the same. Stability Achieved at " << clientsmessage.temperature << " degrees!" << endl;		//prints out what temp stability was achieved
            cout << "Server: Terminating clients c:" << endl;																				
		}
		else{																				//else loop for when the temperatures are not the same
			retrievetemp.centraltemp = (2 * retrievetemp.centraltemp + sum)/6;				//formula for creating a new central temperature
			cout << "Server: New central Temp: " << retrievetemp.centraltemp << " degrees." << endl;				//prints out the new centraltemp
			cout << "Server: Sending new central temperature to the client!" << endl;
			
			for (int clients = 0; clients <= arrayLength; clients++){						//this is a for loop for sending the clients their new central temp for their calculations
				if ((qd_client = mq_open(client_queue[clients].c_str(), O_WRONLY)) == -1){	//opens the message queue for each individual client
					cerr << "Server: Not able to open client queue";
					continue;
				}
				if (mq_send (qd_client, (char*)&retrievetemp, sizeof(retrievetemp), 0) == -1) {		//sends the new central temp and stability status to each individual client
           			cerr << "Server: Not able to send message to client";
            			continue;
				}
                sum = 0;					//resets sum back to 0
			}
		
	    }
	}
    mq_close (qd_server);					//Closes the server
    mq_unlink (SERVER_QUEUE_NAME);			//unlinks the servers message queue
    exit(0);
}
