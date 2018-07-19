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
	Utils::initThreadPool(4);
 
	{
		// Set configuration options for connection: 
		ClientConfiguration config("opc.tcp://opcuaserver.com:48010");
		config.securityMode = SecurityMode::noneSecureMode();
		config.createSession = false; // connecting just to call FindServers and GetEndpoints, therefore no need to create session. 

		// Optional state change callback - it will be called whenever connection state is changed: 
		StateChangeCallback cf = [](const std::string& endpointUrl, ConnectionState state, const OperationResult& result)
		{
			std::cout << "Connection state changed to " << (int)state << "[" << Utils::toString(state) << "], endpoint URL = " << endpointUrl << ", result is " << Utils::toString(result.code) << std::endl;
		};

		// Create connection object and set configuration:
		auto connection = Connection::create(cf);

		connection->setConfiguration(config);

		OperationResult connectResult;
		// Connect synchronously, providing optional callback function too to report this specific connect attempt stages:
		connectResult = connection->connect([](const OperationResult& r) {
			std::cout << "Connect callback message: " << r.text.text << "\n";
		}).get();

		if (connectResult.isGood()) {
			// Call FidnServers and GetEndpoints services:
			std::shared_ptr<FindServersRequest> findServersRequest(new FindServersRequest());
			auto findServersResponse = connection->send(findServersRequest).get();

			std::cout << "FindServers returned " << findServersResponse->servers.size() << " records \n";
			for (int index = 0; index < findServersResponse->servers.size(); index++) {
				std::cout << index << ": " << findServersResponse->servers[index].applicationName.text << "\n";
			}

			// 
			GetEndpointsRequest::Ptr getEndpointsRequest(new GetEndpointsRequest(findServersRequest->endpointUrl));
			auto getEndpointsResponse = connection->send(getEndpointsRequest).get();

			std::cout << "Get Endpoints returned " << getEndpointsResponse->endpoints.size() << " records\n";
			for (int index = 0; index < getEndpointsResponse->endpoints.size(); index++) {
				std::cout << index << ": " << getEndpointsResponse->endpoints[index].endpointUrl << "\n";
			}

			auto disconnectResult = connection->disconnect(false).get();
			std::cout << "Disconnected\n";
		}

		// Modify configuration to connect to the server with session creation:
		config.serverInfo.endpointUrl = "opc.tcp://opcuaserver.com:48010";
		// config.serverInfo.localDiscoveryServerUrl = "opc.tcp://opcuaserver.com:48010";
		// TODO - conneciton in secured mode is WIP. config.securityMode = SecurityMode(SecurityPolicyId::Basic128Rsa15, MessageSecurityMode::SignAndEncrypt);
		config.createSession = true;

		connection->setConfiguration(config);

		// Connect:
		connectResult = connection->connect([](OperationResult result) {
			std::cout << "Connect result is " << result.text.text << "\n";
		}).get();

		// Wait until connected:
		if (!connectResult.isGood()) {
			while (!connection->isConnected()) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			connectResult = StatusCode::Good;
		}

		if (connectResult.isGood()) {
			// Call various services:
			{
				ReadRequest::Ptr readRequest(new ReadRequest(2258)); // CurrentTime node
				readRequest->nodesToRead.push_back(2267); //ServiceLevel node
				auto readResponse = connection->send(readRequest).get();

				for (int index = 0; index < readResponse->results.size(); index++) {
					std::cout << index << ": " << readResponse->results[index].value.toString() << "\n";
				}
			}

			BrowseRequest::Ptr browseRequest(new BrowseRequest(NodeId(Ids::NumericNodeId::RootFolder)));
			browseRequest->requestedMaxReferencesPerNode = 1;
			auto browseResponse = connection->send(browseRequest).get();
			BrowseNextRequest::Ptr browseNextRequest(new BrowseNextRequest());
			std::vector<int> indexes;
			if (Utils::isGood(browseResponse)) {
				std::cout << "Browse result:\n";
				for (int index = 0; index < browseResponse->results.size(); index++) {
					std::cout << "NodeId " << browseRequest->nodesToBrowse[index].nodeId.toString() << ", has " << browseResponse->results[index].references.size() << " references\n";
					for (int r = 0; r < browseResponse->results[index].references.size(); r++) {
						std::cout << "\t" << browseResponse->results[index].references[r].displayName.toString() << "\n";
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
						std::cout << "BrowseNext result:\n";
						for (int index = 0; index < browseNextResponse->results.size(); index++) {
							std::cout << "NodeId " << browseRequest->nodesToBrowse[indexes[index]].nodeId.toString() << ", has " << browseNextResponse->results[index].references.size() << " references\n";
							for (int r = 0; r < browseNextResponse->results[index].references.size(); r++) {
								std::cout << "\t" << browseNextResponse->results[index].references[r].displayName.toString() << "\n";
							}
							if (!browseNextResponse->results[index].continuationPoint.empty()) {
								browseNextRequest->continuationPoints.push_back(browseNextResponse->results[index].continuationPoint);
								newIndexes.push_back(indexes[index]);
							}
						}
						indexes = newIndexes;
					}
					else {
						std::cout << "BrowseNext result is " << StatusCodeUtil::toString(browseNextResponse->header.serviceResult) << "\n";
					}
				}
			}
			else {
				std::cout << "Browse result is " << StatusCodeUtil::toString(browseResponse->header.serviceResult) << "\n";
			}

			CreateSubscriptionRequest::Ptr createSubRequest(new CreateSubscriptionRequest());

			// Define callback function receiving notification messages:
			NotificationObserver dataChangesObserver = [&](NotificationMessage& notificationMessage) {
				std::cout << "Received notification with sequence number = " << notificationMessage.sequenceNumber << "\n";
				for (auto iter = notificationMessage.notificationData.begin(); iter != notificationMessage.notificationData.end(); iter++) {
					DataChangeNotification* dc = dynamic_cast<DataChangeNotification*>(iter->get());
					if (dc != 0) {
						for (auto mi = dc->monitoredItems.begin(); mi != dc->monitoredItems.end(); mi++) {
							std::cout << "Client Handle = " << mi->clientHandle << ", value = " << mi->dataValue.value.toString() << ", status = " << Utils::statusToString(mi->dataValue.statusCode) << "\n";
						}
					}
				}
			};

			auto createSubResponse = connection->send(createSubRequest, dataChangesObserver).get();

			if (createSubResponse->isGood()) {
				std::cout << "Create Subscription succeeded. \n"
					<< "\tid = " << createSubResponse->subscriptionId << "\n"
					<< "\trevisedLifetimeCount = " << createSubResponse->revisedLifetimeCount << "\n"
					<< "\trevisedMaxKeepAliveCount = " << createSubResponse->revisedMaxKeepAliveCount << "\n"
					<< "\trevisedPublishingInterval = " << createSubResponse->revisedPublishingInterval << "\n";

				CreateMonitoredItemsRequest::Ptr createMonReq(new CreateMonitoredItemsRequest());
				createMonReq->subscriptionId = createSubResponse->subscriptionId;

				createMonReq->itemsToCreate.push_back(MonitoredItemCreateRequest(2258));
				createMonReq->itemsToCreate.push_back(MonitoredItemCreateRequest(2259));

				auto createMonRes = connection->send(createMonReq).get();

				if (createMonRes->isGood()) {
					std::cout << "CreateMonitoredItems succeeded. \n";
					for (int index = 0; index < createMonRes->results.size(); index++) {
						std::cout << "\tNode " << createMonReq->itemsToCreate[index].itemToMonitor.nodeId.toString() << " result is " << Utils::statusToString(createMonRes->results[index].statusCode)
							<< ", client id = " << createMonReq->itemsToCreate[index].monitoringParameters.clientHandle << ", server id = " << createMonRes->results[index].monitoredItemId << "\n";
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
					std::cout << "DeleteMonitoredItems result is " << Utils::statusToString(delRes->header.serviceResult) << "\n";
					for (auto iter = delRes->results.begin(); iter != delRes->results.end(); iter++) {
						std::cout << "\tDelete Item result is " << Utils::statusToString(*iter) << "\n";
					}
				}
				else
				{
					std::cout << "CreateMonitoredItems failed: error = " << Utils::statusToString(createMonRes->header.serviceResult) << "\n";
				}

				DeleteSubscriptionsRequest::Ptr delSubReq(new DeleteSubscriptionsRequest());
				delSubReq->subscriptionIds.push_back(createSubResponse->subscriptionId);
				auto delSubRes = connection->send(delSubReq).get();
				if (delSubRes->isGood()) {
					std::cout << "DeleteSubscriptions succeeded\n";
					for (auto index = 0; index < delSubRes->results.size(); index++) {
						std::cout << "\tResult of deleting subscription " << delSubReq->subscriptionIds[index] << " is " << Utils::statusToString(delSubRes->results[index]) << "\n";
					}
				}
				else
				{
					std::cout << "DeleteSubscriptions result is " << Utils::statusToString(delSubRes->header.serviceResult) << "\n";
				}
			}
			else {
				std::cout << "Create Subscription failed, error code = " << Utils::statusToString(createSubResponse->header.serviceResult) << "\n";
			}

			connectResult = connection->disconnect(true).get();

			// Calling shutdown method guarantees that no callbacks are made from background thread.
			connection->shutdown();
		}
	}
	Utils::closeSdk();

	std::cout << "Enter any string to exit";
	std::string s;
	std::cin >> s;

	return 0;
}
