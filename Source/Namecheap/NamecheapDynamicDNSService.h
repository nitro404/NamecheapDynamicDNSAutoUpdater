#ifndef _NAMECHEAP_DYNAMIC_DNS_SERVICE_H_
#define _NAMECHEAP_DYNAMIC_DNS_SERVICE_H_

#include <string>
#include <string_view>
#include <vector>

class NamecheapDynamicDNSService final {
public:
	NamecheapDynamicDNSService();
	~NamecheapDynamicDNSService();

	bool updateIPAddress(const std::vector<std::string> & hosts, std::string_view domainName, std::string_view password);
	bool updateIPAddress(std::string_view host, std::string_view domain, std::string_view password);
	bool setIPAddress(const std::vector<std::string> & hosts, std::string_view domainName, std::string_view password, std::string_view ipAddress);
	bool setIPAddress(std::string_view host, std::string_view domain, std::string_view password, std::string_view ipAddress);

private:
	NamecheapDynamicDNSService(const NamecheapDynamicDNSService &) = delete;
	const NamecheapDynamicDNSService & operator = (const NamecheapDynamicDNSService &) = delete;
};

#endif // _NAMECHEAP_DYNAMIC_DNS_SERVICE_H_
