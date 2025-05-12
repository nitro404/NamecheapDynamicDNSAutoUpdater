#include "NamecheapDomainProfileCollection.h"

#include <Utilities/FileUtilities.h>
#include <Utilities/RapidJSONUtilities.h>
#include <Utilities/StringUtilities.h>

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <fstream>

static constexpr const char * JSON_FILE_TYPE_PROPERTY_NAME = "fileType";
static constexpr const char * JSON_FILE_FORMAT_VERSION_PROPERTY_NAME = "fileFormatVersion";
static constexpr const char * JSON_DOMAIN_PROFILE_PROPERTY_NAME = "profile";
static constexpr const char * JSON_DOMAIN_PROFILES_PROPERTY_NAME = "profiles";
static const std::array<std::string_view, 4> JSON_PROPERTY_NAMES({
	JSON_FILE_TYPE_PROPERTY_NAME,
	JSON_FILE_FORMAT_VERSION_PROPERTY_NAME,
	JSON_DOMAIN_PROFILE_PROPERTY_NAME,
	JSON_DOMAIN_PROFILES_PROPERTY_NAME
});

const std::string NamecheapDomainProfileCollection::FILE_TYPE = "Namecheap Domain Profile";
const uint32_t NamecheapDomainProfileCollection::FILE_FORMAT_VERSION = 1;

NamecheapDomainProfileCollection::NamecheapDomainProfileCollection() { }

NamecheapDomainProfileCollection::NamecheapDomainProfileCollection(std::vector<std::shared_ptr<NamecheapDomainProfile>> && domainProfiles)
	: m_domainProfiles(std::move(domainProfiles))
{
}

NamecheapDomainProfileCollection::NamecheapDomainProfileCollection(const std::vector<std::shared_ptr<NamecheapDomainProfile>> & domainProfiles)
	: m_domainProfiles(domainProfiles)
{
}

NamecheapDomainProfileCollection::NamecheapDomainProfileCollection(NamecheapDomainProfileCollection && domainProfiles) noexcept
	: m_domainProfiles(std::move(domainProfiles.m_domainProfiles))
{
}

NamecheapDomainProfileCollection::NamecheapDomainProfileCollection(const NamecheapDomainProfileCollection & domainProfiles)
	: m_domainProfiles(domainProfiles.m_domainProfiles)
{
}

NamecheapDomainProfileCollection & NamecheapDomainProfileCollection::operator = (NamecheapDomainProfileCollection && domainProfiles) noexcept
{
	if(this != &domainProfiles) {
		m_domainProfiles = std::move(domainProfiles.m_domainProfiles);
	}

	return *this;
}

NamecheapDomainProfileCollection & NamecheapDomainProfileCollection::operator = (const NamecheapDomainProfileCollection & domainProfiles)
{
	m_domainProfiles = domainProfiles.m_domainProfiles;

	return *this;
}

NamecheapDomainProfileCollection::~NamecheapDomainProfileCollection() = default;

size_t NamecheapDomainProfileCollection::numberOfDomainProfiles() const {
	return m_domainProfiles.size();
}

bool NamecheapDomainProfileCollection::hasDomainProfile(const NamecheapDomainProfile & domainProfile) const {
	return indexOfDomainProfile(domainProfile) != std::numeric_limits<size_t>::max();
}

bool NamecheapDomainProfileCollection::hasDomainProfile(std::string_view domain) const {
	return indexOfDomainProfile(domain) != std::numeric_limits<size_t>::max();
}

size_t NamecheapDomainProfileCollection::indexOfDomainProfile(const NamecheapDomainProfile & domainProfile) const {
	auto domainProfileIterator = std::find_if(m_domainProfiles.cbegin(), m_domainProfiles.cend(), [&domainProfile](const std::shared_ptr<NamecheapDomainProfile> & currentDomainProfile) {
		return &domainProfile == currentDomainProfile.get();
	});

	if(domainProfileIterator == m_domainProfiles.cend()) {
		return std::numeric_limits<size_t>::max();
	}

	return domainProfileIterator - m_domainProfiles.cbegin();
}

size_t NamecheapDomainProfileCollection::indexOfDomainProfile(std::string_view domain) const {
	if(domain.empty()) {
		return std::numeric_limits<size_t>::max();
	}

	auto domainProfileIterator = std::find_if(m_domainProfiles.cbegin(), m_domainProfiles.cend(), [&domain](const std::shared_ptr<NamecheapDomainProfile> & currentDomainProfile) {
		return Utilities::areStringsEqualIgnoreCase(domain, currentDomainProfile->getDomain());
	});

	if(domainProfileIterator == m_domainProfiles.cend()) {
		return std::numeric_limits<size_t>::max();
	}

	return domainProfileIterator - m_domainProfiles.cbegin();
}

std::shared_ptr<NamecheapDomainProfile> NamecheapDomainProfileCollection::getDomainProfile(size_t index) const {
	if(index >= m_domainProfiles.size()) {
		return nullptr;
	}

	return m_domainProfiles[index];
}

std::shared_ptr<NamecheapDomainProfile> NamecheapDomainProfileCollection::getDomainProfileWithID(std::string_view domain) const {
	return getDomainProfile(indexOfDomainProfile(domain));
}

const std::vector<std::shared_ptr<NamecheapDomainProfile>> & NamecheapDomainProfileCollection::getDomainProfiles() const {
	return m_domainProfiles;
}

std::vector<std::string> NamecheapDomainProfileCollection::getDomains() const {
	std::vector<std::string> domains;

	for(size_t i = 0; i < m_domainProfiles.size(); i++) {
		domains.push_back(m_domainProfiles[i]->getDomain());
	}

	return domains;
}

bool NamecheapDomainProfileCollection::addDomainProfile(const NamecheapDomainProfile & domainProfile) {
	if(!domainProfile.isValid() || hasDomainProfile(domainProfile.getDomain())) {
		return false;
	}

	m_domainProfiles.push_back(std::make_shared<NamecheapDomainProfile>(domainProfile));

	return true;
}

bool NamecheapDomainProfileCollection::addDomainProfile(std::shared_ptr<NamecheapDomainProfile> domainProfile) {
	if(!NamecheapDomainProfile::isValid(domainProfile.get()) || hasDomainProfile(domainProfile->getDomain())) {
		return false;
	}

	m_domainProfiles.push_back(domainProfile);

	return true;
}

size_t NamecheapDomainProfileCollection::addDomainProfiles(const std::vector<NamecheapDomainProfile> & domainProfiles) {
	size_t numberOfDomainProfilesAdded = 0;

	for(std::vector<NamecheapDomainProfile>::const_iterator i = domainProfiles.begin(); i != domainProfiles.end(); ++i) {
		if(addDomainProfile(*i)) {
			numberOfDomainProfilesAdded++;
		}
	}

	return numberOfDomainProfilesAdded;
}

size_t NamecheapDomainProfileCollection::addDomainProfiles(const std::vector<const NamecheapDomainProfile *> & domainProfiles) {
	size_t numberOfDomainProfilesAdded = 0;

	for(std::vector<const NamecheapDomainProfile *>::const_iterator i = domainProfiles.begin(); i != domainProfiles.end(); ++i) {
		if(addDomainProfile(**i)) {
			numberOfDomainProfilesAdded++;
		}
	}

	return numberOfDomainProfilesAdded;
}

size_t NamecheapDomainProfileCollection::addDomainProfiles(const std::vector<std::shared_ptr<NamecheapDomainProfile>> & domainProfiles) {
	size_t numberOfDomainProfilesAdded = 0;

	for(std::vector<std::shared_ptr<NamecheapDomainProfile>>::const_iterator i = domainProfiles.begin(); i != domainProfiles.end(); ++i) {
		if(*i == nullptr) {
			continue;
		}

		if(addDomainProfile(*i)) {
			numberOfDomainProfilesAdded++;
		}
	}

	return numberOfDomainProfilesAdded;
}

bool NamecheapDomainProfileCollection::removeDomainProfile(size_t index) {
	if(index >= m_domainProfiles.size()) {
		return false;
	}

	m_domainProfiles.erase(m_domainProfiles.begin() + index);

	return true;
}

bool NamecheapDomainProfileCollection::removeDomainProfile(const NamecheapDomainProfile & domainProfile) {
	return removeDomainProfile(indexOfDomainProfile(domainProfile));
}

bool NamecheapDomainProfileCollection::removeDomainProfileWithID(std::string_view domain) {
	return removeDomainProfile(indexOfDomainProfile(domain));
}

void NamecheapDomainProfileCollection::clearDomainProfiles() {
	m_domainProfiles.clear();
}

rapidjson::Document NamecheapDomainProfileCollection::toJSON() const {
	rapidjson::Document domainProfileCollectionDocument(rapidjson::kObjectType);
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator = domainProfileCollectionDocument.GetAllocator();

	rapidjson::Value fileTypeValue(FILE_TYPE.c_str(), allocator);
	domainProfileCollectionDocument.AddMember(rapidjson::StringRef(JSON_FILE_TYPE_PROPERTY_NAME), fileTypeValue, allocator);

	domainProfileCollectionDocument.AddMember(rapidjson::StringRef(JSON_FILE_FORMAT_VERSION_PROPERTY_NAME), rapidjson::Value(FILE_FORMAT_VERSION), allocator);

	if(m_domainProfiles.size() == 1) {
		rapidjson::Value domainProfileValue(m_domainProfiles.front()->toJSON(allocator));
		domainProfileCollectionDocument.AddMember(rapidjson::StringRef(JSON_DOMAIN_PROFILE_PROPERTY_NAME), domainProfileValue, allocator);
	}
	else {
		rapidjson::Value domainProfilesValue(rapidjson::kArrayType);
		domainProfilesValue.Reserve(m_domainProfiles.size(), allocator);

		for(const std::shared_ptr<NamecheapDomainProfile> & domainProfile : m_domainProfiles) {
			domainProfilesValue.PushBack(domainProfile->toJSON(allocator), allocator);
		}

		domainProfileCollectionDocument.AddMember(rapidjson::StringRef(JSON_DOMAIN_PROFILES_PROPERTY_NAME), domainProfilesValue, allocator);
	}

	return domainProfileCollectionDocument;
}

std::unique_ptr<NamecheapDomainProfileCollection> NamecheapDomainProfileCollection::parseFrom(const rapidjson::Value & domainProfileCollection) {
	if(!domainProfileCollection.IsObject()) {
		spdlog::error("Invalid Namecheap domain profile collection type: '{}', expected 'object'.", Utilities::typeToString(domainProfileCollection.GetType()));
		return nullptr;
	}

	// check for unhandled Namecheap domain profile collection properties
	bool propertyHandled = false;

	for(rapidjson::Value::ConstMemberIterator i = domainProfileCollection.MemberBegin(); i != domainProfileCollection.MemberEnd(); ++i) {
		propertyHandled = false;

		for(const std::string_view propertyName : JSON_PROPERTY_NAMES) {
			if(i->name.GetString() == propertyName) {
				propertyHandled = true;
				break;
			}
		}

		if(!propertyHandled) {
			spdlog::warn("Namecheap domain profile collection has unexpected property '{}'.", i->name.GetString());
		}
	}

	// verify file type
	if(domainProfileCollection.HasMember(JSON_FILE_TYPE_PROPERTY_NAME)) {
		const rapidjson::Value & fileTypeValue = domainProfileCollection[JSON_FILE_TYPE_PROPERTY_NAME];

		if(!fileTypeValue.IsString()) {
			spdlog::error("Invalid Namecheap domain profile collection file type type: '{}', expected: 'string'.", Utilities::typeToString(fileTypeValue.GetType()));
			return nullptr;
		}

		if(!Utilities::areStringsEqualIgnoreCase(fileTypeValue.GetString(), FILE_TYPE)) {
			spdlog::error("Incorrect Namecheap domain profile collection file type: '{}', expected: '{}'.", fileTypeValue.GetString(), FILE_TYPE);
			return nullptr;
		}
	}
	else {
		spdlog::warn("Namecheap domain profile collection JSON data is missing file type, and may fail to load correctly!");
	}

	// verify file format version
	if(domainProfileCollection.HasMember(JSON_FILE_FORMAT_VERSION_PROPERTY_NAME)) {
		const rapidjson::Value & fileFormatVersionValue = domainProfileCollection[JSON_FILE_FORMAT_VERSION_PROPERTY_NAME];

		if(!fileFormatVersionValue.IsUint()) {
			spdlog::error("Invalid Namecheap domain profile collection file format version type: '{}', expected unsigned integer 'number'.", Utilities::typeToString(fileFormatVersionValue.GetType()));
			return nullptr;
		}

		if(fileFormatVersionValue.GetUint() != FILE_FORMAT_VERSION) {
			spdlog::error("Unsupported Namecheap domain profile collection file format version: {}, only version {} is supported.", fileFormatVersionValue.GetUint(), FILE_FORMAT_VERSION);
			return nullptr;
		}
	}
	else {
		spdlog::warn("Namecheap domain profile collection JSON data is missing file format version, and may fail to load correctly!");
	}

	std::unique_ptr<NamecheapDomainProfileCollection> newDomainProfilesCollection(std::make_unique<NamecheapDomainProfileCollection>());

	// read domain profile(s)
	if(domainProfileCollection.HasMember(JSON_DOMAIN_PROFILE_PROPERTY_NAME)) {
		const rapidjson::Value & domainProfileValue = domainProfileCollection[JSON_DOMAIN_PROFILE_PROPERTY_NAME];

		std::unique_ptr<NamecheapDomainProfile> newDomainProfile(NamecheapDomainProfile::parseFrom(domainProfileValue));

		if(!NamecheapDomainProfile::isValid(newDomainProfile.get())) {
			spdlog::error("Failed to paese Namecheap domain profile.");
			return nullptr;
		}

		if(!newDomainProfilesCollection->addDomainProfile(std::move(newDomainProfile))) {
			spdlog::error("Failed to add Namecheap domain profile to collection.");
			return nullptr;
		}
	}
	else if(domainProfileCollection.HasMember(JSON_DOMAIN_PROFILES_PROPERTY_NAME)) {
		const rapidjson::Value & domainProfilesValue = domainProfileCollection[JSON_DOMAIN_PROFILES_PROPERTY_NAME];

		if(!domainProfilesValue.IsArray()) {
			spdlog::error("Invalid Namecheap domain profile collection '{}' type: '{}', expected 'array'.", JSON_DOMAIN_PROFILES_PROPERTY_NAME, Utilities::typeToString(domainProfilesValue.GetType()));
			return nullptr;
		}

		if(domainProfilesValue.Empty()) {
			spdlog::error("Namecheap domain profile collection '{}' property cannot be empty.", JSON_DOMAIN_PROFILES_PROPERTY_NAME);
			return nullptr;
		}

		for(rapidjson::Value::ConstValueIterator i = domainProfilesValue.Begin(); i != domainProfilesValue.End(); ++i) {
			const rapidjson::Value & domainProfileValue = *i;

			std::unique_ptr<NamecheapDomainProfile> newDomainProfile(NamecheapDomainProfile::parseFrom(domainProfileValue));

			if(!NamecheapDomainProfile::isValid(newDomainProfile.get())) {
				spdlog::error("Failed to parse Namecheap domain profile #{}.", newDomainProfilesCollection->numberOfDomainProfiles() + 1);
				return nullptr;
			}

			if(!newDomainProfilesCollection->addDomainProfile(std::move(newDomainProfile))) {
				spdlog::error("Failed to add Namecheap domain profile #{} to collection.", newDomainProfilesCollection->numberOfDomainProfiles() + 1);
				return nullptr;
			}
		}
	}
	else {
		spdlog::error("Namecheap domain profile collection is missing '{}' or '{}' property.", JSON_DOMAIN_PROFILE_PROPERTY_NAME, JSON_DOMAIN_PROFILES_PROPERTY_NAME);
		return nullptr;
	}

	return newDomainProfilesCollection;
}

size_t NamecheapDomainProfileCollection::loadFrom(const std::vector<std::string> & filePaths, bool mergeWithExisting) {
	if(filePaths.empty()) {
		return 0;
	}

	size_t numberOfDomainProfilesLoaded = 0;

	for(size_t i = 0; i < filePaths.size(); i++) {
		const std::string & filePath = filePaths[i];

		if(loadFrom(filePath, mergeWithExisting)) {
			numberOfDomainProfilesLoaded++;
		}
		else {
			spdlog::error("Failed to load Namecheap domain profile from file path: '{}'.", filePath);
		}
	}

	return numberOfDomainProfilesLoaded;
}

bool NamecheapDomainProfileCollection::loadFrom(const std::string & filePath, bool mergeWithExisting) {
	if(filePath.empty()) {
		return false;
	}

	std::string fileExtension(Utilities::getFileExtension(filePath));

	if(fileExtension.empty()) {
		return false;
	}
	else if(Utilities::areStringsEqualIgnoreCase(fileExtension, "json")) {
		if(!loadFromJSON(filePath, mergeWithExisting)) {
			return false;
		}
	}

	return true;
}

bool NamecheapDomainProfileCollection::loadFromJSON(const std::string & filePath, bool mergeWithExisting) {
	if(filePath.empty()) {
		return false;
	}

	if(!std::filesystem::is_regular_file(std::filesystem::path(filePath))) {
		return false;
	}

	std::ifstream fileStream(filePath);

	if(!fileStream.is_open()) {
		return false;
	}

	rapidjson::Document domainProfilesValue;
	rapidjson::IStreamWrapper fileStreamWrapper(fileStream);
	domainProfilesValue.ParseStream(fileStreamWrapper);

	fileStream.close();

	std::unique_ptr<NamecheapDomainProfileCollection> domainProfiles(parseFrom(domainProfilesValue));

	if(!NamecheapDomainProfileCollection::isValid(domainProfiles.get())) {
		spdlog::error("Failed to parse Namecheap domain profile collection from JSON file '{}'.", filePath);
		return false;
	}

	if(mergeWithExisting) {
		if(!addDomainProfiles(domainProfiles->m_domainProfiles)) {
			spdlog::error("Failed to add one or more Namecheap domain profiles when merging with existing ones. Did you make sure there are no duplicated domains?");
			return false;
		}
	}
	else {
		m_domainProfiles = std::move(domainProfiles->m_domainProfiles);
	}

	return true;
}

bool NamecheapDomainProfileCollection::saveTo(const std::string & filePath, bool overwrite) const {
	if(filePath.empty()) {
		return false;
	}

	std::string fileExtension(Utilities::getFileExtension(filePath));

	if(fileExtension.empty()) {
		return false;
	}
	else if(Utilities::areStringsEqualIgnoreCase(fileExtension, "json")) {
		return saveToJSON(filePath, overwrite);
	}

	return false;
}

bool NamecheapDomainProfileCollection::saveToJSON(const std::string & filePath, bool overwrite) const {
	if (!overwrite && std::filesystem::exists(std::filesystem::path(filePath))) {
		spdlog::warn("File '{}' already exists, use overwrite to force write.", filePath);
		return false;
	}

	std::ofstream fileStream(filePath);

	if(!fileStream.is_open()) {
		return false;
	}

	rapidjson::Document domainProfiles(toJSON());

	rapidjson::OStreamWrapper fileStreamWrapper(fileStream);
	rapidjson::PrettyWriter<rapidjson::OStreamWrapper> fileStreamWriter(fileStreamWrapper);
	fileStreamWriter.SetIndent('\t', 1);
	domainProfiles.Accept(fileStreamWriter);
	fileStream.close();

	return true;
}

bool NamecheapDomainProfileCollection::isValid() const {
	for(std::vector<std::shared_ptr<NamecheapDomainProfile>>::const_iterator i = m_domainProfiles.begin(); i != m_domainProfiles.end(); ++i) {
		if(!(*i)->isValid()) {
			return false;
		}

		for(std::vector<std::shared_ptr<NamecheapDomainProfile>>::const_iterator j = i + 1; j != m_domainProfiles.end(); ++j) {
			if(Utilities::areStringsEqualIgnoreCase((*i)->getDomain(), (*j)->getDomain())) {
				return false;
			}
		}
	}

	return true;
}

bool NamecheapDomainProfileCollection::isValid(const NamecheapDomainProfileCollection * domainProfiles) {
	return domainProfiles != nullptr &&
		   domainProfiles->isValid();
}

bool NamecheapDomainProfileCollection::operator == (const NamecheapDomainProfileCollection & domainProfiles) const {
	if(m_domainProfiles.size() != domainProfiles.m_domainProfiles.size()) {
		return false;
	}

	for(size_t i = 0; i < domainProfiles.m_domainProfiles.size(); i++) {
		if(*m_domainProfiles[i] != *domainProfiles.m_domainProfiles[i]) {
			return false;
		}
	}

	return true;
}

bool NamecheapDomainProfileCollection::operator != (const NamecheapDomainProfileCollection & domainProfiles) const {
	return !operator == (domainProfiles);
}
