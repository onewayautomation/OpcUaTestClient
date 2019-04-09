// OpcUaTestClient.cpp : Sample app demonstrating how to use 1WA OPC UA Client SDK (https://github.com/onewayautomation/1WaOpcUaSdk)

#include "opcua/Connection.h"
// #include <vld.h>

// Suppress warnings on using STL objects in DLL interface. Safe given that all dependencies are built with the same compiler settings and using the same runtime libs.
#pragma warning (disable: 4275)
#pragma warning (disable: 4251)

using namespace OWA::OpcUa;

int main(int argc, char* argv[])
{
	(void) argc;
	(void) argv;

	// Thread pool size can be set here (optional):
	Utils::initSdk();
 
	{
		std::string serverEndpointUrl = "opc.tcp://opcuaserver.com:48010";
    
    serverEndpointUrl = "opc.tcp://OWA1:48010";

		// Set configuration options for connection: 
		ClientConfiguration config(serverEndpointUrl);
		config.securityMode = SecurityMode::noneSecureMode();


		config.createSession = false; // connecting just to call FindServers and GetEndpoints, therefore no need to create session. 

		// Optional state change callback - it will be called whenever connection state is changed: 
		StateChangeCallback cf = [](const std::string& endpointUrl, ConnectionState state, const OperationResult& result)
		{
			std::cout << "Connection state changed to " << (int)state << "[" << Utils::toString(state) << "], endpoint URL = " << endpointUrl << ", result is " << Utils::toString(result.code) << std::endl;
		};

		// Create connection object and set configuration:
		auto connection = Connection::create(cf);

		

		// Connect synchronously, providing optional callback function too to report this specific connect attempt stages:
    OperationResult connectResult;
    
    //{
    //    connection->setConfiguration(config);
    //  connectResult = connection->connect([](const OperationResult& r) {
    //    std::cout << "Connect callback message: " << r.text.text << std::endl;
    //  }).get();

    //  if (connectResult.isGood()) {
    //    // Call FindServers and GetEndpoints services:
    //    std::shared_ptr<FindServersRequest> findServersRequest(new FindServersRequest());
    //    auto findServersResponse = connection->send(findServersRequest).get();

    //    std::cout << "FindServers returned " << findServersResponse->servers.size() << " records" << std::endl;
    //    for (int index = 0; index < findServersResponse->servers.size(); index++) {
    //      std::cout << index << ": " << findServersResponse->servers[index].applicationName.text << std::endl;
    //    }

    //    // 
    //    GetEndpointsRequest::Ptr getEndpointsRequest(new GetEndpointsRequest(findServersRequest->endpointUrl));
    //    auto getEndpointsResponse = connection->send(getEndpointsRequest).get();

    //    std::cout << "Get Endpoints returned " << getEndpointsResponse->endpoints.size() << " records" << std::endl;
    //    for (int index = 0; index < getEndpointsResponse->endpoints.size(); index++) {
    //      std::cout << index << ": " << getEndpointsResponse->endpoints[index].endpointUrl << std::endl;
    //    }

    //    auto disconnectResult = connection->disconnect(false).get();
    //    std::cout << "Disconnected" << std::endl;
    //  }
    //}

		// Modify configuration to connect to the server with session creation:
		config.serverInfo.endpointUrl = serverEndpointUrl;
    // config.connectionSettings.defaultDiscoveryUrl = serverEndpointUrl;

		// config.serverInfo.localDiscoveryServerUrl = "opc.tcp://opcuaserver.com:48010";
		config.securityMode = SecurityMode(SecurityPolicyId::Basic256Sha256, MessageSecurityMode::SignAndEncrypt);
		config.createSession = true;

		connection->setConfiguration(config);

		// Connect:
		connectResult = connection->connect([](OperationResult result) {
			std::cout << "Connect result is " << result.text.text << std::endl;
		}).get();

		// Wait until connected:
		if (!connectResult.isGood()) {
			while (!connection->isConnected()) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			connectResult = StatusCode::Good;
		}

		if (connectResult.isGood()) 
		{
			// Call various services:
			{
				// Read
				ReadRequest::Ptr readRequest(new ReadRequest(2258)); // CurrentTime node
				readRequest->nodesToRead.push_back(2267); //ServiceLevel node
				auto readResponse = connection->send(readRequest).get();

				for (int index = 0; index < readResponse->results.size(); index++) {
					std::cout << index << ": " << readResponse->results[index].value.toString() << std::endl;
				}
			}

			// Browse
			BrowseRequest::Ptr browseRequest(new BrowseRequest()); // By default initialized to the root folder, Ids::NumericNodeId::RootFolder;
			browseRequest->requestedMaxReferencesPerNode = 1;
			auto browseResponse = connection->send(browseRequest).get();
			BrowseNextRequest::Ptr browseNextRequest(new BrowseNextRequest());
			std::vector<int> indexes;
			if (Utils::isGood(browseResponse)) {
				std::cout << "Browse result:" << std::endl;
				for (int index = 0; index < browseResponse->results.size(); index++) {
					std::cout << "NodeId " << browseRequest->nodesToBrowse[index].nodeId.toString() << ", has " << browseResponse->results[index].references.size() << " references" << std::endl;
					for (int r = 0; r < browseResponse->results[index].references.size(); r++) {
						std::cout << "\t" << browseResponse->results[index].references[r].displayName.toString() << std::endl;
					}
					if (!browseResponse->results[index].continuationPoint.empty()) {
						browseNextRequest->continuationPoints.push_back(browseResponse->results[index].continuationPoint);
						indexes.push_back(index);
					}
				}
				while (!browseNextRequest->continuationPoints.empty()) {
					auto browseNextResponse = connection->send(browseNextRequest).get();
					if (Utils::isGood(browseNextResponse)) {
						browseNextRequest->clear();
						std::vector < int> newIndexes;
						std::cout << "BrowseNext result:" << std::endl;
						for (int index = 0; index < browseNextResponse->results.size(); index++) {
							std::cout << "NodeId " << browseRequest->nodesToBrowse[indexes[index]].nodeId.toString() << ", has " << browseNextResponse->results[index].references.size() << " references" << std::endl;
							for (int r = 0; r < browseNextResponse->results[index].references.size(); r++) {
								std::cout << "\t" << browseNextResponse->results[index].references[r].displayName.toString() << std::endl;
							}
							if (!browseNextResponse->results[index].continuationPoint.empty()) {
								browseNextRequest->continuationPoints.push_back(browseNextResponse->results[index].continuationPoint);
								newIndexes.push_back(indexes[index]);
							}
						}
						indexes = newIndexes;
					}
					else {
						std::cout << "BrowseNext result is " << StatusCodeUtil::toString(browseNextResponse->header.serviceResult) << std::endl;
					}
				}
			}
			else {
				std::cout << "Browse result is " << StatusCodeUtil::toString(browseResponse->header.serviceResult) << std::endl;
			}

			// create subscription
			CreateSubscriptionRequest::Ptr createSubRequest(new CreateSubscriptionRequest());

			// Define callback function receiving notification messages:
			NotificationObserver dataChangesObserver = [&](NotificationMessage& notificationMessage) {
				std::cout << "Received notification with sequence number = " << notificationMessage.sequenceNumber << std::endl;
				for (auto iter = notificationMessage.notificationData.begin(); iter != notificationMessage.notificationData.end(); iter++) {
					DataChangeNotification* dc = dynamic_cast<DataChangeNotification*>(iter->get());
					if (dc != 0) {
						for (auto mi = dc->monitoredItems.begin(); mi != dc->monitoredItems.end(); mi++) {
							std::cout << "Client Handle = " << mi->clientHandle << ", value = " << mi->dataValue.value.toString() << ", status = " << Utils::statusToString(mi->dataValue.statusCode) << std::endl;
						}
					}
				}
			};

			auto createSubResponse = connection->send(createSubRequest, dataChangesObserver).get();

			if (createSubResponse->isGood()) {
				std::cout << "Create Subscription succeeded." << std::endl
					<< "\tid = " << createSubResponse->subscriptionId << std::endl
					<< "\trevisedLifetimeCount = " << createSubResponse->revisedLifetimeCount << std::endl
					<< "\trevisedMaxKeepAliveCount = " << createSubResponse->revisedMaxKeepAliveCount << std::endl
					<< "\trevisedPublishingInterval = " << createSubResponse->revisedPublishingInterval << std::endl;

				// create monitored items
				CreateMonitoredItemsRequest::Ptr createMonReq(new CreateMonitoredItemsRequest());
				createMonReq->subscriptionId = createSubResponse->subscriptionId;

				createMonReq->itemsToCreate.push_back(MonitoredItemCreateRequest(2258));
				createMonReq->itemsToCreate.push_back(MonitoredItemCreateRequest(2259));

				auto createMonRes = connection->send(createMonReq).get();

				if (createMonRes->isGood()) {
					std::cout << "CreateMonitoredItems succeeded." << std::endl;
					for (int index = 0; index < createMonRes->results.size(); index++) {
						std::cout << "\tNode " << createMonReq->itemsToCreate[index].itemToMonitor.nodeId.toString() << " result is " << Utils::statusToString(createMonRes->results[index].statusCode)
							<< ", client id = " << createMonReq->itemsToCreate[index].monitoringParameters.clientHandle << ", server id = " << createMonRes->results[index].monitoredItemId << std::endl;
					}

					std::string s;
					do {
						// Loop until user enters "q":
						std::cin >> s;
					} while (s != "q" && s != "Q");

					// Delete monitored items:
					DeleteMonitoredItemsRequest::Ptr delReq(new DeleteMonitoredItemsRequest());
					delReq->subscriptionId = createSubResponse->subscriptionId;
					for (auto iter = createMonRes->results.begin(); iter != createMonRes->results.end(); iter++) {
						delReq->monitoredItemIds.push_back(iter->monitoredItemId);
					}
					auto delRes = connection->send(delReq).get();
					std::cout << "DeleteMonitoredItems result is " << Utils::statusToString(delRes->header.serviceResult) << std::endl;
					for (auto iter = delRes->results.begin(); iter != delRes->results.end(); iter++) {
						std::cout << "\tDelete Item result is " << Utils::statusToString(*iter) << std::endl;
					}
				}
				else
				{
					std::cout << "CreateMonitoredItems failed: error = " << Utils::statusToString(createMonRes->header.serviceResult) << std::endl;
				}

				DeleteSubscriptionsRequest::Ptr delSubReq(new DeleteSubscriptionsRequest());
				delSubReq->subscriptionIds.push_back(createSubResponse->subscriptionId);
				auto delSubRes = connection->send(delSubReq).get();
				if (delSubRes->isGood()) {
					std::cout << "DeleteSubscriptions succeeded" << std::endl;
					for (auto index = 0; index < delSubRes->results.size(); index++) {
						std::cout << "\tResult of deleting subscription " << delSubReq->subscriptionIds[index] << " is " << Utils::statusToString(delSubRes->results[index]) << std::endl;
					}
				}
				else
				{
					std::cout << "DeleteSubscriptions result is " << Utils::statusToString(delSubRes->header.serviceResult) << std::endl;
				}
			}
			else {
				std::cout << "Create Subscription failed, error code = " << Utils::statusToString(createSubResponse->header.serviceResult) << std::endl;
			}

			connectResult = connection->disconnect(true).get();
		}
		// Calling shutdown method guarantees that no callbacks are made from background thread.
		connection->shutdown();
	}
	Utils::closeSdk();

	return 0;
}
