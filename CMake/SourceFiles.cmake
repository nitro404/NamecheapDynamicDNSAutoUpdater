include_guard()

set(MAIN_SOURCE_FILES
	Application/NamecheapDynamicDNSAutoUpdater.h
	Application/NamecheapDynamicDNSAutoUpdater.cpp
	Application/SettingsManager.h
	Application/SettingsManager.cpp
	Namecheap/NamecheapDomainProfile.h
	Namecheap/NamecheapDomainProfile.cpp
	Namecheap/NamecheapDomainProfileCollection.h
	Namecheap/NamecheapDomainProfileCollection.cpp
	Namecheap/NamecheapDomainProfileManager.h
	Namecheap/NamecheapDomainProfileManager.cpp
	Namecheap/NamecheapDynamicDNSService.h
	Namecheap/NamecheapDynamicDNSService.cpp
	Main.cpp
	Project.h
)

list(APPEND SOURCE_FILES ${MAIN_SOURCE_FILES} ${MAIN_SOURCE_FILES_${PLATFORM_UPPER}})

list(APPEND SOURCE_FILES ${GUI_SOURCE_FILES})

list(TRANSFORM SOURCE_FILES PREPEND "${_SOURCE_DIRECTORY}/")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/${_SOURCE_DIRECTORY}" PREFIX "Source Files" FILES ${SOURCE_FILES})
