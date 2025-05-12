#include "NamecheapDynamicDNSService.h"

#include <Network/HTTPService.h>
#include <Network/IPAddressService.h>
#include <Utilities/FileUtilities.h>

#include <spdlog/spdlog.h>

static const std::string NAMECHEAP_DYNAMIC_DNS_BASE_URL("https://dynamicdns.park-your-domain.com");
static const std::string NAMECHEAP_DYNAMIC_DNS_UPDATE_PATH("update");
static const std::string HOST_QUERY_PARAMETER("host");
static const std::string DOMAIN_QUERY_PARAMETER("domain");
static const std::string PASSWORD_QUERY_PARAMETER("password");
static const std::string IP_ADDRESS_QUERY_PARAMETER("ip");

NamecheapDynamicDNSService::NamecheapDynamicDNSService() { }

NamecheapDynamicDNSService::~NamecheapDynamicDNSService() { }

bool NamecheapDynamicDNSService::updateIPAddress(std::string_view host, std::string_view domain, std::string_view password) {
	return updateIPAddress({ host }, domain, password);
}

bool NamecheapDynamicDNSService::updateIPAddress(const std::vector<std::string> & hosts, std::string_view domain, std::string_view password) {
	std::string ipAddress(IPAddressService::getInstance()->getIPAddress(IPAddressService::IPAddressType::V4));

	if(ipAddress.empty()) {
		spdlog::error("Failed to determine external IP address.");
		return false;
	}

	return setIPAddress(hosts, domain, password, ipAddress);
}

bool NamecheapDynamicDNSService::setIPAddress(std::string_view host, std::string_view domain, std::string_view password, std::string_view ipAddress) {
	if(host.empty() || domain.empty() || password.empty() || ipAddress.empty()) {
		spdlog::error("Missing or invalid arguments provided when attempting to set Namecheap domain IP address.");
		return false;
	}

	HTTPService * httpService = HTTPService::getInstance();

	if(!httpService->isInitialized()) {
		spdlog::error("Failed to initialize HTTP service.");
		return false;
	}

	std::shared_ptr<HTTPRequest> request(httpService->createRequest(HTTPRequest::Method::Get, Utilities::joinPaths(NAMECHEAP_DYNAMIC_DNS_BASE_URL, NAMECHEAP_DYNAMIC_DNS_UPDATE_PATH)));
	request->addQueryParameter(HOST_QUERY_PARAMETER, host);
	request->addQueryParameter(DOMAIN_QUERY_PARAMETER, domain);
	request->addQueryParameter(PASSWORD_QUERY_PARAMETER, password);
	request->addQueryParameter(IP_ADDRESS_QUERY_PARAMETER, ipAddress);

	std::shared_ptr<HTTPResponse> response(httpService->sendRequestAndWait(request));

	if(response == nullptr || response->isFailure()) {
		spdlog::error("Failed to update IP address with error: {}", response != nullptr ? response->getErrorMessage() : "Invalid request.");
		return false;
	}

	if(response->isFailureStatusCode()) {
		std::string statusCodeName(HTTPUtilities::getStatusCodeName(response->getStatusCode()));
		spdlog::error("Failed to update IP address ({}{})!", response->getStatusCode(), statusCodeName.empty() ? "" : " " + statusCodeName);
		return false;
	}

	return true;
}

bool NamecheapDynamicDNSService::setIPAddress(const std::vector<std::string> & hosts, std::string_view domain, std::string_view password, std::string_view ipAddress) {
	if(hosts.empty()) {
		return false;
	}

	bool allIPAddressesSet = true;

	for(const std::string & host : hosts) {
		if(!setIPAddress(host, domain, password, ipAddress)) {
			allIPAddressesSet = false;
		}
	}

	return allIPAddressesSet;
}
