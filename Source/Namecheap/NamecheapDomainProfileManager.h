#ifndef _NAMECHEAP_DOMAIN_PROFILE_MANAGER_H_
#define _NAMECHEAP_DOMAIN_PROFILE_MANAGER_H_

#include "NamecheapDomainProfileCollection.h"

#include <Arguments/ArgumentCollection.h>

#include <atomic>
#include <memory>

class NamecheapDomainProfileManager final {
public:
	NamecheapDomainProfileManager();
	~NamecheapDomainProfileManager();

	bool isInitialized() const;
	bool initialize(const ArgumentCollection * arguments);

private:
	std::atomic<bool> m_initialized;
	std::shared_ptr<NamecheapDomainProfileCollection> m_domainProfiles;

	NamecheapDomainProfileManager(const NamecheapDomainProfileManager &) = delete;
	const NamecheapDomainProfileManager & operator = (const NamecheapDomainProfileManager &) = delete;
};

#endif // _NAMECHEAP_DOMAIN_PROFILE_MANAGER_H_
