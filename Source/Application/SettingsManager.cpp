#include "SettingsManager.h"

#include <Arguments/ArgumentParser.h>
#include <Logging/LogSystem.h>
#include <Utilities/RapidJSONUtilities.h>
#include <Utilities/StringUtilities.h>
#include <Utilities/TimeUtilities.h>

#include <magic_enum.hpp>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

using namespace std::chrono_literals;

static constexpr const char * DOWNLOADS_LIST_FILE_PATH = "downloadsListFilePath";
static constexpr const char * DIRECTORY_PATH = "directoryPath";
static constexpr const char * DATA_DIRECTORY_NAME = "dataDirectoryName";

static constexpr const char * FILE_TYPE_PROPERTY_NAME = "fileType";
static constexpr const char * FILE_FORMAT_VERSION_PROPERTY_NAME = "fileFormatVersion";
static constexpr const char * LOG_LEVEL_PROPERTY_NAME = "logLevel";
static constexpr const char * DATA_DIRECTORY_PATH_PROPERTY_NAME = "dataDirectoryPath";

static constexpr const char * DOWNLOADS_CATEGORY_NAME = "downloads";
static constexpr const char * DOWNLOADS_DIRECTORY_PATH_PROPERTY_NAME = DIRECTORY_PATH;

static constexpr const char * TIME_ZONE_CATEGORY_NAME = "timeZone";
static constexpr const char * TIME_ZONE_DATA_DIRECTORY_NAME_PROPERTY_NAME = DATA_DIRECTORY_NAME;

static constexpr const char * CURL_CATEGORY_NAME = "curl";
static constexpr const char * CURL_DATA_DIRECTORY_NAME_PROPERTY_NAME = DATA_DIRECTORY_NAME;
static constexpr const char * CURL_CONNECTION_TIMEOUT_PROPERTY_NAME = "connectionTimeout";
static constexpr const char * CURL_NETWORK_TIMEOUT_PROPERTY_NAME = "networkTimeout";
static constexpr const char * CURL_TRANSFER_TIMEOUT_PROPERTY_NAME = "transferTimeout";
static constexpr const char * CURL_VERBOSE_REQUEST_LOGGING_PROPERTY_NAME = "verboseRequestLogging";

static constexpr const char * FILE_ETAGS_PROPERTY_NAME = "fileETags";

static constexpr const char * DOWNLOAD_THROTTLING_CATEGORY_NAME = "downloadThrottling";
static constexpr const char * DOWNLOAD_THROTTLING_ENABLED_PROPERTY_NAME = "enabled";
static constexpr const char * CACERT_LAST_DOWNLOADED_PROPERTY_NAME = "cacertLastDownloaded";
static constexpr const char * CACERT_UPDATE_FREQUENCY_PROPERTY_NAME = "cacertUpdateFrequency";
static constexpr const char * TIME_ZONE_DATA_LAST_DOWNLOADED_PROPERTY_NAME = "timeZoneDataLastDownloaded";
static constexpr const char * TIME_ZONE_DATA_UPDATE_FREQUENCY_PROPERTY_NAME = "timeZoneDataUpdateFrequency";

const std::string SettingsManager::FILE_TYPE("Namecheap Dynamic DNS Auto-Updater Settings");
const uint32_t SettingsManager::FILE_FORMAT_VERSION = 1;
const std::string SettingsManager::DEFAULT_SETTINGS_FILE_PATH("Namecheap Dynamic DNS Auto-Updater Settings.json");
const std::string SettingsManager::DEFAULT_DOWNLOADS_DIRECTORY_PATH("Downloads");
const std::string SettingsManager::DEFAULT_DATA_DIRECTORY_PATH("Data");
const std::string SettingsManager::DEFAULT_TIME_ZONE_DATA_DIRECTORY_NAME("Time Zone");
const std::string SettingsManager::DEFAULT_CURL_DATA_DIRECTORY_NAME("cURL");
const std::chrono::seconds SettingsManager::DEFAULT_CONNECTION_TIMEOUT = 15s;
const std::chrono::seconds SettingsManager::DEFAULT_NETWORK_TIMEOUT = 30s;
const std::chrono::seconds SettingsManager::DEFAULT_TRANSFER_TIMEOUT = 0s;
const bool SettingsManager::DEFAULT_VERBOSE_REQUEST_LOGGING = false;
const bool SettingsManager::DEFAULT_DOWNLOAD_THROTTLING_ENABLED = true;
const std::chrono::minutes SettingsManager::DEFAULT_CACERT_UPDATE_FREQUENCY = std::chrono::hours(2 * 24 * 7); // 2 weeks
const std::chrono::minutes SettingsManager::DEFAULT_TIME_ZONE_DATA_UPDATE_FREQUENCY = std::chrono::hours(1 * 24 * 7); // 1 week

static bool assignStringSetting(std::string & setting, const rapidjson::Value & categoryValue, const std::string & propertyName) {
	if(propertyName.empty() || !categoryValue.IsObject() || !categoryValue.HasMember(propertyName.c_str())) {
		return false;
	}

	const rapidjson::Value & settingValue = categoryValue[propertyName.c_str()];

	if(!settingValue.IsString()) {
		return false;
	}

	setting = settingValue.GetString();

	return true;
}

static bool assignBooleanSetting(bool & setting, const rapidjson::Value & categoryValue, const std::string & propertyName) {
	if(propertyName.empty() || !categoryValue.IsObject() || !categoryValue.HasMember(propertyName.c_str())) {
		return false;
	}

	const rapidjson::Value & settingValue = categoryValue[propertyName.c_str()];

	if(!settingValue.IsBool()) {
		return false;
	}

	setting = settingValue.GetBool();

	return true;
}

static bool assignOptionalTimePointSetting(std::optional<std::chrono::time_point<std::chrono::system_clock>> & setting, const rapidjson::Value & categoryValue, const std::string & propertyName) {
	if(propertyName.empty() || !categoryValue.IsObject() || !categoryValue.HasMember(propertyName.c_str())) {
		return false;
	}

	const rapidjson::Value & settingValue = categoryValue[propertyName.c_str()];

	if(!settingValue.IsString()) {
		return false;
	}

	setting = Utilities::parseTimePointFromString(settingValue.GetString());

	return setting.has_value();
}

template <typename T>
static bool assignChronoSetting(T & setting, const rapidjson::Value & categoryValue, const std::string & propertyName) {
	if(propertyName.empty() || !categoryValue.IsObject() || !categoryValue.HasMember(propertyName.c_str())) {
		return false;
	}

	const rapidjson::Value & settingValue = categoryValue[propertyName.c_str()];

	if(!settingValue.IsUint64()) {
		return false;
	}

	setting = T(settingValue.GetUint64());

	return true;
}

SettingsManager::SettingsManager()
	: downloadsDirectoryPath(DEFAULT_DOWNLOADS_DIRECTORY_PATH)
	, dataDirectoryPath(DEFAULT_DATA_DIRECTORY_PATH)
	, timeZoneDataDirectoryName(DEFAULT_TIME_ZONE_DATA_DIRECTORY_NAME)
	, curlDataDirectoryName(DEFAULT_CURL_DATA_DIRECTORY_NAME)
	, connectionTimeout(DEFAULT_CONNECTION_TIMEOUT)
	, networkTimeout(DEFAULT_NETWORK_TIMEOUT)
	, transferTimeout(DEFAULT_TRANSFER_TIMEOUT)
	, verboseRequestLogging(DEFAULT_VERBOSE_REQUEST_LOGGING)
	, downloadThrottlingEnabled(DEFAULT_DOWNLOAD_THROTTLING_ENABLED)
	, cacertUpdateFrequency(DEFAULT_CACERT_UPDATE_FREQUENCY)
	, timeZoneDataUpdateFrequency(DEFAULT_TIME_ZONE_DATA_UPDATE_FREQUENCY)
	, m_loaded(false)
	, m_filePath(DEFAULT_SETTINGS_FILE_PATH) { }

SettingsManager::~SettingsManager() = default;

void SettingsManager::reset() {
	downloadsDirectoryPath = DEFAULT_DOWNLOADS_DIRECTORY_PATH;
	dataDirectoryPath = DEFAULT_DATA_DIRECTORY_PATH;
	timeZoneDataDirectoryName = DEFAULT_TIME_ZONE_DATA_DIRECTORY_NAME;
	curlDataDirectoryName = DEFAULT_CURL_DATA_DIRECTORY_NAME;
	connectionTimeout = DEFAULT_CONNECTION_TIMEOUT;
	networkTimeout = DEFAULT_NETWORK_TIMEOUT;
	transferTimeout = DEFAULT_TRANSFER_TIMEOUT;
	verboseRequestLogging = DEFAULT_VERBOSE_REQUEST_LOGGING;
	downloadThrottlingEnabled = DEFAULT_DOWNLOAD_THROTTLING_ENABLED;
	cacertLastDownloadedTimestamp.reset();
	cacertUpdateFrequency = DEFAULT_CACERT_UPDATE_FREQUENCY;
	timeZoneDataLastDownloadedTimestamp.reset();
	timeZoneDataUpdateFrequency = DEFAULT_TIME_ZONE_DATA_UPDATE_FREQUENCY;
	fileETags.clear();
}

rapidjson::Document SettingsManager::toJSON() const {
	rapidjson::Document settingsDocument(rapidjson::kObjectType);
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> & allocator = settingsDocument.GetAllocator();

	rapidjson::Value fileTypeValue(FILE_TYPE.c_str(), allocator);
	settingsDocument.AddMember(rapidjson::StringRef(FILE_TYPE_PROPERTY_NAME), fileTypeValue, allocator);
	settingsDocument.AddMember(rapidjson::StringRef(FILE_FORMAT_VERSION_PROPERTY_NAME), rapidjson::Value(FILE_FORMAT_VERSION), allocator);
	rapidjson::Value logLevelValue(std::string(magic_enum::enum_name(LogSystem::getInstance()->getLevel())).c_str(), allocator);
	settingsDocument.AddMember(rapidjson::StringRef(LOG_LEVEL_PROPERTY_NAME), logLevelValue, allocator);
	rapidjson::Value dataDirectoryPathValue(dataDirectoryPath.c_str(), allocator);
	settingsDocument.AddMember(rapidjson::StringRef(DATA_DIRECTORY_PATH_PROPERTY_NAME), dataDirectoryPathValue, allocator);

	rapidjson::Value downloadsCategoryValue(rapidjson::kObjectType);

	rapidjson::Value downloadsDirectoryPathValue(downloadsDirectoryPath.c_str(), allocator);
	downloadsCategoryValue.AddMember(rapidjson::StringRef(DOWNLOADS_DIRECTORY_PATH_PROPERTY_NAME), downloadsDirectoryPathValue, allocator);

	settingsDocument.AddMember(rapidjson::StringRef(DOWNLOADS_CATEGORY_NAME), downloadsCategoryValue, allocator);

	rapidjson::Value timeZoneCategoryValue(rapidjson::kObjectType);

	rapidjson::Value timeZoneDataDirectoryNameValue(timeZoneDataDirectoryName.c_str(), allocator);
	timeZoneCategoryValue.AddMember(rapidjson::StringRef(TIME_ZONE_DATA_DIRECTORY_NAME_PROPERTY_NAME), timeZoneDataDirectoryNameValue, allocator);

	settingsDocument.AddMember(rapidjson::StringRef(TIME_ZONE_CATEGORY_NAME), timeZoneCategoryValue, allocator);

	rapidjson::Value curlCategoryValue(rapidjson::kObjectType);

	rapidjson::Value curlDataDirectoryNameValue(curlDataDirectoryName.c_str(), allocator);
	curlCategoryValue.AddMember(rapidjson::StringRef(CURL_DATA_DIRECTORY_NAME_PROPERTY_NAME), curlDataDirectoryNameValue, allocator);
	curlCategoryValue.AddMember(rapidjson::StringRef(CURL_CONNECTION_TIMEOUT_PROPERTY_NAME), rapidjson::Value(connectionTimeout.count()), allocator);
	curlCategoryValue.AddMember(rapidjson::StringRef(CURL_NETWORK_TIMEOUT_PROPERTY_NAME), rapidjson::Value(networkTimeout.count()), allocator);
	curlCategoryValue.AddMember(rapidjson::StringRef(CURL_TRANSFER_TIMEOUT_PROPERTY_NAME), rapidjson::Value(transferTimeout.count()), allocator);
	curlCategoryValue.AddMember(rapidjson::StringRef(CURL_VERBOSE_REQUEST_LOGGING_PROPERTY_NAME), rapidjson::Value(verboseRequestLogging), allocator);

	settingsDocument.AddMember(rapidjson::StringRef(CURL_CATEGORY_NAME), curlCategoryValue, allocator);

	rapidjson::Value downloadThrottlingCategoryValue(rapidjson::kObjectType);

	downloadThrottlingCategoryValue.AddMember(rapidjson::StringRef(DOWNLOAD_THROTTLING_ENABLED_PROPERTY_NAME), rapidjson::Value(downloadThrottlingEnabled), allocator);

	if(cacertLastDownloadedTimestamp.has_value()) {
		rapidjson::Value cacertLastDownloadedValue(Utilities::timePointToString(cacertLastDownloadedTimestamp.value(), Utilities::TimeFormat::ISO8601).c_str(), allocator);
		downloadThrottlingCategoryValue.AddMember(rapidjson::StringRef(CACERT_LAST_DOWNLOADED_PROPERTY_NAME), cacertLastDownloadedValue, allocator);
	}

	downloadThrottlingCategoryValue.AddMember(rapidjson::StringRef(CACERT_UPDATE_FREQUENCY_PROPERTY_NAME), rapidjson::Value(cacertUpdateFrequency.count()), allocator);

	if(timeZoneDataLastDownloadedTimestamp.has_value()) {
		rapidjson::Value timeZoneDataLastDownloadedValue(Utilities::timePointToString(timeZoneDataLastDownloadedTimestamp.value(), Utilities::TimeFormat::ISO8601).c_str(), allocator);
		downloadThrottlingCategoryValue.AddMember(rapidjson::StringRef(TIME_ZONE_DATA_LAST_DOWNLOADED_PROPERTY_NAME), timeZoneDataLastDownloadedValue, allocator);
	}

	downloadThrottlingCategoryValue.AddMember(rapidjson::StringRef(TIME_ZONE_DATA_UPDATE_FREQUENCY_PROPERTY_NAME), rapidjson::Value(timeZoneDataUpdateFrequency.count()), allocator);

	settingsDocument.AddMember(rapidjson::StringRef(DOWNLOAD_THROTTLING_CATEGORY_NAME), downloadThrottlingCategoryValue, allocator);

	rapidjson::Value fileETagsValue(rapidjson::kObjectType);

	for(std::map<std::string, std::string>::const_iterator i = fileETags.begin(); i != fileETags.end(); ++i) {
		rapidjson::Value fileNameValue(i->first.c_str(), allocator);
		rapidjson::Value fileETagValue(i->second.c_str(), allocator);
		fileETagsValue.AddMember(fileNameValue, fileETagValue, allocator);
	}

	settingsDocument.AddMember(rapidjson::StringRef(FILE_ETAGS_PROPERTY_NAME), fileETagsValue, allocator);

	return settingsDocument;
}

bool SettingsManager::parseFrom(const rapidjson::Value & settingsDocument) {
	if(!settingsDocument.IsObject()) {
		spdlog::error("Invalid settings value, expected object.");
		return false;
	}

	if(settingsDocument.HasMember(FILE_TYPE_PROPERTY_NAME)) {
		const rapidjson::Value & fileTypeValue = settingsDocument[FILE_TYPE_PROPERTY_NAME];

		if(!fileTypeValue.IsString()) {
			spdlog::error("Invalid settings file type type: '{}', expected: 'string'.", Utilities::typeToString(fileTypeValue.GetType()));
			return false;
		}

		if(!Utilities::areStringsEqualIgnoreCase(fileTypeValue.GetString(), FILE_TYPE)) {
			spdlog::error("Incorrect settings file type: '{}', expected: '{}'.", fileTypeValue.GetString(), FILE_TYPE);
			return false;
		}
	}
	else {
		spdlog::warn("Settings JSON data is missing file type, and may fail to load correctly!");
	}

	if(settingsDocument.HasMember(FILE_FORMAT_VERSION_PROPERTY_NAME)) {
		const rapidjson::Value & fileFormatVersionValue = settingsDocument[FILE_FORMAT_VERSION_PROPERTY_NAME];

		if(!fileFormatVersionValue.IsUint()) {
			spdlog::error("Invalid settings file format version type: '{}', expected unsigned integer 'number'.", Utilities::typeToString(fileFormatVersionValue.GetType()));
			return false;
		}

		if(fileFormatVersionValue.GetUint() != FILE_FORMAT_VERSION) {
			spdlog::error("Unsupported settings file format version: {}, only version {} is supported.", fileFormatVersionValue.GetUint(), FILE_FORMAT_VERSION);
			return false;
		}
	}
	else {
		spdlog::warn("Settings file is missing file format version, and may fail to load correctly!");
	}

	if(settingsDocument.HasMember(LOG_LEVEL_PROPERTY_NAME) && settingsDocument[LOG_LEVEL_PROPERTY_NAME].IsString()) {
		std::optional<spdlog::level::level_enum> optionalLogLevel(magic_enum::enum_cast<spdlog::level::level_enum>(settingsDocument[LOG_LEVEL_PROPERTY_NAME].GetString()));

		if(optionalLogLevel.has_value() && optionalLogLevel.value() != spdlog::level::n_levels) {
			LogSystem::getInstance()->setLevel(optionalLogLevel.value());
		}
	}

	assignStringSetting(dataDirectoryPath, settingsDocument, DATA_DIRECTORY_PATH_PROPERTY_NAME);

	if(settingsDocument.HasMember(DOWNLOADS_CATEGORY_NAME) && settingsDocument[DOWNLOADS_CATEGORY_NAME].IsObject()) {
		const rapidjson::Value & downloadsCategoryValue = settingsDocument[DOWNLOADS_CATEGORY_NAME];

		assignStringSetting(downloadsDirectoryPath, downloadsCategoryValue, DOWNLOADS_DIRECTORY_PATH_PROPERTY_NAME);
	}

	if(settingsDocument.HasMember(TIME_ZONE_CATEGORY_NAME) && settingsDocument[TIME_ZONE_CATEGORY_NAME].IsObject()) {
		const rapidjson::Value & timeZoneCategoryValue = settingsDocument[TIME_ZONE_CATEGORY_NAME];

		assignStringSetting(timeZoneDataDirectoryName, timeZoneCategoryValue, TIME_ZONE_DATA_DIRECTORY_NAME_PROPERTY_NAME);
	}

	if(settingsDocument.HasMember(CURL_CATEGORY_NAME) && settingsDocument[CURL_CATEGORY_NAME].IsObject()) {
		const rapidjson::Value & curlCategoryValue = settingsDocument[CURL_CATEGORY_NAME];

		assignStringSetting(curlDataDirectoryName, curlCategoryValue, CURL_DATA_DIRECTORY_NAME_PROPERTY_NAME);

		if(curlCategoryValue.HasMember(CURL_CONNECTION_TIMEOUT_PROPERTY_NAME) && curlCategoryValue[CURL_CONNECTION_TIMEOUT_PROPERTY_NAME].IsUint64()) {
			connectionTimeout = std::chrono::seconds(curlCategoryValue[CURL_CONNECTION_TIMEOUT_PROPERTY_NAME].GetUint64());
		}

		if(curlCategoryValue.HasMember(CURL_NETWORK_TIMEOUT_PROPERTY_NAME) && curlCategoryValue[CURL_NETWORK_TIMEOUT_PROPERTY_NAME].IsUint64()) {
			networkTimeout = std::chrono::seconds(curlCategoryValue[CURL_NETWORK_TIMEOUT_PROPERTY_NAME].GetUint64());
		}

		if(curlCategoryValue.HasMember(CURL_TRANSFER_TIMEOUT_PROPERTY_NAME) && curlCategoryValue[CURL_TRANSFER_TIMEOUT_PROPERTY_NAME].IsUint64()) {
			transferTimeout = std::chrono::seconds(curlCategoryValue[CURL_TRANSFER_TIMEOUT_PROPERTY_NAME].GetUint64());
		}

		if(curlCategoryValue.HasMember(CURL_VERBOSE_REQUEST_LOGGING_PROPERTY_NAME) && curlCategoryValue[CURL_VERBOSE_REQUEST_LOGGING_PROPERTY_NAME].IsBool()) {
			verboseRequestLogging = curlCategoryValue[CURL_VERBOSE_REQUEST_LOGGING_PROPERTY_NAME].GetBool();
		}
	}

	if(settingsDocument.HasMember(DOWNLOAD_THROTTLING_CATEGORY_NAME) && settingsDocument[DOWNLOAD_THROTTLING_CATEGORY_NAME].IsObject()) {
		const rapidjson::Value & downloadThrottlingValue = settingsDocument[DOWNLOAD_THROTTLING_CATEGORY_NAME];

		assignBooleanSetting(downloadThrottlingEnabled, downloadThrottlingValue, DOWNLOAD_THROTTLING_ENABLED_PROPERTY_NAME);
		assignOptionalTimePointSetting(cacertLastDownloadedTimestamp, downloadThrottlingValue, CACERT_LAST_DOWNLOADED_PROPERTY_NAME);
		assignChronoSetting(cacertUpdateFrequency, downloadThrottlingValue, CACERT_UPDATE_FREQUENCY_PROPERTY_NAME);
		assignOptionalTimePointSetting(timeZoneDataLastDownloadedTimestamp, downloadThrottlingValue, TIME_ZONE_DATA_LAST_DOWNLOADED_PROPERTY_NAME);
		assignChronoSetting(timeZoneDataUpdateFrequency, downloadThrottlingValue, TIME_ZONE_DATA_UPDATE_FREQUENCY_PROPERTY_NAME);
	}

	if(settingsDocument.HasMember(FILE_ETAGS_PROPERTY_NAME) && settingsDocument[FILE_ETAGS_PROPERTY_NAME].IsObject()) {
		const rapidjson::Value & fileETagsValue = settingsDocument[FILE_ETAGS_PROPERTY_NAME];

		for(rapidjson::Value::ConstMemberIterator i = fileETagsValue.MemberBegin(); i != fileETagsValue.MemberEnd(); ++i) {
			fileETags.emplace(i->name.GetString(), i->value.GetString());
		}
	}

	return true;
}

bool SettingsManager::isLoaded() const {
	return m_loaded;
}

bool SettingsManager::load(const ArgumentParser * arguments, bool autoCreate) {
	if(arguments != nullptr) {
		std::string alternateSettingsFilePath(arguments->getFirstValue("f"));

		if(alternateSettingsFilePath.empty()) {
			alternateSettingsFilePath = arguments->getFirstValue("file");
		}

		if(!alternateSettingsFilePath.empty()) {
			spdlog::debug("Loading settings from alternate file: '{}'...", alternateSettingsFilePath);

			m_filePath = alternateSettingsFilePath;
		}
	}

	return loadFrom(m_filePath, autoCreate);
}

bool SettingsManager::save(bool overwrite) const {
	if(!Utilities::areStringsEqual(m_filePath, DEFAULT_SETTINGS_FILE_PATH)) {
		spdlog::debug("Saving settings to alternate file: '{}'...", m_filePath);
	}

	return saveTo(m_filePath, overwrite);
}

bool SettingsManager::loadFrom(const std::string & filePath, bool autoCreate) {
	if(filePath.empty()) {
		spdlog::error("Settings file path cannot be empty!");
		return false;
	}

	if(!std::filesystem::is_regular_file(std::filesystem::path(filePath))) {
		spdlog::error("Failed to open missing or invalid settings file: '{}'!", filePath);

		if(autoCreate) {
			saveTo(filePath);
		}

		return false;
	}

	std::ifstream fileStream(filePath);

	if(!fileStream.is_open()) {
		spdlog::error("Failed to open settings file '{}' for parsing!", filePath);
		return false;
	}

	rapidjson::Document settings;
	rapidjson::IStreamWrapper fileStreamWrapper(fileStream);
	if(settings.ParseStream(fileStreamWrapper).HasParseError()) {
		spdlog::error("Failed to parse settings file JSON data!");
		return false;
	}

	fileStream.close();

	if(!parseFrom(settings)) {
		spdlog::error("Failed to parse settings from file '{}'!", filePath);
		return false;
	}

	spdlog::info("Settings successfully loaded from file '{}'.", filePath);

	m_loaded = true;

	return true;
}

bool SettingsManager::saveTo(const std::string & filePath, bool overwrite) const {
	if (!overwrite && std::filesystem::exists(std::filesystem::path(filePath))) {
		spdlog::warn("File '{}' already exists, use overwrite to force write.", filePath);
		return false;
	}

	std::ofstream fileStream(filePath);

	if(!fileStream.is_open()) {
		return false;
	}

	rapidjson::Document settings(toJSON());

	rapidjson::OStreamWrapper fileStreamWrapper(fileStream);
	rapidjson::PrettyWriter<rapidjson::OStreamWrapper> fileStreamWriter(fileStreamWrapper);
	fileStreamWriter.SetIndent('\t', 1);
	settings.Accept(fileStreamWriter);
	fileStream.close();

	spdlog::info("Settings successfully saved to file '{}'.", filePath);

	return true;
}
