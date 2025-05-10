#ifndef _NAMECHEAP_DOMAIN_PROFILE_H_
#define _NAMECHEAP_DOMAIN_PROFILE_H_

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class NamecheapDomainProfile final {
public:
	NamecheapDomainProfile(std::vector<std::string> && hosts, std::string_view domain, std::string_view password);
	NamecheapDomainProfile(const std::vector<std::string> & hosts, std::string_view domain, std::string_view password);
	NamecheapDomainProfile(NamecheapDomainProfile && domainProfile) noexcept;
	NamecheapDomainProfile(const NamecheapDomainProfile & domainProfile);
	NamecheapDomainProfile & operator = (NamecheapDomainProfile && domainProfile) noexcept;
	NamecheapDomainProfile & operator = (const NamecheapDomainProfile & domainProfile);
	~NamecheapDomainProfile();

	size_t numberOfHosts() const;
	bool hasHost(std::string_view host) const;
	size_t indexOfHost(std::string_view host) const;
	const std::string & getHost(size_t index) const;
	const std::vector<std::string> getHosts() const;
	const std::string & getDomain() const;
	const std::string & getPassword() const;

	rapidjson::Value toJSON(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) const;
	static std::unique_ptr<NamecheapDomainProfile> parseFrom(const rapidjson::Value & domainProfileValue);
	static std::vector<std::unique_ptr<NamecheapDomainProfile>> parseFromList(const rapidjson::Value & domainProfileListValue);

	bool isValid() const;
	static bool isValid(const NamecheapDomainProfile * domainProfile);

	bool operator == (const NamecheapDomainProfile & domainProfile) const;
	bool operator != (const NamecheapDomainProfile & domainProfile) const;

private:
	std::vector<std::string> m_hosts;
	std::string m_domain;
	std::string m_password;
};

#endif // _NAMECHEAP_DOMAIN_PROFILE_H_
