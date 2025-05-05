#include "Application/NamecheapDynamicDNSAutoUpdater.h"

#include <Application/ComponentRegistry.h>

int main(int argc, char * argv[]) {
	ComponentRegistry::getInstance().registerGlobalComponents();

	NamecheapDynamicDNSAutoUpdater application;

	if(!application.initialize(argc, argv)) {
		return 1;
	}

	bool result = application.run();

	application.uninitialize();

	ComponentRegistry::getInstance().deleteAllGlobalComponents();

	return result ? 0 : 1;
}
