#include "NamecheapDomainProfileManager.h"

#include "Application/SettingsManager.h"

#include <spdlog/spdlog.h>

NamecheapDomainProfileManager::NamecheapDomainProfileManager() { }

NamecheapDomainProfileManager::~NamecheapDomainProfileManager() = default;

bool NamecheapDomainProfileManager::isInitialized() const {
	return m_initialized;
}

bool NamecheapDomainProfileManager::initialize(const ArgumentCollection * arguments) {
	if(m_initialized) {
		return true;
	}

	SettingsManager * settings = SettingsManager::getInstance();
	bool domainProfilesOverriden = false;
	size_t numberOfDomainProfilesToLoad = 0;
	std::shared_ptr<NamecheapDomainProfileCollection> domainProfiles(std::make_shared<NamecheapDomainProfileCollection>());

	if(arguments != nullptr) {
		std::vector<std::string> domainProfileFilePaths(arguments->getValues("p", "profile"));

		if(!domainProfileFilePaths.empty()) {
			domainProfilesOverriden = true;
			numberOfDomainProfilesToLoad = domainProfileFilePaths.size();

			domainProfiles->loadFrom(domainProfileFilePaths, true);
		}
	}

	if(!domainProfilesOverriden) {
		numberOfDomainProfilesToLoad = settings->domainProfileFilePaths.size();
		domainProfiles->loadFrom(settings->domainProfileFilePaths, true);
	}

	if(domainProfiles == nullptr) {
		spdlog::error("Failed to load one or more Namecheap domain profile files.");
		return false;
	}

	if(domainProfiles->numberOfDomainProfiles() == 0) {
		spdlog::error("No Namecheap domain profiles loaded from files.");
		return false;
	}

	m_domainProfiles = std::move(domainProfiles);

	spdlog::info("Successfully loaded {} Namecheap domain profiles from files.", m_domainProfiles->numberOfDomainProfiles());

	return true;
}
