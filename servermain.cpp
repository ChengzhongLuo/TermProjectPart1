/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <sstream>

#define PORT "23108"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 100 // max number of bytes we can get at once

using namespace std;

//code from beej's guide
void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6: //code from beej's guide
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//a map that stores department to the backend server ID
map<string, int> departmentMap;

//read the list.txt file and load it
void loadFile(){
    //Open the file
    ifstream file("list.txt");
    if (!file.is_open()) {
        cerr << "Unable to open list.txt" << endl;
        exit(1);
    }

    string line;
    int backendID;
    while (getline(file, line)) { //loop when there is line in file
        // Convert the line to backendID
        backendID = stoi(line);

        // Get the departments line
        getline(file, line);

        // Split the departments by ';'
        stringstream ss(line);
        string department;
        while (getline(ss, department, ';')) { //loop when there is string in stringstream
            departmentMap[department] = backendID; //map the department name to backendID
        }
    }
    file.close();//close file
}

int main(void)
{
    //code from beej's guide
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

    //code from beej's guide
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo("127.0.0.1", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can//code from beej's guide
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

    cout << "Main server is up and running." << endl;

    loadFile(); // load the file

    cout << "Main server has read the department list from list.txt." << endl;

    // Count distinct departments for each backend server ID
    map<int, set<string> > distinctDepartments;
    for (const auto& pair : departmentMap) {
        distinctDepartments[pair.second].insert(pair.first);
    }

    cout << "Total number of Backend Servers: " << distinctDepartments.size() << endl;
    for (const auto& pair : distinctDepartments) {
        cout << "Backend Servers " << pair.first << " contains " << pair.second.size() << " distinct departments" << endl;
    }

    int clientId = 0;

    while(1) {  // main accept() loop, always alive
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        clientId++; // client ID increase 1 for every new client
        string clientPort;
        // Retrieve and print the client's dynamically assigned port
        if (their_addr.ss_family == AF_INET) {
            clientPort = to_string(ntohs(((struct sockaddr_in*)&their_addr)->sin_port));
        } else { // AF_INET6 for IPv6
            clientPort = to_string(ntohs(((struct sockaddr_in6*)&their_addr)->sin6_port));
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener

            char buffer[1024];
            int bytes_received;
            while ((bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[bytes_received] = '\0';  // Null-terminate received string

                cout << "Main server has received the request on Department " << buffer
                     << " from client" << clientId
                     << " using TCP over port " << PORT << endl;

                if (departmentMap.find(buffer) != departmentMap.end()) {
                    int backendID = departmentMap[buffer];
                    string message =  string(buffer) + " shows up in backend server " + to_string(backendID) ;
                    cout << message << endl;

                    string response =  "Client has received results from Main Server: \n" + string(buffer) +" is associated with backend server " + to_string(backendID) + ".";
                    send(new_fd, response.c_str(), response.length(), 0);

                    cout << "Main Server has sent searching result to client" << clientId << " using TCP over port "
                         << PORT << endl;
                } else {
                    // Department not found, generate error message
                    stringstream errorMsg;
                    errorMsg << buffer << " does not show up in backend server <";
                    for (const auto& pair : distinctDepartments) {
                        errorMsg  <<pair.first << ",";
                    }
                    string errorMessage = errorMsg.str();
                    errorMessage.pop_back();  // Remove the last comma
                    errorMessage += ">";
                    cout << errorMessage<<endl;

                    cout << "The Main Server has sent \"" << buffer << " not found.\" to client "
                         << s << " using TCP over port " << PORT << endl;

                    string toClient= string(buffer) + " not found.";
                    // Send the error message to the client
                    send(new_fd, (toClient).c_str(), toClient.size(), 0);
                }
            }
            close(new_fd);
            exit(0);
        }
    }

    return 0;
}

