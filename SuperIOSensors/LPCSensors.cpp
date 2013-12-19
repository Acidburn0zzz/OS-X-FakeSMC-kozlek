/*
 *  SuperIOFamily.cpp
 *  HWSensors
 *
 *  Created by kozlek on 08/10/10.
 *
 */

//  The MIT License (MIT)
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


#include "FakeSMCDefinitions.h"
#include "LPCSensors.h"
#include "SuperIO.h"
#include "OEMInfo.h"

#include <IOKit/IOTimerEventSource.h>

#define super FakeSMCPlugin
OSDefineMetaClassAndAbstractStructors(LPCSensors, FakeSMCPlugin)

IOReturn LPCSensors::woorkloopTimerEvent(void)
{
    for (int index = 0; index < kLPCSensorsMaxFanControls; index ++) {
        
        LPCSensorsFanControl *control = &tachometerControls[index];
        
        switch (control->action) {
            case kLPCSensorsFanActionProbe: {
                
                float value = readTachometer(index);
                UInt8 percent = readTachometerControl(index);
                
                HWSensorsDebugLog("probbing[%d] value=%d target=%d control=%d", index, (UInt32)value, (UInt32)control->target, percent);
                
                if (value > control->target && value - control->target > kLPCSensorsMatchTheresholdRPM && percent > 0) {
                    control->action = kLPCSensorsFanActionDecrement;
                }
                else if (value < control->target && control->target - value > kLPCSensorsMatchTheresholdRPM && percent < 99) {
                    control->action = kLPCSensorsFanActionIncrement;
                }
                else {
                    control->action = kLPCSensorsFanActionNone;
                }
                
                break;
            }
                
            case kLPCSensorsFanActionDecrement: {
                
                float value = readTachometer(index);
                SInt16 percent = readTachometerControl(index);
                
                HWSensorsDebugLog("decrementing[%d] value=%d target=%d control=%d", index, (UInt32)value, (UInt32)control->target, percent);

                if ((value > control->target ? value - control->target : control->target - value) <= kLPCSensorsMatchTheresholdRPM) {
                    control->action = kLPCSensorsFanActionMatched;
                }
                else if (value < control->target) {
                    control->action = kLPCSensorsFanActionProbe;
                }
                else if (percent >= 0) {
                    percent -= kLPCSensorsControlIncrement;
                    writeTachometerControl(index, percent < 0 ? 0 : percent);
                }
                else {
                    control->action = kLPCSensorsFanActionProbe;
                }
                
                break;    
            }
                
            case kLPCSensorsFanActionIncrement: {
                
                float value = readTachometer(index);
                UInt8 percent = readTachometerControl(index);
                
                HWSensorsDebugLog("incrementing[%d] value=%d target=%d control=%d", index, (UInt32)value, (UInt32)control->target, percent);
                
                if ((value > control->target ? value - control->target : control->target - value) <= kLPCSensorsMatchTheresholdRPM) {
                    control->action = kLPCSensorsFanActionMatched;
                }
                else if (value > control->target) {
                    control->action = kLPCSensorsFanActionProbe;
                }
                else if (percent < 99) {
                    percent += kLPCSensorsControlIncrement;
                    writeTachometerControl(index, percent > 100 ? 100 : percent);
                }
                else {
                    control->action = kLPCSensorsFanActionProbe;
                }
                
                break;    
            }
                
            case kLPCSensorsFanActionMatched:              
                HWSensorsDebugLog("matched!");
                control->action = kLPCSensorsFanActionNone;
                break;
                
            case kLPCSensorsFanActionNone:
            default:
                break;
        }
    }
    
    timerEventSource->setTimeoutMS(kLPCSensorsWorkloopTimeout);
    
    return kIOReturnSuccess;
}

bool LPCSensors::checkConfigurationNode(OSObject *node, const char *name)
{
    if (node) {
        if (OSString *configName = OSDynamicCast(OSString, node)) {
            if (!configName->isEqualTo(name))
                return false;
        }
        else if (OSDictionary *configDict = OSDynamicCast(OSDictionary, node)) {
            if ((configName = OSDynamicCast(OSString, configDict->getObject("name")))) {
                if (!configName->isEqualTo(name))
                    return false;
            }
            else return false;
        }
        else return false;
    }
    
    return true;
}

bool LPCSensors::addSensorFromConfigurationNode(OSObject *node, const char *key, const char *type, UInt8 size, UInt32 group, UInt32 index)
{
    float reference = 0, gain = 0, offset = 0;
    
    if (OSDictionary *dictionary = OSDynamicCast(OSDictionary, node))
        FakeSMCSensor::parseModifiers(dictionary, &reference, &gain, &offset);
    
    if (!this->addSensor(key, type, size, group, index, reference, gain, offset)) {
        const char *group_name;
        
        switch (group) {
            case kFakeSMCTemperatureSensor:
                group_name = "temperature";
                break;
            case kFakeSMCTachometerSensor:
                group_name = "tachometer";
                break;
            case kFakeSMCVoltageSensor:
                group_name = "voltage";
                break;
            case kFakeSMCFrequencySensor:
                group_name = "frequency";
                break;
            case kFakeSMCMultiplierSensor:
                group_name = "multiplier";
                break;
                
            default:
                group_name = "";
                break;
        }
        
        HWSensorsWarningLog("failed to add %s sensor for key %s", group_name, key);
        
        return false;
    }
    
    return true;
}

bool LPCSensors::addTemperatureSensors(OSDictionary *configuration)
{
    HWSensorsDebugLog("adding temperature sensors...");
        
    for (int i = 0; i < temperatureSensorsLimit(); i++) 
    {				
        char key[8];

        snprintf(key, 8, "TEMPIN%X", i);
        
        if (OSObject* node = configuration->getObject(key)) {
            if (!addSensor(node, kFakeSMCCategoryTemperature, kFakeSMCTemperatureSensor, i)) {
                if (gpuIndex < 0)
                    gpuIndex = takeVacantGPUIndex();
                    
                if (gpuIndex >= 0 && checkConfigurationNode(configuration, "GPU Die")) {
                    snprintf(key, 5, KEY_FORMAT_GPU_DIODE_TEMPERATURE, gpuIndex);
                    if (!addSensorFromConfigurationNode(node, key, TYPE_SP78, TYPE_SPXX_SIZE, kFakeSMCTemperatureSensor, i)) {
                        releaseGPUIndex(gpuIndex);
                        gpuIndex = -1;
                    }
                }
            }
        }
    }

    return true;
}

bool LPCSensors::addVoltageSensors(OSDictionary *configuration)
{
    HWSensorsDebugLog("adding voltage sensors...");
       
    for (int i = 0; i < voltageSensorsLimit(); i++)
    {				
        char key[5];
        
        snprintf(key, 5, "VIN%X", i);
        
        if (OSObject* node = configuration->getObject(key)) {
            if (!addSensor(node, kFakeSMCCategoryVoltage, kFakeSMCVoltageSensor, i)) {
                if (gpuIndex < 0)
                    gpuIndex = takeVacantGPUIndex();
                
                if (gpuIndex >= 0 && checkConfigurationNode(configuration, "GPU Core")) {
                    snprintf(key, 5, KEY_FORMAT_GPU_VOLTAGE, gpuIndex);
                    if (!addSensorFromConfigurationNode(node, key, TYPE_FP2E, TYPE_FPXX_SIZE, kFakeSMCVoltageSensor, i)) {
                        releaseGPUIndex(gpuIndex);
                        gpuIndex = -1;
                    }
                }
            }
        }
    }

    return true;
}

bool LPCSensors::addTachometerSensors(OSDictionary *configuration)
{
    HWSensorsDebugLog("adding tachometer sensors...");
    
    FanLocationType location = LEFT_LOWER_FRONT;
    
    for (int i = 0; i < tachometerSensorsLimit(); i++) {
        char key[7];
        SInt8 fanIndex;
        
        snprintf(key, 7, "FANIN%X", i);
        
        if (OSString* name = OSDynamicCast(OSString, configuration->getObject(key))){
            if (addTachometer(i, name->getLength() > 0 ? name->getCStringNoCopy() : 0, FAN_RPM, 0, location++, &fanIndex)){
                
                if (supportsTachometerControl() && fanIndex > -1) {
                    
                    //if (readTachometerControl(fanIndex) > 0) {
                        
                        tachometerControls[fanIndex].target = 0;
                        tachometerControls[fanIndex].action = kLPCSensorsFanActionNone;
                        
                        UInt16 value;
                        
                        // Minimum RPM
                        snprintf(key, 5, KEY_FORMAT_FAN_MIN, fanIndex);
                        
                        fakeSMCPluginEncodeNumericValue(kLPCSensorsMinRPM, TYPE_FPE2, TYPE_FPXX_SIZE, &value);
                        
                        setKeyValue(key, TYPE_FPE2, TYPE_FPXX_SIZE, &value);
                        
                        // Maximum RPM
                        snprintf(key, 5, KEY_FORMAT_FAN_MAX, fanIndex);
                        
                        fakeSMCPluginEncodeNumericValue(kLPCSensorsMaxRPM, TYPE_FPE2, TYPE_FPXX_SIZE, &value);
                        
                        setKeyValue(key, TYPE_FPE2, TYPE_FPXX_SIZE, &value);
                        
                        // Target RPM and fan control sensor
                        snprintf(key, 5, KEY_FORMAT_FAN_TARGET, fanIndex);
                        
                        addSensor(key, TYPE_FPE2, TYPE_FPXX_SIZE, kLPCSensorsFanController, i);
                    }
                //}
            }
            else HWSensorsWarningLog("failed to add tachometer sensor %d", i);
        }
    }
    
    return true;
}

UInt8 LPCSensors::temperatureSensorsLimit()
{
    return 3;
}

UInt8 LPCSensors::voltageSensorsLimit()
{
    return 9;
}

UInt8 LPCSensors::tachometerSensorsLimit()
{
    return 5;
}

float LPCSensors::readTemperature(UInt32 index)
{
	return 0;
}

float LPCSensors::readVoltage(UInt32 index)
{
	return 0;
}

float LPCSensors::readTachometer(UInt32 index)
{
	return 0;
}

bool LPCSensors::supportsTachometerControl()
{
    return false;
}

UInt8 LPCSensors::readTachometerControl(UInt32 index)
{
    return 0;
}

void LPCSensors::writeTachometerControl(UInt32 index, UInt8 percent)
{
    //
}

float LPCSensors::getSensorValue(FakeSMCSensor *sensor)
{
    float value = 0;
    
    if (sensor) {
        switch (sensor->getGroup()) {
            case kFakeSMCTemperatureSensor:
                value = sensor->getOffset() + readTemperature(sensor->getIndex());
                break;
                
            case kFakeSMCVoltageSensor:
                value = readVoltage(sensor->getIndex());
                value = sensor->getOffset() + value + (value - sensor->getReference()) * sensor->getGain();
                break;
                
            case kFakeSMCTachometerSensor:
                value = readTachometer(sensor->getIndex());
                break;
                
            case kLPCSensorsFanController:
                value = tachometerControls[sensor->getIndex()].target;
                break;
        }
    }
    
	return value;
}

void LPCSensors::setSensorValue(FakeSMCSensor *sensor, float value)
{
    if (sensor) {
        switch (sensor->getGroup()) {
            case kLPCSensorsFanController:
                tachometerControls[sensor->getIndex()].target = value;
                tachometerControls[sensor->getIndex()].action = kLPCSensorsFanActionProbe;                
                break;
        }
    }
}

bool LPCSensors::initialize()
{
    return true;
}

bool LPCSensors::init(OSDictionary *properties)
{
	if (!super::init(properties))
		return false;
	
    address = 0;
    port = 0;
   	model = 0;
    
    modelName = "unknown";
    vendorName = "unknown";
    
    gpuIndex = -1;
        
	return true;
}

bool LPCSensors::start(IOService *provider)
{	
	if (!super::start(provider)) 
        return false;
    
    OSNumber *number = OSDynamicCast(OSNumber, provider->getProperty(kSuperIOHWMAddress));
    
    if (!number || !(address = number->unsigned16BitValue())) {
        HWSensorsFatalLog("wrong address provided");
        return false;
    }
    
    number = OSDynamicCast(OSNumber, provider->getProperty(kSuperIOControlPort));
    
    if (!number || !(port = number->unsigned8BitValue())) {
        HWSensorsFatalLog("wrong port provided");
        return false;
    }
    
    number = OSDynamicCast(OSNumber, provider->getProperty(kSuperIOModelValue));
    
    if (!number || !(model = number->unsigned16BitValue())) {
        HWSensorsFatalLog("wrong model provided");
        return false;
    }
    
    OSString *string = OSDynamicCast(OSString, provider->getProperty(kSuperIOModelName));
    
    if (!string || !(modelName = string->getCStringNoCopy())) {
        HWSensorsFatalLog("wrong model name provided");
        return false;
    }
    
    string = OSDynamicCast(OSString, provider->getProperty(kSuperIOVendorName));
    
    if (!string || !(vendorName = string->getCStringNoCopy())) {
        HWSensorsFatalLog("wrong vendor name provided");
        return false;
    }
    
    if (!initialize())
        return false;
    
    OSString *modelString = OSString::withCString(modelName);

	if (OSDictionary *configuration = getConfigurationNode(modelString))
    {
        enableExclusiveAccessMode();
        addTemperatureSensors(configuration);
        addVoltageSensors(configuration);
        addTachometerSensors(configuration);
        disableExclusiveAccessMode();
    }
    else HWSensorsWarningLog("no platform profile provided");
    
    OSSafeReleaseNULL(modelString);
    
    // woorkloop
    if (!(workloop = getWorkLoop())) {
        HWSensorsFatalLog("Failed to obtain workloop");
        return false;
    }
    
    if (!(timerEventSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &LPCSensors::woorkloopTimerEvent)))) {
        HWSensorsFatalLog("failed to initialize timer event source");
        return false;
    }
    
    if (kIOReturnSuccess != workloop->addEventSource(timerEventSource))
    {
        HWSensorsFatalLog("failed to add timer event source into workloop");
        return false;
    }
    
    timerEventSource->setTimeoutMS(1000);
    
    registerService();
    
    HWSensorsInfoLog("started");

	return true;
}

void LPCSensors::stop(IOService *provider)
{
    timerEventSource->cancelTimeout();
    workloop->removeEventSource(timerEventSource);
    
    if (gpuIndex >= 0)
        releaseGPUIndex(gpuIndex);

    super::stop(provider);
}