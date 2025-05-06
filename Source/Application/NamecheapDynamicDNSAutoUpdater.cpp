#include "NamecheapDynamicDNSAutoUpdater.h"

#include "Project.h"

#include <Network/HTTPService.h>
#include <Network/IPAddressService.h>
#include <Utilities/FileUtilities.h>
#include <Utilities/StringUtilities.h>

#include <spdlog/spdlog.h>

static const std::string HTTP_USER_AGENT(Utilities::replaceAll(APPLICATION_NAME, " ", "") + "/" + APPLICATION_VERSION);

NamecheapDynamicDNSAutoUpdater::NamecheapDynamicDNSAutoUpdater()
	: Application()
	, m_initialized(false) { }

NamecheapDynamicDNSAutoUpdater::~NamecheapDynamicDNSAutoUpdater() { }

bool NamecheapDynamicDNSAutoUpdater::initialize(int argc, char * argv[]) {
	std::shared_ptr<ArgumentParser> arguments;

	if(argc != 0) {
		arguments = std::make_shared<ArgumentParser>(argc, argv);
	}

	return initialize(arguments);
}

bool NamecheapDynamicDNSAutoUpdater::initialize(std::shared_ptr<ArgumentParser> arguments) {
	if(m_initialized) {
		return false;
	}

	HTTPConfiguration configuration;
	configuration.certificateAuthorityCertificateStoreDirectoryPath = Utilities::joinPaths("../Data", "cURL");

	HTTPService * httpService = HTTPService::getInstance();
	httpService->setUserAgent(HTTP_USER_AGENT);

	if(!httpService->initialize(configuration)) {
		spdlog::error("Failed to initialize HTTP service!");
		return false;
	}

	m_initialized = true;

	return true;
}

void NamecheapDynamicDNSAutoUpdater::uninitialize() {
	if(!m_initialized) {
		return;
	}

	m_initialized = false;
}

bool NamecheapDynamicDNSAutoUpdater::run() {
	if(!m_initialized) {
		spdlog::error("Application must be initialized first.");
		return false;
	}

	return true;
}
