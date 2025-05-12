#include "NamecheapDynamicDNSAutoUpdater.h"

#include "Project.h"
#include "SettingsManager.h"

#include <Network/HTTPService.h>
#include <Platform/TimeZoneDataManager.h>
#include <Utilities/FileUtilities.h>
#include <Utilities/StringUtilities.h>

#include <spdlog/spdlog.h>

static const std::string HTTP_USER_AGENT(Utilities::replaceAll(APPLICATION_NAME, " ", "") + "/" + APPLICATION_VERSION);

NamecheapDynamicDNSAutoUpdater::NamecheapDynamicDNSAutoUpdater()
	: Application()
	, m_initialized(false)
	, m_domainProfileManager(std::make_shared<NamecheapDomainProfileManager>()) {
	FactoryRegistry & factoryRegistry = FactoryRegistry::getInstance();

	factoryRegistry.setFactory<SettingsManager>([]() {
		return std::make_unique<SettingsManager>();
	});
}

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

	if(arguments != nullptr) {
		m_arguments = arguments;
	}

	SettingsManager * settings = SettingsManager::getInstance();

	if(!settings->isLoaded()) {
		settings->load(m_arguments.get());
	}

	HTTPConfiguration configuration = {
		Utilities::joinPaths(settings->dataDirectoryPath, settings->curlDataDirectoryName),
		"",
		settings->connectionTimeout,
		settings->networkTimeout,
		settings->transferTimeout
	};

	HTTPService * httpService = HTTPService::getInstance();
	httpService->setUserAgent(HTTP_USER_AGENT);
	httpService->setVerboseLoggingEnabled(settings->verboseRequestLogging);

	if(!httpService->initialize(configuration)) {
		spdlog::error("Failed to initialize HTTP service!");
		return false;
	}

	if(!settings->downloadThrottlingEnabled || !settings->cacertLastDownloadedTimestamp.has_value() || std::chrono::system_clock::now() - settings->cacertLastDownloadedTimestamp.value() > settings->cacertUpdateFrequency) {
		if(httpService->updateCertificateAuthorityCertificateAndWait()) {
			settings->cacertLastDownloadedTimestamp = std::chrono::system_clock::now();
			settings->save();
		}
	}

	bool timeZoneDataUpdated = false;
	bool shouldUpdateTimeZoneData = !settings->downloadThrottlingEnabled || !settings->timeZoneDataLastDownloadedTimestamp.has_value() || std::chrono::system_clock::now() - settings->timeZoneDataLastDownloadedTimestamp.value() > settings->timeZoneDataUpdateFrequency;

	if(!TimeZoneDataManager::getInstance()->initialize(Utilities::joinPaths(settings->dataDirectoryPath, settings->timeZoneDataDirectoryName), settings->fileETags, shouldUpdateTimeZoneData, false, &timeZoneDataUpdated)) {
		spdlog::error("Failed to initialize time zone data manager!");
		return false;
	}

	if(timeZoneDataUpdated) {
		settings->timeZoneDataLastDownloadedTimestamp = std::chrono::system_clock::now();
		settings->save();
	}

	if(!m_domainProfileManager->initialize(arguments.get())) {
		spdlog::error("Failed to initialize domain profile manager!");
		return false;
	}

	m_initialized = true;

	return true;
}

void NamecheapDynamicDNSAutoUpdater::uninitialize() {
	if(!m_initialized) {
		return;
	}

	SettingsManager * settings = SettingsManager::getInstance();

	settings->save(m_arguments.get());

	if(m_arguments != nullptr) {
		m_arguments.reset();
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
