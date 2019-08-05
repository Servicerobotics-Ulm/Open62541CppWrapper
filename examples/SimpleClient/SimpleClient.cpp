//============================================================================
// Name        : OpcUaSimpleClient.cpp
// Author      : Alexej
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "OpcUaGenericClient.hh"

#include <chrono>
#include <thread>
#include <mutex>

static bool running = true;
static void stopHandler(int sig) {
	std::cout << "Received ctrl-c" << std::endl;
    running = false;
}

std::mutex mutex;

class MyClient: public OPCUA::GenericClient {
protected:
	virtual void handleVariableValueUpdate(const std::string &entityName, const OPCUA::Variant &value) override
	{
		std::cout << "handleEntityUpdate(" << entityName << "): " << value << std::endl;
	}

	virtual bool createClientSpace(const bool activateUpcalls=true) override
	{
		std::chrono::steady_clock::duration interval = std::chrono::milliseconds(100);
		return addVariableNode("MyVar", activateUpcalls, interval);
	}
public:
	MyClient() { }
	virtual ~MyClient() { }
};

void run(MyClient *client) {
	OPCUA::StatusCode status;
	do {
		std::cout << "run_once()..." <<std::endl;
		status = client->run_once();
	} while(running == true);
}

int main(int argc, char* argv[]) 
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

	std::string address = "opc.tcp://localhost:4840";
	std::string serverName = "TestServer";
	if(argc > 1) {
		serverName = argv[1];
	}

	// create and connect client
	MyClient client;
	std::cout << "connecting client: " << client.connect(address, serverName) << std::endl;

	// call a remote method at the sever
	std::vector<OPCUA::Variant> inputArguments(2);
	inputArguments[0] = 100;
	inputArguments[1] = std::string("Hello");
	std::vector<OPCUA::Variant> outputArguments;
	std::cout << "call method MyMethod: " << client.callMethod("MyMethod", inputArguments, outputArguments) << std::endl;
	for(size_t i=0; i<outputArguments.size(); ++i) {
		std::cout << "output" << i << ": " << outputArguments[i] << std::endl;
	}

	// execute client's upcall interface (i.e. its method "run_once()") in a separate thread (just for testing, can also be executed in this main)
	std::thread th(run, &client);

	while(running == true) {
		OPCUA::Variant v;
		// actively poll for variable values
		client.getVariableCurrentValue("MyVar", v);
		std::cout << "polling MyVar: " << v << std::endl;
		std::this_thread::sleep_for (std::chrono::milliseconds(50));
	}

	th.join();
	return 0;
}
