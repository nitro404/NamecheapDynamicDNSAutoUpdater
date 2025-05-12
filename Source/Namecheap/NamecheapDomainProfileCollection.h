#ifndef _NAMECHEAP_DOMAIN_PROFILE_COLLECTION_H_
#define _NAMECHEAP_DOMAIN_PROFILE_COLLECTION_H_

#include "NamecheapDomainProfile.h"

#include <rapidjson/document.h>

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class NamecheapDomainProfileCollection final {
public:
	NamecheapDomainProfileCollection();
	NamecheapDomainProfileCollection(std::vector<std::shared_ptr<NamecheapDomainProfile>> && domainProfiles);
	NamecheapDomainProfileCollection(const std::vector<std::shared_ptr<NamecheapDomainProfile>> & domainProfiles);
	NamecheapDomainProfileCollection(NamecheapDomainProfileCollection && domainProfiles) noexcept;
	NamecheapDomainProfileCollection(const NamecheapDomainProfileCollection & domainProfiles);
	NamecheapDomainProfileCollection & operator = (NamecheapDomainProfileCollection && domainProfiles) noexcept;
	NamecheapDomainProfileCollection & operator = (const NamecheapDomainProfileCollection & domainProfiles);
	~NamecheapDomainProfileCollection();

	size_t numberOfDomainProfiles() const;
	bool hasDomainProfile(const NamecheapDomainProfile & domainProfile) const;
	bool hasDomainProfile(std::string_view domain) const;
	size_t indexOfDomainProfile(const NamecheapDomainProfile & domainProfile) const;
	size_t indexOfDomainProfile(std::string_view domain) const;
	std::shared_ptr<NamecheapDomainProfile> getDomainProfile(size_t index) const;
	std::shared_ptr<NamecheapDomainProfile> getDomainProfileWithID(std::string_view domain) const;
	const std::vector<std::shared_ptr<NamecheapDomainProfile>> & getDomainProfiles() const;
	std::vector<std::string> getDomains() const;
	bool addDomainProfile(const NamecheapDomainProfile & domainProfile);
	bool addDomainProfile(std::shared_ptr<NamecheapDomainProfile> domainProfile);
	size_t addDomainProfiles(const std::vector<NamecheapDomainProfile> & domainProfiles);
	size_t addDomainProfiles(const std::vector<const NamecheapDomainProfile *> & domainProfiles);
	size_t addDomainProfiles(const std::vector<std::shared_ptr<NamecheapDomainProfile>> & domainProfiles);
	bool removeDomainProfile(size_t index);
	bool removeDomainProfile(const NamecheapDomainProfile & domainProfile);
	bool removeDomainProfileWithID(std::string_view domain);
	void clearDomainProfiles();

	rapidjson::Document toJSON() const;
	static std::unique_ptr<NamecheapDomainProfileCollection> parseFrom(const rapidjson::Value & domainProfileCollection);
	size_t loadFrom(const std::vector<std::string> & filePaths, bool mergeWithExisting = false);
	bool loadFrom(const std::string & filePath, bool mergeWithExisting = false);
	bool loadFromJSON(const std::string & filePath, bool mergeWithExisting = false);
	bool saveTo(const std::string & filePath, bool overwrite = true) const;
	bool saveToJSON(const std::string & filePath, bool overwrite = true) const;

	bool isValid() const;
	static bool isValid(const NamecheapDomainProfileCollection * domainProfiles);

	bool operator == (const NamecheapDomainProfileCollection & domainProfiles) const;
	bool operator != (const NamecheapDomainProfileCollection & domainProfiles) const;

	static const std::string FILE_TYPE;
	static const uint32_t FILE_FORMAT_VERSION;

private:
	std::vector<std::shared_ptr<NamecheapDomainProfile>> m_domainProfiles;
};

#endif // _NAMECHEAP_DOMAIN_PROFILE_COLLECTION_H_
