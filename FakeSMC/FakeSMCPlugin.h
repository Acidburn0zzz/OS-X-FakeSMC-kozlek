//
//  FakeSMCPlugin.h
//  HWSensors
//
//  Created by kozlek on 11/02/12.
//
//  Copyright (c) 2012 Natan Zalkin <natan.zalkin@me.com>. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, merge, publish, distribute,
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef HWSensors_FakeSMCFamily_h
#define HWSensors_FakeSMCFamily_h

#include <IOKit/IOService.h>

#include "FakeSMCDefinitions.h"

#define kFakeSMCTemperatureSensor   1
#define kFakeSMCVoltageSensor       2
#define kFakeSMCTachometerSensor    3
#define kFakeSMCFrequencySensor     4
#define kFakeSMCMultiplierSensor    5

#ifdef DEFINE_FAKESMC_SENSOR_PARAMS

struct FakeSMCSensorParams {
    const char *name;
    const char *key;
    const char *type;
    UInt8       size;
};

#define FakeSMCTemperatureCount ((int)(sizeof(FakeSMCTemperature)/sizeof(FakeSMCTemperature[0])))

const struct FakeSMCSensorParams FakeSMCTemperature[] =
{
    {"CPU", KEY_CPU_HEATSINK_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU Proximity", KEY_CPU_PROXIMITY_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
    {"System", KEY_MAINBOARD_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
    {"PCH", KEY_PCH_DIE_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
    {"Northbridge", KEY_NORTHBRIDGE_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
    {"Ambient", KEY_AMBIENT_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE},
};

#define FakeSMCVoltageCount ((int)(sizeof(FakeSMCVoltage)/sizeof(FakeSMCVoltage[0])))

const struct FakeSMCSensorParams FakeSMCVoltage[] =
{
    {"CPU", KEY_CPU_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"CPU Vcore", KEY_CPU_VCORE_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"CPU VTT", KEY_CPU_VTT_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"PCH", KEY_PCH_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"Memory", KEY_MEMORY_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"Main 12V", KEY_MAIN_12V_VOLTAGE, TYPE_SP4B, TYPE_SPXX_SIZE},
    {"PCIe 12V", KEY_PCIE_12V_VOLTAGE, TYPE_SP4B, TYPE_SPXX_SIZE},
    {"Main 5V", KEY_MAIN_5V_VOLTAGE, TYPE_FP4C, TYPE_FPXX_SIZE},
    {"Standby 5V", KEY_STANDBY_5V_VOLTAGE, TYPE_FP4C, TYPE_FPXX_SIZE},
    {"Main 3V", KEY_MAIN_3V3_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"Auxiliary 3V", KEY_AUXILIARY_3V3V_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"CMOS Battery", KEY_POWERBATTERY_VOLTAGE, TYPE_FP2E, TYPE_FPXX_SIZE},
    {"CPU VRM", "VS0C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 1", "VS1C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 2", "VS2C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 3", "VS3C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 4", "VS4C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 5", "VS5C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 6", "VS6C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 7", "VS7C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 8", "VS8C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM 9", "VS9C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM A", "VSAC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM B", "VSBC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM C", "VSCC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM D", "VSDC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM E", "VSEC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"CPU VRM F", "VSFC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply", "Vp0C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 1", "Vp1C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 2", "Vp2C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 3", "Vp3C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 4", "Vp4C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 5", "Vp5C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 6", "Vp6C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 7", "Vp7C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 8", "Vp8C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply 9", "Vp9C", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply A", "VpAC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply B", "VpBC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply C", "VpCC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply D", "VpDC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply E", "VpEC", TYPE_SP78, TYPE_SPXX_SIZE},
    {"Power Supply F", "VpFC", TYPE_SP78, TYPE_SPXX_SIZE},
};

#endif // DEFINE_FAKESMC_SENSOR_PARAMS

class FakeSMCPlugin;

class EXPORT FakeSMCSensor : public OSObject {
    OSDeclareDefaultStructors(FakeSMCSensor)
	
protected:
	FakeSMCPlugin       *owner;
    char                key[5];
	char                type[5];
    UInt8               size;
	UInt32              group;
	UInt32              index;
    float               reference;
    float               gain;
    float               offset;
	
public:
    
	static FakeSMCSensor *withOwner(FakeSMCPlugin *aOwner, const char* aKey, const char* aType, UInt8 aSize, UInt32 aGroup, UInt32 aIndex, float aReference = 0.0f, float aGain = 0.0f, float aOffset = 0.0f);
    
   	virtual bool		initWithOwner(FakeSMCPlugin *aOwner, const char* aKey, const char* aType, UInt8 aSize, UInt32 aGroup, UInt32 aIndex, float aReference, float aGain, float aOffset);
    
    const char          *getKey();
    const char          *getType();
    UInt8               getSize();
	UInt32              getGroup();
	UInt32              getIndex();
    float               getReference();
    float               getGain();
    float               getOffset();
    
    void                encodeNumericValue(float value, void *outBuffer);
};

class EXPORT FakeSMCPlugin : public IOService {
	OSDeclareAbstractStructors(FakeSMCPlugin)

private:
    IOService               *headingProvider;
    IOService               *storageProvider;
    OSDictionary            *sensors;
    
protected:
    OSString                *getPlatformManufacturer();
    OSString                *getPlatformProduct();
    
    bool                    isKeyExists(const char *key);
    bool                    isKeyHandled(const char *key);
    
    SInt8                   takeVacantGPUIndex();
    bool                    takeGPUIndex(UInt8 index);
    bool                    releaseGPUIndex(UInt8 index);
    SInt8                   takeVacantFanIndex();
    bool                    releaseFanIndex(UInt8 index);
    
    bool                    setKeyValue(const char *key, const char *type, UInt8 size, void *value);
    
    virtual FakeSMCSensor   *addSensor(const char *key, const char *type, UInt8 size, UInt32 group, UInt32 index, float reference = 0.0f, float gain = 0.0f, float offset = 0.0f);
    virtual bool            addSensor(FakeSMCSensor *sensor);
	virtual FakeSMCSensor   *addTachometer(UInt32 index, const char *name = 0, FanType type = FAN_RPM, UInt8 zone = 0, FanLocationType location = CENTER_MID_FRONT, SInt8 *fanIndex = 0);
	virtual FakeSMCSensor   *getSensor(const char *key);
    
    OSDictionary            *getConfigurationNode(OSDictionary *root, OSString *name);
    OSDictionary            *getConfigurationNode(OSDictionary *root, const char *name);
    OSDictionary            *getConfigurationNode(OSString *model = NULL);
    
    virtual float           getSensorValue(FakeSMCSensor *sensor);
    
public:    
	virtual bool			init(OSDictionary *properties=0);
	virtual IOService       *probe(IOService *provider, SInt32 *score);
    virtual bool			start(IOService *provider);
	virtual void			stop(IOService *provider);
	virtual void			free(void);
	
	virtual IOReturn		callPlatformFunction(const OSSymbol *functionName, bool waitForFunction, void *param1, void *param2, void *param3, void *param4 ); 
};

#endif