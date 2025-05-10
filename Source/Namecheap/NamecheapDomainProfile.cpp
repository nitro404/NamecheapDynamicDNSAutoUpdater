#include "NamecheapDomainProfile.h"

#include <Utilities/RapidJSONUtilities.h>
#include <Utilities/StringUtilities.h>

#include <spdlog/spdlog.h>

#include <array>

static constexpr const char * JSON_HOSTS_PROPERTY_NAME = "hosts";
static constexpr const char * JSON_HOST_PROPERTY_NAME = "host";
static constexpr const char * JSON_DOMAIN_PROPERTY_NAME = "domain";
static constexpr const char * JSON_PASSWORD_PROPERTY_NAME = "password";
static const std::array<std::string_view, 4> JSON_PROPERTY_NAMES({
	JSON_HOSTS_PROPERTY_NAME,
	JSON_HOST_PROPERTY_NAME,
	JSON_DOMAIN_PROPERTY_NAME,
	JSON_PASSWORD_PROPERTY_NAME
});

NamecheapDomainProfile::NamecheapDomainProfile(std::vector<std::string> && hosts, std::string_view domain, std::string_view password)
	: m_hosts(std::move(hosts))
	, m_domain(domain)
	, m_password(password)
{
}

NamecheapDomainProfile::NamecheapDomainProfile(const std::vector<std::string> & hosts, std::string_view domain, std::string_view password)
	: m_hosts(hosts)
	, m_domain(domain)
	, m_password(password)
{
}

NamecheapDomainProfile::NamecheapDomainProfile(NamecheapDomainProfile && domainProfile) noexcept
	: m_hosts(std::move(domainProfile.m_hosts))
	, m_domain(std::move(domainProfile.m_domain))
	, m_password(std::move(domainProfile.m_password)) { }

NamecheapDomainProfile::NamecheapDomainProfile(const NamecheapDomainProfile & domainProfile)
	: m_hosts(domainProfile.m_hosts)
	, m_domain(domainProfile.m_domain)
	, m_password(domainProfile.m_password) { }

NamecheapDomainProfile & NamecheapDomainProfile::operator = (NamecheapDomainProfile && domainProfile) noexcept {
	if(this != &domainProfile) {
		m_hosts = std::move(domainProfile.m_hosts);
		m_domain = std::move(domainProfile.m_domain);
		m_password = std::move(domainProfile.m_password);
	}

	return *this;
}

NamecheapDomainProfile & NamecheapDomainProfile::operator = (const NamecheapDomainProfile & domainProfile) {
	m_hosts = domainProfile.m_hosts;
	m_domain = domainProfile.m_domain;
	m_password = domainProfile.m_password;

	return *this;
}

NamecheapDomainProfile::~NamecheapDomainProfile() = default;

size_t NamecheapDomainProfile::numberOfHosts() const {
	return m_hosts.size();
}

bool NamecheapDomainProfile::hasHost(std::string_view host) const {
	return indexOfHost(host) != std::numeric_limits<size_t>::max();
}

size_t NamecheapDomainProfile::indexOfHost(std::string_view host) const {
	std::vector<std::string>::const_iterator hostIterator(std::find(m_hosts.cbegin(), m_hosts.cend(), host));

	if(hostIterator == m_hosts.cend()) {
		return std::numeric_limits<size_t>::max();
	}

	return hostIterator - m_hosts.cbegin();
}

const std::string & NamecheapDomainProfile::getHost(size_t index) const {
	if(index >= m_hosts.size()) {
		return Utilities::emptyString;
	}

	return m_hosts[index];
}

const std::vector<std::string> NamecheapDomainProfile::getHosts() const {
	return m_hosts;
}

const std::string & NamecheapDomainProfile::getDomain() const {
	return m_domain;
}

const std::string & NamecheapDomainProfile::getPassword() const {
	return m_password;
}

rapidjson::Value NamecheapDomainProfile::toJSON(rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator) const {
	rapidjson::Value domainProfileValue(rapidjson::kObjectType);

	if(m_hosts.size() == 1) {
		rapidjson::Value hostValue(m_hosts.front().c_str(), allocator);
		domainProfileValue.AddMember(rapidjson::StringRef(JSON_HOST_PROPERTY_NAME), hostValue, allocator);
	}
	else {
		rapidjson::Value hostsValue(rapidjson::kArrayType);
		hostsValue.Reserve(m_hosts.size(), allocator);

		for(const std::string & host : m_hosts) {
			rapidjson::Value hostValue(host.c_str(), allocator);
			hostsValue.PushBack(hostValue, allocator);
		}

		domainProfileValue.AddMember(rapidjson::StringRef(JSON_HOSTS_PROPERTY_NAME), hostsValue, allocator);
	}

	rapidjson::Value domainValue(m_domain.c_str(), allocator);
	domainProfileValue.AddMember(rapidjson::StringRef(JSON_DOMAIN_PROPERTY_NAME), domainValue, allocator);

	rapidjson::Value passwordValue(m_password.c_str(), allocator);
	domainProfileValue.AddMember(rapidjson::StringRef(JSON_PASSWORD_PROPERTY_NAME), passwordValue, allocator);

	return domainProfileValue;
}

std::unique_ptr<NamecheapDomainProfile> NamecheapDomainProfile::parseFrom(const rapidjson::Value & domainProfileValue) {
	if(!domainProfileValue.IsObject()) {
		spdlog::error("Invalid Namecheap domain profile type: '{}', expected 'object'.", Utilities::typeToString(domainProfileValue.GetType()));
		return nullptr;
	}

	// check for unhandled Namecheap domain profile properties
	bool propertyHandled = false;

	for(rapidjson::Value::ConstMemberIterator i = domainProfileValue.MemberBegin(); i != domainProfileValue.MemberEnd(); ++i) {
		propertyHandled = false;

		for(const std::string_view propertyName : JSON_PROPERTY_NAMES) {
			if(i->name.GetString() == propertyName) {
				propertyHandled = true;
				break;
			}
		}

		if(!propertyHandled) {
			spdlog::warn("Namecheap domain profile has unexpected property '{}'.", i->name.GetString());
		}
	}

	// parse domain profile host(s)
	std::vector<std::string> hosts;

	if(domainProfileValue.HasMember(JSON_HOST_PROPERTY_NAME)) {
		const rapidjson::Value & hostValue = domainProfileValue[JSON_HOST_PROPERTY_NAME];

		if(!hostValue.IsString()) {
			spdlog::error("Invalid Namecheap domain profile '{}' property type: '{}', expected: 'string'.", JSON_HOST_PROPERTY_NAME, Utilities::typeToString(hostValue.GetType()));
			return nullptr;
		}

		hosts.emplace_back(Utilities::trimString(hostValue.GetString()));
	}
	else if(domainProfileValue.HasMember(JSON_HOSTS_PROPERTY_NAME)) {
		const rapidjson::Value & hostsValue = domainProfileValue[JSON_HOSTS_PROPERTY_NAME];

		if(!hostsValue.IsArray()) {
			spdlog::error("Invalid Namecheap domain profile '{}' property type: '{}', expected: 'array'.", JSON_HOSTS_PROPERTY_NAME, Utilities::typeToString(hostsValue.GetType()));
			return nullptr;
		}

		for(rapidjson::Value::ConstValueIterator i = hostsValue.Begin(); i != hostsValue.End(); ++i) {
			const rapidjson::Value & hostValue = *i;

			if(!hostValue.IsString()) {
				spdlog::error("Invalid Namecheap domain profile host #{} property type: '{}', expected: 'string'.", hosts.size() + 1, Utilities::typeToString(hostValue.GetType()));
				return nullptr;
			}

			hosts.emplace_back(Utilities::trimString(hostValue.GetString()));
		}
	}
	else {
		spdlog::error("Namecheap domain profile is missing '{}' or '{}' property.", JSON_HOST_PROPERTY_NAME, JSON_HOSTS_PROPERTY_NAME);
		return nullptr;
	}

	if(hosts.empty()) {
		spdlog::error("Namecheap domain profile does not specify any hosts, at least one is required.");
		return nullptr;
	}

	for(const std::string & host : hosts) {
		if(host.empty()) {
			spdlog::error("Namecheap domain profile host #{} is empty, expected non-empty string.");
			return nullptr;
		}
	}

	// parse domain profile domain
	std::string domain;

	if(domainProfileValue.HasMember(JSON_DOMAIN_PROPERTY_NAME)) {
		const rapidjson::Value & domainValue = domainProfileValue[JSON_DOMAIN_PROPERTY_NAME];

		if(!domainValue.IsString()) {
			spdlog::error("Invalid Namecheap domain profile '{}' property type: '{}', expected: 'string'.", JSON_DOMAIN_PROPERTY_NAME, Utilities::typeToString(domainValue.GetType()));
			return nullptr;
		}

		domain = Utilities::trimString(domainValue.GetString());
	}
	else {
		spdlog::error("Namecheap domain profile is missing '{}' property.", JSON_DOMAIN_PROPERTY_NAME);
		return nullptr;
	}

	// parse domain profile password
	std::string password;

	if(domainProfileValue.HasMember(JSON_PASSWORD_PROPERTY_NAME)) {
		const rapidjson::Value & passwordValue = domainProfileValue[JSON_PASSWORD_PROPERTY_NAME];

		if(!passwordValue.IsString()) {
			spdlog::error("Invalid Namecheap domain profile '{}' property type: '{}', expected: 'string'.", JSON_PASSWORD_PROPERTY_NAME, Utilities::typeToString(passwordValue.GetType()));
			return nullptr;
		}

		password = Utilities::trimString(passwordValue.GetString());
	}
	else {
		spdlog::error("Namecheap domain profile is missing '{}' property.", JSON_PASSWORD_PROPERTY_NAME);
		return nullptr;
	}

	return std::make_unique<NamecheapDomainProfile>(std::move(hosts), std::move(domain), std::move(password));
}

std::vector<std::unique_ptr<NamecheapDomainProfile>> parseFromList(const rapidjson::Value & domainProfileListValue) {
	if(!domainProfileListValue.IsArray()) {
		spdlog::error("Invalid Namecheap domain profile list type: '{}', expected 'array'.", Utilities::typeToString(domainProfileListValue.GetType()));
		return {};
	}

	std::unique_ptr<NamecheapDomainProfile> domainProfile;
	std::vector<std::unique_ptr<NamecheapDomainProfile>> domainProfileList;

	for(rapidjson::Value::ConstValueIterator i = domainProfileListValue.Begin(); i != domainProfileListValue.End(); ++i) {
		const rapidjson::Value & domainProfileValue = *i;

		domainProfile = NamecheapDomainProfile::parseFrom(domainProfileValue);

		if(domainProfile == nullptr) {
			spdlog::error("Failed to parse Namecheap domain profile list entry #{}.", domainProfileList.size() + 1);
			return {};
		}

		domainProfileList.emplace_back(std::move(domainProfile));
	}

	return domainProfileList;
}

bool NamecheapDomainProfile::isValid() const {
	for(const std::string & host : m_hosts) {
		if(host.empty()) {
			return false;
		}
	}

	return !m_hosts.empty() &&
		   !m_domain.empty() &&
		   !m_password.empty();
}

bool NamecheapDomainProfile::isValid(const NamecheapDomainProfile * domainProfile) {
	return domainProfile != nullptr &&
		   domainProfile->isValid();
}

bool NamecheapDomainProfile::operator == (const NamecheapDomainProfile & domainProfile) const {
	if(m_hosts.size() != domainProfile.m_hosts.size()) {
		return false;
	}

	for(size_t i = 0; i < m_hosts.size(); i++) {
		if(!Utilities::areStringsEqual(m_hosts[i], domainProfile.m_hosts[i])) {
			return false;
		}
	}

	return Utilities::areStringsEqual(m_domain, domainProfile.m_domain) &&
		   Utilities::areStringsEqual(m_password, domainProfile.m_password);
}

bool NamecheapDomainProfile::operator != (const NamecheapDomainProfile & domainProfile) const {
	return !operator == (domainProfile);
}
