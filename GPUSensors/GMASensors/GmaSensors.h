/*
 *  X3100.h
 *  HWSensors
 *
 *  Created by Sergey on 19.12.10 with templates of Natan Zalkin <natan.zalkin@me.com>.
 *  Copyright 2010 Slice.
 *
 */

#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "GPUSensors.h"

class EXPORT GmaSensors : public GPUSensors
{
    OSDeclareDefaultStructors(GmaSensors) 
    
private:
	OSDictionary *		sensors;
	volatile UInt8*     mmio_base;
	IOPCIDevice *		VCard;
	IOMemoryMap *		mmio;
	
    SInt8               gpuIndex;
    
protected:	
    virtual float       getSensorValue(FakeSMCSensor *sensor);
	
public:
    virtual IOService* probe(IOService *provider, SInt32 *score);
    virtual bool       start(IOService * provider);
    virtual void       stop(IOService* provider);
};
