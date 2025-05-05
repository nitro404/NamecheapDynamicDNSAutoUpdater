#include "NamecheapDynamicDNSAutoUpdater.h"

#include <spdlog/spdlog.h>

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
