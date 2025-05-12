#ifndef _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_
#define _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_

#include "Namecheap/NamecheapDomainProfileManager.h"

#include <Application/Application.h>
#include <Arguments/ArgumentParser.h>

#include <atomic>
#include <memory>

class NamecheapDynamicDNSAutoUpdater final : public Application {
public:
	NamecheapDynamicDNSAutoUpdater();
	virtual ~NamecheapDynamicDNSAutoUpdater();

	bool isInitialized() const;
	bool initialize(int argc = 0, char * argv[] = nullptr);
	bool initialize(std::shared_ptr<ArgumentParser> arguments);
	void uninitialize();
	bool run();

	static std::string getArgumentHelpInformation();
	static void displayArgumentHelp();
	static void displayVersion();
	static void displayLibraryInformation();
private:
	std::atomic<bool> m_initialized;
	std::shared_ptr<ArgumentParser> m_arguments;
	std::shared_ptr<NamecheapDomainProfileManager> m_domainProfileManager;

	NamecheapDynamicDNSAutoUpdater(const NamecheapDynamicDNSAutoUpdater &) = delete;
	const NamecheapDynamicDNSAutoUpdater & operator = (const NamecheapDynamicDNSAutoUpdater &) = delete;
};

#endif // _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_
