//REVIEW: including version.h here causes xcode to error about HWSENSORS_LASTYEAR (PCH problem?)
#include "version.h"

#include "FakeSMC.h"
#include "FakeSMCDefinitions.h"

#include "OEMInfo.h"

#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IONVRAM.h>

#define super IOService
OSDefineMetaClassAndStructors (FakeSMC, IOService)

bool FakeSMC::init(OSDictionary *dictionary)
{	
	if (!super::init(dictionary))
		return false;
    
    IOLog("HWSensors v%s Copyright %d netkas, slice, usr-sse2, kozlek, navi, THe KiNG, RehabMan. All rights reserved.\n", HWSENSORS_VERSION_STRING, HWSENSORS_LASTYEAR);

    //HWSensorsInfoLog("Opensource SMC device emulator. Copyright 2009 netkas. All rights reserved.");
    
    if (!(smcDevice = new FakeSMCDevice)) {
		HWSensorsInfoLog("failed to create SMC device");
		return false;
	}
    
    if (!setOemProperties(this)) {
        // Another try after 200 ms spin
        IOSleep(200);
        setOemProperties(this);
    }
    
    if (!getProperty(kOEMInfoProduct) || !getProperty(kOEMInfoManufacturer)) {

        HWSensorsErrorLog("failed to obtain OEM vendor & product information from DMI");
        
        // Try to obtain OEM info from Clover EFI
        if (IORegistryEntry* platformNode = fromPath("/efi/platform", gIODTPlane)) {
            
            if (OSData *data = OSDynamicCast(OSData, platformNode->getProperty("OEMVendor"))) {
                if (OSString *vendor = OSString::withCString((char*)data->getBytesNoCopy())) {
                    if (OSString *manufacturer = getManufacturerNameFromOEMName(vendor)) {
                        this->setProperty(kOEMInfoManufacturer, manufacturer);
                        //OSSafeReleaseNULL(manufacturer);
                    }
                    //OSSafeReleaseNULL(vendor);
                }
                //OSSafeReleaseNULL(data);
            }
            
            if (OSData *data = OSDynamicCast(OSData, platformNode->getProperty("OEMBoard"))) {                
                if (OSString *product = OSString::withCString((char*)data->getBytesNoCopy())) {
                    this->setProperty(kOEMInfoProduct, product);
                    //OSSafeReleaseNULL(product);
                }
                //OSSafeReleaseNULL(data);
            }
        }
        else {
            HWSensorsErrorLog("failed to get OEM info from Clover EFI, specific platform profiles will be unavailable");
        }
    }
    
    if (IORegistryEntry *efi = IORegistryEntry::fromPath("/efi", gIODTPlane)) {
        if (OSData *vendor = OSDynamicCast(OSData, efi->getProperty("firmware-vendor"))) { // firmware-vendor is in EFI node
            OSData *buffer = OSData::withCapacity(128);
            const unsigned char* data = static_cast<const unsigned char*>(vendor->getBytesNoCopy());
            
            for (unsigned int index = 0; index < vendor->getLength(); index += 2) {
                buffer->appendByte(data[index], 1);
            }
            
            OSString *name = OSString::withCString(static_cast<const char *>(buffer->getBytesNoCopy()));
            
            setProperty(kFakeSMCFirmwareVendor, name);
            
            //OSSafeRelease(vendor);
            //OSSafeRelease(name);
            OSSafeRelease(buffer);
        }
        
        OSSafeRelease(efi);
    }
		
	return true;
}

IOService *FakeSMC::probe(IOService *provider, SInt32 *score)
{
    if (!super::probe(provider, score))
		return 0;
	
	return this;
}

bool FakeSMC::start(IOService *provider)
{
	if (!super::start(provider)) 
        return false;
    
    int arg_value = 1;
    
    // Check if we have SMC already
    if (OSDictionary *matching = serviceMatching("IOACPIPlatformDevice")) {
        if (OSIterator *iterator = getMatchingServices(matching)) {
            
            OSString *smcNameProperty = OSString::withCString("APP0001");

            while (IOService *service = (IOService*)iterator->getNextObject()) {
                
                OSObject *serviceNameProperty = service->getProperty("name");
                
                if (serviceNameProperty && serviceNameProperty->isEqualTo(smcNameProperty)) {
                    HWSensorsFatalLog("SMC device detected, will not create another one");
                    return false;
                }
            }
            
            OSSafeRelease(iterator);
        }
        
        OSSafeRelease(matching);
    }
    
	if (!smcDevice->initAndStart(provider, this)) {
        HWSensorsInfoLog("failed to initialize SMC device");
		return false;
    }

	registerService();
    
#if NVRAMKEYS
    // Load keys from NVRAM
    if (PE_parse_boot_argn("-fakesmc-use-nvram", &arg_value, sizeof(arg_value)) && !PE_parse_boot_argn("-fakesmc-no-nvram", &arg_value, sizeof(arg_value))) {
        if (UInt32 count = smcDevice->loadKeysFromNVRAM())
            HWSensorsInfoLog("%d key%s loaded from NVRAM", count, count == 1 ? "" : "s");
        else
            HWSensorsInfoLog("NVRAM will be used to store system written keys...");
    }
#endif

	return true;
}

void FakeSMC::stop(IOService *provider)
{
    super::stop(provider);
}

void FakeSMC::free()
{
    super::free();
}