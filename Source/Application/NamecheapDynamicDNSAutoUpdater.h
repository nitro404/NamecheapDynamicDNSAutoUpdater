#ifndef _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_
#define _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_

#include <Application/Application.h>
#include <Arguments/ArgumentParser.h>

#include <atomic>
#include <memory>

class NamecheapDynamicDNSAutoUpdater final : public Application {
public:
	NamecheapDynamicDNSAutoUpdater();
	virtual ~NamecheapDynamicDNSAutoUpdater();

	bool initialize(int argc = 0, char * argv[] = nullptr);
	bool initialize(std::shared_ptr<ArgumentParser> arguments);
	void uninitialize();
	bool run();

private:
	std::atomic<bool> m_initialized;
	std::shared_ptr<ArgumentParser> m_arguments;

	NamecheapDynamicDNSAutoUpdater(const NamecheapDynamicDNSAutoUpdater &) = delete;
	const NamecheapDynamicDNSAutoUpdater & operator = (const NamecheapDynamicDNSAutoUpdater &) = delete;
};

#endif // _NAMECHEAP_DYNAMIC_DNS_AUTO_UPDATER_H_
