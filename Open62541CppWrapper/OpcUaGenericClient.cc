//--------------------------------------------------------------------------
// Code generated by the SmartSoft MDSD Toolchain
// The SmartSoft Toolchain has been developed by:
//  
// Service Robotics Research Center
// University of Applied Sciences Ulm
// Prittwitzstr. 10
// 89075 Ulm (Germany)
//
// Information about the SmartSoft MDSD Toolchain is available at:
// www.servicerobotik-ulm.de
//
// Please do not modify this file. It will be re-generated
// running the code generator.
//--------------------------------------------------------------------------

#include "OpcUaGenericClient.hh"

#include <vector>
#include <sstream>
#include <functional>

#include <chrono>
#include <thread>

#ifdef HAS_OPCUA
#ifndef UA_ENABLE_AMALGAMATION
#include <open62541/client_config_default.h>
#include <open62541/client_subscriptions.h>
#include <open62541/client_highlevel.h>
#endif
#endif // HAS_OPCUA

namespace OPCUA {

#ifdef HAS_OPCUA
static void
inactivityCallback (UA_Client *client)
{
	//TODO: check if this method is of any use
	std::cerr << "Server Inactivity" << std::endl;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
// this is an internal registry map that connects subscription handle_entity_update(...)
// method with the client's handleEntity(...) object-member-method
static std::map<UA_UInt32,std::function<void(const UA_UInt32&,UA_DataValue*)>> subscription_bindings;

// static and generic subscription handling method (shared by all client instances)
static void
handle_entity_update(UA_Client *client, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext, UA_DataValue *value)
{
	auto it = subscription_bindings.find(subId);
	if(it != subscription_bindings.end()) {
		// call bound function
		it->second(subId, value);
	}
}

static std::map<UA_UInt32,std::function<void(const UA_UInt32&, const size_t&, UA_Variant*)>> event_bindings;
static void
handle_events(UA_Client *client, UA_UInt32 subId, void *subContext,
               UA_UInt32 monId, void *monContext,
               size_t nEventFields, UA_Variant *eventFields)
{
	// TODO: do we need to consider the monId
	auto it = event_bindings.find(subId);
	if(it != event_bindings.end()) {
		// call bound function
		it->second(subId, nEventFields, eventFields);
	}
}
#endif

bool GenericClient::hasEndpoints(const std::string &address, const bool &display)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

    // Listing endpoints
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, address.c_str(),
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        return false;
    }
    if(display==true) {
    	printf("%i endpoints found\n", (int)endpointArraySize);
        for(size_t i=0;i<endpointArraySize;i++) {
            printf("URL of endpoint %i is %.*s\n", (int)i,
                   (int)endpointArray[i].endpointUrl.length,
                   endpointArray[i].endpointUrl.data);
        }
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    return (endpointArraySize > 0);
}
#endif // HAS_OPCUA

GenericClient::GenericClient(const std::chrono::steady_clock::duration &minSubscriptionInterval)
:	minSubscriptionInterval(minSubscriptionInterval)
{
#ifdef HAS_OPCUA
	// set client to initially to zero (the real initialization happens during connection)
	client = 0;

	// default parent object is the default Objects Folder
	rootObjectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
#endif // HAS_OPCUA
}

GenericClient::~GenericClient()
{
	// make sure the client is disconnected before destroying this instance
	this->disconnect();
}

OPCUA::StatusCode GenericClient::connect(const std::string &address, const std::string &objectPath, const bool activateUpcalls)
{
	// make sure the client is disconnected in any case
	this->disconnect();

#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// create a new client
	client = UA_Client_new();

	// create default client configuration
	UA_ClientConfig *config = UA_Client_getConfig(client);
	UA_ClientConfig_setDefault(config);
	/* Set stateCallback */
	config->inactivityCallback = inactivityCallback;
	/* Perform a connectivity check every 2 seconds */
	config->connectivityCheckInterval = 2000;

	if( hasEndpoints(address) == true ) 
	{
		// Connect client to a server
		UA_StatusCode retval = UA_Client_connect(client, address.c_str());
		//retval = UA_Client_connect_username(client, "opc.tcp://localhost:4840", "user1", "password");
		if(retval != UA_STATUSCODE_GOOD) {
			UA_Client_delete(client);
			client = 0;
			return OPCUA::StatusCode::ERROR_COMMUNICATION;
		}

		// find the root object using its objectPath under the default objects folder
		rootObjectId = this->browseObjectPath(objectPath);

		if(rootObjectId.isNull()) {
			// if the object could no be found -> disconnect client and return wrong ID
			this->disconnect();
			return OPCUA::StatusCode::WRONG_ID;
		}

		// call the method that hopefully creates the client space in derived classes
		if( this->createClientSpace(activateUpcalls) == true ) {
			// client is now connected
			return OPCUA::StatusCode::ALL_OK;
		}
	} else {
		UA_Client_delete(client);
		client = 0;
	}
#endif // HAS_OPCUA
	// something else went wrong
	return OPCUA::StatusCode::ERROR_COMMUNICATION;
}

bool GenericClient::createClientSpace(const bool activateUpcalls)
{
	// no-op
	return true;
}

OPCUA::StatusCode GenericClient::disconnect()
{
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// disconnect and clean-up client
	if(client != 0) {
	    UA_Client_disconnect(client);
	    UA_Client_delete(client);
	    client = 0;
	}
#endif // HAS_OPCUA
	return OPCUA::StatusCode::ALL_OK;
}

#ifdef HAS_OPCUA
NodeId GenericClient::browseObjectPath(const std::string &objectPath) const
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

    // create new node id
    NodeId nodeId;

    // parse path
	std::vector<std::string> path_segments;
	std::stringstream ss(objectPath);
	std::string segment;
	const char delimiter = '/';
	// parse path to get individual path elements
	while(std::getline(ss, segment, delimiter)) {
		path_segments.push_back(segment);
	}

	if(path_segments.size() > 0)
	{
		// start iterating from the default objects folder
		nodeId = NodeId(UA_NS0ID_OBJECTSFOLDER, 0);
		for(auto segment: path_segments) {
			// iteratively call find element for all segments, each time using the next nodeId as root
			if(!nodeId.isNull()) {
				nodeId = this->findElement(segment, nodeId, UA_NODECLASS_OBJECT);
			}
		}
	} // end if(path_elements.size() > 0)

	// nodeId is UA_NODEID_NULL in case the element has no been found
	return nodeId;
}

NodeId GenericClient::findElement(const std::string &elementBrowseName, const NodeId &parentNodeId, const UA_NodeClass &filterNodeType) const
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// new id
	NodeId resultId;

    // create default browse request
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    // use given nodeId as parent (default is UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER))
    bReq.nodesToBrowse[0].nodeId = parentNodeId.getNativeIdCopy();
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */

    // call the browse service
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);

    // look for the element with the given name within the response
    for(size_t i = 0; i < bResp.resultsSize; ++i)
    {
    	// pointer to the browse-results struct
    	UA_BrowseResult *result = &(bResp.results[i]);
        for(size_t j = 0; j < result->referencesSize; ++j)
        {
        	UA_ReferenceDescription *ref = &(result->references[j]);
            // check the node-class type
            if(ref->nodeClass == filterNodeType) {
            	// check the browseName
            	std::string browseName((const char*)ref->browseName.name.data, ref->browseName.name.length);
            	if(browseName == elementBrowseName) {
            		// copy node id into the variable that is returned below
                    resultId = ref->nodeId.nodeId;
                    // we only use the first match and ignore further possible matches
                    break;
            	}
            }
        }
    }
    // cleanup allocated memory
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    // nodeId can also be NULL in case that the searched element was not found
    return resultId;
}

UA_StatusCode GenericClient::registerNode(const NodeId &nodeId)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

    UA_RegisterNodesRequest request;
    UA_RegisterNodesRequest_init(&request);

    request.nodesToRegister = UA_NodeId_new();
    // example: req.nodesToRegister[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    request.nodesToRegister[0] = nodeId;
    request.nodesToRegisterSize = 1;

    UA_RegisterNodesResponse response = UA_Client_Service_registerNodes(client, request);

    // copy result
    UA_StatusCode result = response.responseHeader.serviceResult;

    // store result-nodeIds if needed
    if(result == UA_STATUSCODE_GOOD && response.registeredNodeIdsSize > 0) {
//    	std::cout << "registered node: " << nodeId.identifier.numeric <<
//    			" now is: " << response.registeredNodeIds[0].identifier.numeric << std::endl;
//    UA_NodeId nodeId = res.registeredNodeIds[0];
    }

    UA_RegisterNodesRequest_deleteMembers(&request);
    UA_RegisterNodesResponse_deleteMembers(&response);

    return result;
}

UA_StatusCode GenericClient::unregisterNode(const NodeId &nodeId)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

    UA_UnregisterNodesRequest reqUn;
    UA_UnregisterNodesRequest_init(&reqUn);

    reqUn.nodesToUnregister = UA_NodeId_new();
    reqUn.nodesToUnregister[0] = nodeId;
    reqUn.nodesToUnregisterSize = 1;

    UA_UnregisterNodesResponse resUn = UA_Client_Service_unregisterNodes(client, reqUn);
    UA_StatusCode result = resUn.responseHeader.serviceResult;

    UA_UnregisterNodesRequest_deleteMembers(&reqUn);
    UA_UnregisterNodesResponse_deleteMembers(&resUn);

    return result;
}


OPCUA::StatusCode GenericClient::checkAndGetEntityId(const std::string &entityBrowseName, NodeId &entityId, const UA_NodeClass &filterNodeType) const
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// check if client is connected, if not, return immediately
	if(client == 0) {
		return OPCUA::StatusCode::DISCONNECTED;
	}

	// look for an entity name under the given object
	auto entityIt = entityRegistry.find(entityBrowseName);
	if(entityIt != entityRegistry.end()) {
		entityId = entityIt->second;
	} else {
		// browse for the entity ID on the fly (please implement the createClientSpace() to prevent on-the-fly browsing
		entityId = this->findElement(entityBrowseName, rootObjectId, filterNodeType);
	}

	// check that the entityId has been found
	if(entityId.isNull()) {
		return OPCUA::StatusCode::WRONG_ID;
	}

    return OPCUA::StatusCode::ALL_OK;
}


UA_StatusCode GenericClient::createSubscription(const std::string &entityBrowseName, const std::chrono::steady_clock::duration &subscriptionInterval)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// try getting the entity ID
	NodeId entityId;
	OPCUA::StatusCode result = checkAndGetEntityId(entityBrowseName, entityId);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
	}

	if(subscriptionInterval < minSubscriptionInterval) {
		minSubscriptionInterval = subscriptionInterval;
	}
#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    request.requestedPublishingInterval = std::chrono::duration_cast<std::chrono::milliseconds>(subscriptionInterval).count();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
    	// subscription failed, return immediately
    	return response.responseHeader.serviceResult;
    }

    // save the entity-name for the new subscription-id
    subscriptionsRegistry[response.subscriptionId] = entityBrowseName;

    // create upcall binding
    subscription_bindings[response.subscriptionId] = std::bind(&GenericClient::handleEntity, this, std::placeholders::_1, std::placeholders::_2);

    // create a monitored item
    UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(entityId.getNativeIdCopy());
    monRequest.requestedParameters.samplingInterval = request.requestedPublishingInterval;
    UA_MonitoredItemCreateResult monResponse =
    UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                              UA_TIMESTAMPSTORETURN_BOTH,
                                              monRequest, NULL, handle_entity_update, NULL);
    UA_MonitoredItemCreateRequest_deleteMembers(&monRequest);
    return monResponse.statusCode;
#else
    return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
#endif
}

UA_StatusCode GenericClient::deleteSubscription(const std::string &entityBrowseName)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

#ifdef UA_ENABLE_SUBSCRIPTIONS
	for(auto it=subscriptionsRegistry.begin(); it!=subscriptionsRegistry.end(); it++) {
		// search for the entity to unsubscribe
		if(it->second == entityBrowseName) {
			// delete a single subscription
			return UA_Client_Subscriptions_deleteSingle(client, it->first);
		}
	}
#endif
    return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
}

void GenericClient::handleEntity(const UA_UInt32 &subscriptionId, UA_DataValue *data)
{
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// extract the actual value
	OPCUA::Variant value(data->value);

	auto it = subscriptionsRegistry.find(subscriptionId);
	if(it != subscriptionsRegistry.end())
	{
		{
			// acquire the entityUpdateMutex
			std::unique_lock<std::mutex> entityUpdateLock(entityUpdateMutex);
			// store the update for the respective entity
			entityUpdateValueRegistry[it->second] = value;
			// notify all waiting clients
			entityUpdateSignalRegistry[it->second].notify_all();
		}
		// propagate the update handling to the user method implemented in derived classes
		this->handleVariableValueUpdate(it->second, value);
	}
}
#endif // HAS_OPCUA


void GenericClient::handleVariableValueUpdate(const std::string &variableBrowseName, const OPCUA::Variant &value)
{
	// no-op
}

// this method activates a generic event handler for a given root-node and filter-clause
unsigned int GenericClient::activateEvent(const std::vector<std::string> &eventSelections)
{
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	if(client == 0) {
		return 0;
	}

#ifdef UA_ENABLE_SUBSCRIPTIONS
	/* Create a default subscription */
	UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
	UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(
			client, request,
			NULL, NULL, NULL);
	if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
		return 0;
	}
	UA_UInt32 subId = response.subscriptionId;

	/* Add a MonitoredItem */
	UA_MonitoredItemCreateRequest item;
	UA_MonitoredItemCreateRequest_init(&item);
	item.itemToMonitor.nodeId = rootObjectId;
	item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
	item.monitoringMode = UA_MONITORINGMODE_REPORTING;

	EventEntry event;
	UA_EventFilter_init(&event.filter);
	size_t nSelectClauses = eventSelections.size();
	event.filter.selectClausesSize = nSelectClauses;

	UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand*)
	        UA_Array_new(nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return 0;

    for(size_t i =0; i<nSelectClauses; ++i) {
        UA_SimpleAttributeOperand_init(&selectClauses[i]);
        selectClauses[i].typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        selectClauses[i].browsePathSize = 1;
        selectClauses[i].browsePath = (UA_QualifiedName*)
            UA_Array_new(selectClauses[0].browsePathSize, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
        if(!selectClauses[i].browsePath) {
            UA_SimpleAttributeOperand_delete(selectClauses);
            return 0;
        }
        selectClauses[i].attributeId = UA_ATTRIBUTEID_VALUE;
        selectClauses[i].browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, eventSelections[i].c_str());
    }

	event.filter.selectClauses = selectClauses;

	item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
	item.requestedParameters.filter.content.decoded.data = &event.filter;
	item.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];

	UA_UInt32 monId = 0;
	event.result = UA_Client_MonitoredItems_createEvent(
			client, subId, UA_TIMESTAMPSTORETURN_BOTH, item, &monId,
			handle_events, NULL);
	if(event.result.statusCode == UA_STATUSCODE_GOOD) {
		eventRegistry[subId] = event;
		// store the obect's function pointer within the internal registry
		event_bindings[subId] = std::bind(&GenericClient::handleEventUpcall, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		return subId;
	}
#endif
	return 0;
}

void GenericClient::deactivateEvent(const unsigned int &eventId)
{
	std::unique_lock<std::recursive_mutex> lock(clientMutex);
#ifdef UA_ENABLE_SUBSCRIPTIONS
	auto eventEntry = eventRegistry.find(eventId);
	if(eventEntry != eventRegistry.end()) {
		UA_Client_Subscriptions_deleteSingle(client, eventId);
		UA_MonitoredItemCreateResult_clear(&eventEntry->second.result);
		UA_Array_delete(eventEntry->second.filter.selectClauses, eventEntry->second.filter.selectClausesSize, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
		eventRegistry.erase(eventEntry);
	}
#endif
}

void GenericClient::handleEventUpcall(const UA_UInt32 &eventId, const size_t &nEventFields, UA_Variant *eventFields)
{
	std::unique_lock<std::recursive_mutex> lock(clientMutex);
#ifdef UA_ENABLE_SUBSCRIPTIONS
	auto eventEntry = eventRegistry.find(eventId);
	if(eventEntry != eventRegistry.end()) {
		std::map<std::string,OPCUA::Variant> eventData;
		UA_EventFilter filter = eventEntry->second.filter;
		for(size_t i=0; i<nEventFields; ++i) {
			if(i <= filter.selectClausesSize) {
				// here we assume that the field-names match the same order as the incoming event-fields
				UA_String uname = filter.selectClauses[i].browsePath[0].name;
				std::string field_name((const char*)uname.data, uname.length);
				eventData[field_name] = OPCUA::Variant(eventFields[i]);
			}
		}
		this->handleEvent(eventId, eventData);
	}
#endif
}

/// overload this method in derived classes to individually handle events
void GenericClient::handleEvent(const unsigned int &eventId, const std::map<std::string,OPCUA::Variant> &eventData)
{
	// no-op
}

bool GenericClient::addVariableNode(const std::string &entityBrowseName, const bool activateUpdateUpcall, const std::chrono::steady_clock::duration &samplingInterval)
{
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// try getting the entity ID
	NodeId entityId;
	OPCUA::StatusCode result = checkAndGetEntityId(entityBrowseName, entityId);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return false;
	}

	// save the entity ID within the internal registry
	entityRegistry[entityBrowseName] = entityId;
	
	if(activateUpdateUpcall == true) {
		// create subscription for the current entity
		this->createSubscription(entityBrowseName, samplingInterval);
	}
	return true;
#else
	return false;
#endif // HAS_OPCUA
}

bool GenericClient::addMethodNode(const std::string &methodBrowseName)
{
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	NodeId methodId;
	OPCUA::StatusCode result = checkAndGetEntityId(methodBrowseName, methodId, UA_NODECLASS_METHOD);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return false;
	}

	// save the method ID within the internal registry
	entityRegistry[methodBrowseName] = methodId;
	return true;
#else
	return false;
#endif // HAS_OPCUA
}

OPCUA::StatusCode GenericClient::getVariableCurrentValue(const std::string &variableName, OPCUA::Variant &value) const
{
	OPCUA::StatusCode result = OPCUA::StatusCode::ERROR_COMMUNICATION;
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// check if the entity ID can be found
	NodeId entityId;
	result = checkAndGetEntityId(variableName, entityId);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return result;
	}

	// Read attribute value
	UA_Variant *uaVar = UA_Variant_new();
	UA_StatusCode retval = UA_Client_readValueAttribute(client, entityId, uaVar);
	if(retval == UA_STATUSCODE_GOOD) {
		bool takeOwnership = true;
		value = Variant(uaVar, takeOwnership);
		result = OPCUA::StatusCode::ALL_OK;
	} else {
		result = OPCUA::StatusCode::ERROR_COMMUNICATION;
	}
#endif // HAS_OPCUA
	return result;
}

OPCUA::StatusCode GenericClient::getVariableNextValue(const std::string &variableName, OPCUA::Variant &value)
{
	// don't lock the clientMutex here as this would lead to a deadlock with the entityUpdateSignalRegistry (see below)
	OPCUA::StatusCode result = OPCUA::StatusCode::ERROR_COMMUNICATION;
#ifdef HAS_OPCUA
	// check if the entity ID can be found
	NodeId entityId;
	result = checkAndGetEntityId(variableName, entityId);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return result;
	}

	// acquire the entityUpdateMutex
	std::unique_lock<std::mutex> entityUpdateLock(entityUpdateMutex);
	// blocking wait until the entity is released
	entityUpdateSignalRegistry[variableName].wait(entityUpdateLock);
	// copy the updated value
	value = entityUpdateValueRegistry[variableName];
#endif // HAS_OPCUA
	// return result
	return result;
}

OPCUA::StatusCode GenericClient::setVariableValue(const std::string &variableName, const OPCUA::Variant &value)
{
	OPCUA::StatusCode result = OPCUA::StatusCode::ERROR_COMMUNICATION;
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// check if the entity ID can be found
	NodeId entityId;
	result = checkAndGetEntityId(variableName, entityId);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return result;
	}

	// write attribute value
	UA_StatusCode retval = UA_Client_writeValueAttribute(client, entityId, value.getInternalValuePtr().get());
	if(retval == UA_STATUSCODE_GOOD) {
    	result = OPCUA::StatusCode::ALL_OK;
	} else {
		result = OPCUA::StatusCode::ERROR_COMMUNICATION;
	}
#endif // HAS_OPCUA
	// return result
	return result;
}

OPCUA::StatusCode GenericClient::callMethod(const std::string &methodName,
                        const std::vector<OPCUA::Variant> &inputArguments,
                        std::vector<OPCUA::Variant> &outputArguments)
{
#ifdef UA_ENABLE_METHODCALLS
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// check if the method ID can be found
	NodeId methodId;
	OPCUA::StatusCode result = checkAndGetEntityId(methodName, methodId, UA_NODECLASS_METHOD);
	if(result != OPCUA::StatusCode::ALL_OK) {
		return result;
	}

	// create input arguments
	std::vector<UA_Variant> uaInputArguments(inputArguments.size());
	for(size_t i=0; i<inputArguments.size(); ++i) {
		uaInputArguments[i] = inputArguments[i].getInternalValueCopy();
	}

	// output variables
	size_t outputSize=0;
	UA_Variant *output;

	// call the method (using high-level interface)
	UA_StatusCode retval = UA_Client_call(client,
    	rootObjectId, 	// the parent object ID
		methodId,		// the current method ID
		inputArguments.size(), uaInputArguments.data(), // input arguments
		&outputSize, &output // output arguments
	);

	if(retval == UA_STATUSCODE_GOOD) {
		// collect and return the output arguments
		outputArguments.resize(outputSize);
		for(size_t i=0; i<outputSize; ++i) {
			outputArguments[i] = OPCUA::Variant(output[i]);
		}
		// cleanup output arguments memory
		UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
		// cleanup input arguments memory
		for(size_t i=0; i<uaInputArguments.size(); ++i) {
			UA_Variant_deleteMembers(&uaInputArguments[i]);
		}
		// return SUCCESS
		return OPCUA::StatusCode::ALL_OK;
	}
#endif
	return OPCUA::StatusCode::ERROR_COMMUNICATION;
}

OPCUA::StatusCode GenericClient::run_once() const
{
#ifdef HAS_OPCUA
	// lock client mutex
	std::unique_lock<std::recursive_mutex> lock(clientMutex);

	// check if client is connected at all (if not, sleep for the minSubscriptionInterval time and return DISCONNECTED)
	if(client == 0) {
		lock.unlock();
		std::this_thread::sleep_for(minSubscriptionInterval);
		return OPCUA::StatusCode::DISCONNECTED;
	}

	// calculate the wake-up-time
	std::chrono::system_clock::time_point wakeupTime = std::chrono::system_clock::now() + minSubscriptionInterval;

	/* if already connected, this will return GOOD and do nothing */
	/* if the connection is closed/errored, the connection will be reset and then reconnected */
	/* Alternatively you can also use UA_Client_getState to get the current state */
	UA_ClientState clientState = UA_Client_getState(client);
	if(clientState != UA_CLIENTSTATE_SESSION) {
		std::cerr << "client-state != UA_CLIENTSTATE_SESSION: " << clientState << std::endl;
		/* The connect may timeout after 1 second (see above) or it may fail immediately on network errors */
		/* E.g. name resolution errors or unreachable network. Thus there should be a small sleep here */
		std::this_thread::sleep_for(minSubscriptionInterval);
		return OPCUA::StatusCode::DISCONNECTED;
	}

	// iterate client's async interface at least once for every subscription
	for(size_t i=0; i < subscriptionsRegistry.size(); ++i) {
		// run client's callback interface non-blocking
		UA_Client_run_iterate(client, 0);
	}
	// iterate client's async interface at least once for every subscription
	for(size_t i=0; i < eventRegistry.size(); ++i) {
		// run client's callback interface non-blocking
		UA_Client_run_iterate(client, 0);
	}

	// release the lock BEFORE! the sleep is called (this enables using synchronous calls more frequently than the subscription interval)
	lock.unlock();

	if(wakeupTime > std::chrono::system_clock::now()) {
		std::this_thread::sleep_until(wakeupTime);
	}

	return OPCUA::StatusCode::ALL_OK;
#else
	return OPCUA::StatusCode::ERROR_COMMUNICATION;
#endif
}

} /* namespace OPCUA */
