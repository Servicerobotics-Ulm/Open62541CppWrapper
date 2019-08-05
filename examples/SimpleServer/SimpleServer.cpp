//============================================================================
// Name        : SimpleServer.cpp
// Author      : Alexej
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "OpcUaGenericServer.hh"

#include <thread>
#include <chrono>

class MyServer : public OPCUA::GenericServer {
private:
	std::thread thr;
	bool cancelled;

	virtual void handleMethodCall(const std::string &browseName,
			const std::vector<OPCUA::Variant> &inputs,
			std::vector<OPCUA::Variant> &outputs) override
	{
		std::cout << "CallMethod(" << browseName << ", ";
		for(size_t i=0; i<inputs.size(); ++i) {
			std::cout << "input" << i << ": " << inputs[i] << ", ";
		}
		std::cout << ")" << std::endl;

		outputs[0] = std::string("It Works!!!");
	}
public:
	MyServer(const std::string &rootObjectName)
	:	OPCUA::GenericServer(rootObjectName)
	{
		cancelled = false;
	}
	virtual ~MyServer() {
		stopThread();
	}

	void startThread() {
		thr = std::thread(&MyServer::execution, this);
	}

	void stopThread() {
		cancelled = true;
		thr.join();
	}

	void execution() {
		int i=0;
		while(!cancelled) {
			std::cout << "set MyVar to " << i << std::endl;
			writeVariable("MyVar", i++);
			std::this_thread::sleep_for (std::chrono::milliseconds(200));
		}
	}

	virtual bool createServerSpace() override {

		// add an integer variable
		bool readOnly = false;
		int myVar = 0;
		if(addVariableNode("MyVar", myVar, readOnly) != true) {
			std::cout << "failed adding MyVar" << std::endl;
			return false;
		}

		// add a method
		std::map<std::string, OPCUA::Variant> inputArguments;
		inputArguments["IntInput1"] = 0;
		inputArguments["StringInput2"] = std::string("");

		std::map<std::string, OPCUA::Variant> outputArguments;
		outputArguments["Result"] = std::string("");

		if(addMethodNode("MyMethod", inputArguments, outputArguments) != true) {
			std::cout << "failed adding MyMethod" << std::endl;
			return false;
		}
		return true;
	}
};

int main(int argc, char* argv[])
{
	std::string rootObjectName = "TestServer";
	if(argc > 1) {
		rootObjectName = argv[1];
	}

	MyServer server(rootObjectName);

	server.startThread();

	return server.run();
}
