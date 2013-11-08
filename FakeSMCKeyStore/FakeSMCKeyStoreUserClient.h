//
//  FakeSMCKeyStoreUserClient.h
//  HWSensors
//
//  Created by Kozlek on 02/11/13.
//
//

#ifndef __HWSensors__FakeSMCKeyStoreUserClient__
#define __HWSensors__FakeSMCKeyStoreUserClient__

#include <IOKit/IOUserClient.h>

class FakeSMCKeyStore;

class FakeSMCKeyStoreUserClient : public IOUserClient
{
	OSDeclareDefaultStructors(FakeSMCKeyStoreUserClient);

private:
	FakeSMCKeyStore *keyStore;

public:
	/* IOService overrides */
	virtual bool start(IOService* provider);
	virtual void stop(IOService* provider);

	/* IOUserClient overrides */
	virtual bool initWithTask(task_t task, void* securityID, UInt32 type,  OSDictionary* properties);
	virtual IOReturn clientClose(void);
	virtual IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* arguments,
									IOExternalMethodDispatch* dispatch, OSObject* target, void* reference);
};

#endif /* defined(__HWSensors__FakeSMCKeyStoreUserClient__) */
