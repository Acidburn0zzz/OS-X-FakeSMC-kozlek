/*
 *  HWSensors.h
 *  CPUSensorsPlugin
 *  
 *  Based on code by mercurysquad, superhai (C) 2008
 *  Based on code from Open Hardware Monitor project by Michael Möller (C) 2011
 *  Based on code by slice (C) 2013
 *
 *  Created by kozlek on 30/09/10.
 *  Copyright 2010 Natan Zalkin <natan.zalkin@me.com>. All rights reserved.
 *
 */

/*
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 
 http://www.mozilla.org/MPL/
 
 Software distributed under the License is distributed on an "AS IS" basis,
 WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 for the specific language governing rights and limitations under the License.
 
 The Original Code is the Open Hardware Monitor code.
 
 The Initial Developer of the Original Code is 
 Michael Möller <m.moeller@gmx.ch>.
 Portions created by the Initial Developer are Copyright (C) 2011
 the Initial Developer. All Rights Reserved.
 
 Contributor(s):
 
 Alternatively, the contents of this file may be used under the terms of
 either the GNU General Public License Version 2 or later (the "GPL"), or
 the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 in which case the provisions of the GPL or the LGPL are applicable instead
 of those above. If you wish to allow use of your version of this file only
 under the terms of either the GPL or the LGPL, and not to allow others to
 use your version of this file under the terms of the MPL, indicate your
 decision by deleting the provisions above and replace them with the notice
 and other provisions required by the GPL or the LGPL. If you do not delete
 the provisions above, a recipient may use your version of this file under
 the terms of any one of the MPL, the GPL or the LGPL.
 
 */

#include "CPUSensors.h"
#include "FakeSMCDefinitions.h"
#include "IntelDefinitions.h"

#include <IOKit/IODeviceTreeSupport.h>
#include <IOKit/IORegistryEntry.h>

#include "timer.h"

//REVIEW: avoids problem with Xcode 5.1.0 where -dead_strip eliminates these required symbols
#include <libkern/OSKextLib.h>
void* _hack_CPUSensors_dontstrip[] =
{
    (void*)&OSKextGetCurrentIdentifier,
    (void*)&OSKextGetCurrentLoadTag,
    (void*)&OSKextGetCurrentVersionString,
};

#define SAMPLE_EVENT_INTERVAL        100

enum {
    kCPUSensorsThermalCore           = BIT(0),
    kCPUSensorsThermalPackage        = BIT(1),
    kCPUSensorsMultiplierCore        = BIT(2),
    kCPUSensorsMultiplierPackage     = BIT(3),
    kCPUSensorsFrequencyCore         = BIT(4),
    kCPUSensorsFrequencyPackage      = BIT(5),

    kCPUSensorsPowerTotal            = BIT(6),
    kCPUSensorsPowerCores            = BIT(7),
    kCPUSensorsPowerUncore           = BIT(8),
    kCPUSensorsPowerDram             = BIT(9),
};

static UInt16 cpu_energy_msrs[] =
{
    MSR_PKG_ENERY_STATUS,
    MSR_PP0_ENERY_STATUS,
    MSR_PP1_ENERY_STATUS,
    MSR_DRAM_ENERGY_STATUS
};

static UInt16 cpu_energy_flgs[] =
{
    kCPUSensorsPowerTotal,
    kCPUSensorsPowerCores,
    kCPUSensorsPowerUncore,
    kCPUSensorsPowerDram
};

#define super FakeSMCPlugin
OSDefineMetaClassAndStructors(CPUSensors, FakeSMCPlugin)

inline uint64_t rdmpc64(uint32_t counter)
{
    UInt32 lo,hi;
    rdpmc(counter, lo, hi);
    return ((UInt64)hi << 32 ) | lo;
}

static inline UInt8 get_hex_index(char c)
{       
	return c > 96 && c < 103 ? c - 87 : c > 47 && c < 58 ? c - 48 : 0;
};

static inline UInt8 get_cpu_number()
{
    UInt8 number = cpu_number() & 0xFF;
    
    if (cpuid_info()->thread_count > cpuid_info()->core_count) {
        return !(number % 2) ? number >> 1 : UINT8_MAX;
    }
    
    return number;
}

static UInt8  tjmax[kCPUSensorsMaxCpus];

static void read_cpu_tjmax(void *magic)
{
    UInt32 number = get_cpu_number();

    if (number < kCPUSensorsMaxCpus) {
        tjmax[number] = (rdmsr64(MSR_IA32_TEMP_TARGET) >> 16) & 0xFF;
    }
}

static void update_counters(void *arg)
{
    CPUSensorsCounters *counters = (CPUSensorsCounters *)arg;

    UInt32 number = get_cpu_number();

    if (number < kCPUSensorsMaxCpus) {

        UInt64 msr;

        if (bit_get(counters->event_flags, kCPUSensorsThermalCore)) {
            if ((msr = rdmsr64(MSR_IA32_THERM_STS)) & 0x80000000) {
                counters->thermal_status[number] = (msr >> 16) & 0x7F;
            }
        }

        if (number == 0 && bit_get(counters->event_flags, kCPUSensorsThermalPackage)) {
            if ((msr = rdmsr64(MSR_IA32_PACKAGE_THERM_STATUS)) & 0x80000000) {
                counters->thermal_status_package = (msr >> 16) & 0x7F;
            }
        }

        if (bit_get(counters->event_flags, kCPUSensorsMultiplierCore) || (number == 0 && bit_get(counters->event_flags, kCPUSensorsMultiplierPackage))) {

            counters->perf_status[number] =  rdmsr64(MSR_IA32_PERF_STS) & 0xFFFF;

            // Performance counters
            if (counters->update_perf_counters) {
                counters->aperf_before[number] = counters->aperf_after[number];
                counters->mperf_before[number] = counters->mperf_after[number];
                counters->aperf_after[number] = rdmsr64(MSR_IA32_APERF);
                counters->mperf_after[number] = rdmsr64(MSR_IA32_MPERF);
            }
        }

        // Frequency counters
        if (counters->update_perf_counters && (bit_get(counters->event_flags, kCPUSensorsFrequencyCore) || (number == 0 && bit_get(counters->event_flags, kCPUSensorsFrequencyPackage)))) {
            counters->utc_before[number] = counters->utc_after[number];
            counters->urc_before[number] = counters->urc_after[number];
            counters->utc_after[number] = rdmpc64(0x40000001);
            counters->urc_after[number] = rdmpc64(0x40000002);
        }

        // Energy counters
        if (number == 0) {
            for (UInt8 index = 0; index < 4; index++) {
                if (bit_get(counters->event_flags, cpu_energy_flgs[index])) {
                    counters->energy_before[index] = counters->energy_after[index];
                    counters->energy_after[index] = rdmsr64(cpu_energy_msrs[index]);
                }
            }
        }
    }
}

void CPUSensors::calculateMultiplier(UInt32 index)
{
    switch (cpuid_info()->cpuid_cpufamily) {
        case CPUFAMILY_INTEL_NEHALEM:
        case CPUFAMILY_INTEL_WESTMERE:
            if (baseMultiplier > 0 && ratio[index] > 1.0)
                multiplier[index] = ROUND(ratio[index] * (float)baseMultiplier);
            else
                multiplier[index] = (float)(counters.perf_status[index] & 0xFF);
            break;
        case CPUFAMILY_INTEL_SANDYBRIDGE:
        case CPUFAMILY_INTEL_IVYBRIDGE:
        case CPUFAMILY_INTEL_HASWELL:
            if (baseMultiplier > 0 && ratio[index] > 1.0)
                multiplier[index] = ROUND(ratio[index] * (float)baseMultiplier);
            else
                multiplier[index] = (float)((counters.perf_status[index] >> 8) & 0xFF);
            break;
        default: {
            UInt8 fid = (counters.perf_status[index] >> 8) & 0xFF;
            multiplier[index] = float((float)((fid & 0x1f)) * (fid & 0x80 ? 0.5 : 1.0) + 0.5f * (float)((fid >> 6) & 1));
            break;
        }
    }
}

void CPUSensors::calculateTimedCounters()
{
    if (bit_get(counters.event_flags, kCPUSensorsMultiplierCore | kCPUSensorsMultiplierPackage)) {
        for (UInt8 index = 0; index < availableCoresCount; index++) {
            if (baseMultiplier) {
                UInt64 aperf = counters.aperf_after[index] - counters.aperf_before[index];
                UInt64 mperf = counters.mperf_after[index] - counters.mperf_before[index];

                if (mperf) {
                    ratio[index] = (double)aperf / (double)mperf;
                }
            }

            calculateMultiplier(index);
        }
    }

    if (bit_get(counters.event_flags, kCPUSensorsFrequencyCore | kCPUSensorsFrequencyPackage)) {
        for (UInt8 index = 0; index < availableCoresCount; index++) {
            if (baseMultiplier > 0) {
                UInt64 thread_clocks = counters.utc_after[index] < counters.utc_before[index] ? UINT64_MAX - counters.utc_before[index] + counters.utc_after[index] : counters.utc_after[index] - counters.utc_before[index];
                UInt64 ref_clocks = counters.urc_after[index] < counters.urc_before[index] ? UINT64_MAX - counters.urc_before[index] + counters.urc_after[index] : counters.urc_after[index] - counters.urc_before[index];

                if (ref_clocks) {
                    turbo[index] = (double)thread_clocks / (double)ref_clocks;
                }
            }
            else if (!bit_get(counters.event_flags, kCPUSensorsMultiplierCore | kCPUSensorsMultiplierPackage)) {
                calculateMultiplier(index);
            }
        }
    }

    if (timerEventDeltaTime && timerEventDeltaTime < 10.0f && bit_get(counters.event_flags, kCPUSensorsPowerTotal | kCPUSensorsPowerCores | kCPUSensorsPowerUncore | kCPUSensorsPowerDram)) {
        for (UInt8 index = 0; index < 4; index++) {

            UInt64 deltaEnergy = counters.energy_after[index] < counters.energy_before[index] ? UINT64_MAX - counters.energy_before[index] + counters.energy_after[index] : counters.energy_after[index] - counters.energy_before[index];

            energy[index] = (double)deltaEnergy / timerEventDeltaTime;
        }
    }
}

IOReturn CPUSensors::timerEventAction()
{
    if (counters.event_flags) {

        double time = ptimer_read_seconds();

        timerEventDeltaTime =  time - timerEventLastTime;
        timerEventLastTime = time;

        mp_rendezvous_no_intrs(update_counters, &counters);

        calculateTimedCounters();

        if (timerEventDeltaTime == 0 || timerEventDeltaTime > 10.0f) {
            timerEventSource->setTimeoutMS(500);
        }
    }
    
    return kIOReturnSuccess;
}

bool CPUSensors::willReadSensorValue(FakeSMCSensor *sensor, float *outValue)
{    
    UInt32 index = sensor->getIndex();

    switch (sensor->getGroup()) {
        case kCPUSensorsThermalCore:
            bit_set(counters.event_flags, kCPUSensorsThermalCore);
            *outValue = tjmax[index] - counters.thermal_status[index];
            break;

        case kCPUSensorsThermalPackage:
            bit_set(counters.event_flags, kCPUSensorsThermalPackage);
            *outValue = tjmax[index] - counters.thermal_status_package;
            break;
            
        case kCPUSensorsMultiplierCore:
        case kCPUSensorsMultiplierPackage:
            bit_set(counters.event_flags, sensor->getGroup());
            *outValue = multiplier[index];
            break;
            
        case kCPUSensorsFrequencyCore:
        case kCPUSensorsFrequencyPackage:
            bit_set(counters.event_flags, sensor->getGroup());
            if (baseMultiplier) {
                *outValue = turbo[index] * (float)busClock * (float)baseMultiplier;
            }
            else {
                *outValue = multiplier[index] * (float)busClock;
            }
            break;

        case kCPUSensorsPowerTotal:
        case kCPUSensorsPowerCores:
        case kCPUSensorsPowerUncore:
        case kCPUSensorsPowerDram:
            bit_set(counters.event_flags, sensor->getGroup());
            *outValue = energyUnits * energy[index];
            break;

        default:
            return false;
            
    }

    if (counters.event_flags) {
        // Rearm timer
        //if (timerEventCounter <= 0) {
            timerEventSource->setTimeoutMS(50);
        //}

        //timerEventCounter = 0;
    }
    
    return true;
}

FakeSMCSensor *CPUSensors::addSensor(const char *key, const char *type, UInt8 size, UInt32 group, UInt32 index, float reference, float gain, float offset)
{
    FakeSMCSensor *result = super::addSensor(key, type, size, group, index);
    
    if (result) {
        bit_set(counters.event_flags, group);
    }
    
    return result;
}

bool CPUSensors::start(IOService *provider)
{
    if (!super::start(provider)) 
        return false;

    // Pre-checks
    
    cpuid_set_info();
	
	if (strcmp(cpuid_info()->cpuid_vendor, CPUID_VID_INTEL) != 0)	{
		HWSensorsFatalLog("no Intel processor found");
		return false;
	}
	
	if(!(cpuid_info()->cpuid_features & CPUID_FEATURE_MSR))	{
		HWSensorsFatalLog("processor does not support Model Specific Registers (MSR)");
		return false;
	}

    // Init timer

    if (IOWorkLoop *workloop = getWorkLoop()) {
        if (!(timerEventSource = IOTimerEventSource::timerEventSource( this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &CPUSensors::timerEventAction)))) {
            HWSensorsFatalLog("Failed to initialize timer event source");
            return false;
        }

        if (kIOReturnSuccess != workloop->addEventSource(timerEventSource))
        {
            HWSensorsFatalLog("Failed to add timer event source into workloop");
            return false;
        }
    }
    else {
        HWSensorsFatalLog("Failed to obtain current workloop");
        return false;
    }

    // Configure
        
    if (OSDictionary *configuration = getConfigurationNode())
    {
        if (OSNumber* number = OSDynamicCast(OSNumber, configuration->getObject("Tjmax"))) {

            UInt8 userTjmax = number->unsigned8BitValue();
            
            if (userTjmax) {
                memset(tjmax, userTjmax, kCPUSensorsMaxCpus);
                HWSensorsInfoLog("force Tjmax value to %d", tjmax[0]);
            }
        }
        
        if (OSString* string = OSDynamicCast(OSString, configuration->getObject("PlatformString"))) {
            // User defined platform key (RPlt)
            if (string->getLength() > 0) {
                char p[9] = "\0\0\0\0\0\0\0\0";
                snprintf(p, 9, "%s", string->getCStringNoCopy());
                platform = OSData::withBytes(p, 8);
            }
        }
    }

    // Estimating Tjmax value if not set
    if (!tjmax[0]) {
		switch (cpuid_info()->cpuid_family)
		{
			case 0x06: 
				switch (cpuid_info()->cpuid_model) 
                {
                    case CPUID_MODEL_PENTIUM_M:
                        tjmax[0] = 100;
                        if (!platform) platform = OSData::withBytes("M70\0\0\0\0\0", 8);
                        break;
                            
                    case CPUID_MODEL_YONAH:
                        if (!platform) platform = OSData::withBytes("K22\0\0\0\0\0", 8);
                        tjmax[0] = 85;
                        break;
                        
                    case CPUID_MODEL_MEROM: // Intel Core (65nm)
                        if (!platform) platform = OSData::withBytes("M75\0\0\0\0\0", 8);
                        switch (cpuid_info()->cpuid_stepping)
                        {
                            case 0x02: // G0
                                tjmax[0] = 100; 
                                break;
                                
                            case 0x06: // B2
                                switch (cpuid_info()->core_count) 
                                {
                                    case 2:
                                        tjmax[0] = 80; 
                                        break;
                                    case 4:
                                        tjmax[0] = 90; 
                                        break;
                                    default:
                                        tjmax[0] = 85; 
                                        break;
                                }
                                //tjmax[0] = 80; 
                                break;
                                
                            case 0x0B: // G0
                                tjmax[0] = 90; 
                                break;
                                
                            case 0x0D: // M0
                                tjmax[0] = 85; 
                                break;
                                
                            default:
                                tjmax[0] = 85; 
                                break;
                                
                        } 
                        break;
                        
                    case CPUID_MODEL_PENRYN: // Intel Core (45nm)
                                             // Mobile CPU ?
                        if (!platform) platform = OSData::withBytes("M82\0\0\0\0\0", 8);
                        if (rdmsr64(0x17) & (1<<28))
                            tjmax[0] = 105;
                        else
                            tjmax[0] = 100; 
                        break;
                        
                    case CPUID_MODEL_ATOM: // Intel Atom (45nm)
                        if (!platform) platform = OSData::withBytes("T9\0\0\0\0\0", 8);
                        switch (cpuid_info()->cpuid_stepping)
                        {
                            case 0x02: // C0
                                tjmax[0] = 90; 
                                break;
                            case 0x0A: // A0, B0
                                tjmax[0] = 100; 
                                break;
                            default:
                                tjmax[0] = 90; 
                                break;
                        } 
                        break;
                        
                    case CPUID_MODEL_NEHALEM:
                    case CPUID_MODEL_FIELDS:
                    case CPUID_MODEL_DALES:
                    case CPUID_MODEL_DALES_32NM:
                    case CPUID_MODEL_WESTMERE:
                    case CPUID_MODEL_NEHALEM_EX:
                    case CPUID_MODEL_WESTMERE_EX:
                        if (!platform) platform = OSData::withBytes("k74\0\0\0\0\0", 8);
                        mp_rendezvous_no_intrs(read_cpu_tjmax, NULL);
                        break;
                        
                    case CPUID_MODEL_SANDYBRIDGE:
                    case CPUID_MODEL_JAKETOWN:
                        if (!platform) platform = OSData::withBytes("k62\0\0\0\0\0", 8);
                        mp_rendezvous_no_intrs(read_cpu_tjmax, NULL);
                        break;
                        
                    case CPUID_MODEL_IVYBRIDGE:
                    case CPUID_MODEL_IVYBRIDGE_EP:
                        if (!platform) platform = OSData::withBytes("d8\0\0\0\0\0\0", 8);
                        mp_rendezvous_no_intrs(read_cpu_tjmax, NULL);
                        break;
                    
                    case CPUID_MODEL_HASWELL_DT:
                    case CPUID_MODEL_HASWELL_MB:
                        // TODO: platform value for desktop Haswells
                    case CPUID_MODEL_HASWELL_ULT:
                    case CPUID_MODEL_HASWELL_ULX:
                        if (!platform) platform = OSData::withBytes("j43\0\0\0\0\0", 8); // TODO: got from macbookair6,2 need to check for other platforms
                        mp_rendezvous_no_intrs(read_cpu_tjmax, NULL);
                        break;
                        
                    default:
                        HWSensorsWarningLog("found unsupported Intel processor, using default Tjmax");
                        tjmax[0] = 100;
                        break;
                }
                break;
                
            case 0x0F: 
                switch (cpuid_info()->cpuid_model) 
                {
                    case 0x00: // Pentium 4 (180nm)
                    case 0x01: // Pentium 4 (130nm)
                    case 0x02: // Pentium 4 (130nm)
                    case 0x03: // Pentium 4, Celeron D (90nm)
                    case 0x04: // Pentium 4, Pentium D, Celeron D (90nm)
                    case 0x06: // Pentium 4, Pentium D, Celeron D (65nm)
                        tjmax[0] = 100;
                        break;
                        
                    default:
                        HWSensorsWarningLog("found unsupported Intel processor, using default Tjmax");
                        tjmax[0] = 100;
                        break;
                }
                break;
				
			default:
				HWSensorsFatalLog("found unknown Intel processor family");
				return false;
		}

        // Setup Tjmax
        switch (cpuid_info()->cpuid_cpufamily) {
            case CPUFAMILY_INTEL_NEHALEM:
            case CPUFAMILY_INTEL_WESTMERE:
            case CPUFAMILY_INTEL_SANDYBRIDGE:
            case CPUFAMILY_INTEL_IVYBRIDGE:
            case CPUFAMILY_INTEL_HASWELL:
                break;

            default: {
                UInt8 calculatedTjmax = tjmax[0];
                memset(tjmax, calculatedTjmax, kCPUSensorsMaxCpus);
                break;
            }
        }
	}

    // bus clock
    busClock = 0;
    
    if (IORegistryEntry *regEntry = fromPath("/efi/platform", gIODTPlane))
        if (OSData *data = OSDynamicCast(OSData, regEntry->getProperty("FSBFrequency")))
            busClock = *((UInt64*) data->getBytesNoCopy()) / 1e6;
    
    if (busClock == 0)
        busClock = (gPEClockFrequencyInfo.bus_frequency_max_hz >> 2) / 1e6;
    
    HWSensorsInfoLog("CPU family 0x%x, model 0x%x, stepping 0x%x, cores %d, threads %d, TJmax %d", cpuid_info()->cpuid_family, cpuid_info()->cpuid_model, cpuid_info()->cpuid_stepping, cpuid_info()->core_count, cpuid_info()->thread_count, tjmax[0]);
    
//    mp_rendezvous_no_intrs(cpu_check, NULL);
//    
//    for (int count = 0; count < kCPUSensorsMaxCpus; count++) {
//        if (cpu_enabled[count]) {
//            HWSensorsInfoLog("CPU[%d] lapic=0x%llx value = 0x%llx", count, cpu_lapic[count], cpu_check_value[count]);
//        }        
//    }

    // platform keys
    if (platform) {
        HWSensorsInfoLog("setting platform keys to [%-8s]", (const char*)platform->getBytesNoCopy());
        
        if (/*!isKeyExists("RPlt") &&*/ !setKeyValue("RPlt", TYPE_CH8, platform->getLength(), (void*)platform->getBytesNoCopy()))
            HWSensorsWarningLog("failed to set platform key RPlt");
        
        if (/*!isKeyExists("RBr") &&*/ !setKeyValue("RBr", TYPE_CH8, platform->getLength(), (void*)platform->getBytesNoCopy()))
            HWSensorsWarningLog("failed to set platform key RBr");
    }
    
    // digital thermal sensor at core level
    bit_set(counters.event_flags, kCPUSensorsThermalCore);
    mp_rendezvous_no_intrs(update_counters, &counters);
                           
    for (uint32_t i = 0; i < kCPUSensorsMaxCpus; i++) {
        if (counters.thermal_status[i]) {
            
            availableCoresCount++;
            
            char key[5];
            
            snprintf(key, 5, KEY_FORMAT_CPU_DIE_TEMPERATURE, i);
            
            if (!addSensor(key, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsThermalCore, i))
                HWSensorsWarningLog("failed to add temperature sensor");
        }
    }
    
    // digital thermal sensor at package level
    switch (cpuid_info()->cpuid_cpufamily) {
        case CPUFAMILY_INTEL_SANDYBRIDGE:
        case CPUFAMILY_INTEL_IVYBRIDGE:
        case CPUFAMILY_INTEL_HASWELL:
        {
            uint32_t cpuid_reg[4];
            
            do_cpuid(6, cpuid_reg);
            
            if ((uint32_t)bitfield32(cpuid_reg[eax], 4, 4)) {
                if (!addSensor(KEY_CPU_PACKAGE_TEMPERATURE, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsThermalPackage, 0))
                    HWSensorsWarningLog("failed to add cpu package temperature sensor");
            }
            break;
        }
    }
    
    // multiplier
    switch (cpuid_info()->cpuid_cpufamily) {
        case CPUFAMILY_INTEL_SANDYBRIDGE:
        case CPUFAMILY_INTEL_IVYBRIDGE:
        case CPUFAMILY_INTEL_HASWELL:
            if ((baseMultiplier = (rdmsr64(MSR_PLATFORM_INFO) >> 8) & 0xFF)) {
                //mp_rendezvous_no_intrs(init_cpu_turbo_counters, NULL);
                HWSensorsInfoLog("base CPU multiplier is %d", baseMultiplier);
                counters.update_perf_counters = true;
            }
            if (!addSensor(KEY_FAKESMC_CPU_PACKAGE_MULTIPLIER, TYPE_FP88, TYPE_FPXX_SIZE, kCPUSensorsMultiplierPackage, 0))
                HWSensorsWarningLog("failed to add package multiplier sensor");
            if (!addSensor(KEY_FAKESMC_CPU_PACKAGE_FREQUENCY, TYPE_UI32, TYPE_UI32_SIZE, kCPUSensorsFrequencyPackage, 0))
                HWSensorsWarningLog("failed to add package frequency sensor");
            break;

        case CPUFAMILY_INTEL_NEHALEM:
        case CPUFAMILY_INTEL_WESTMERE:
            if ((baseMultiplier = (rdmsr64(MSR_PLATFORM_INFO) >> 8) & 0xFF)) {
                HWSensorsInfoLog("base CPU multiplier is %d", baseMultiplier);
                counters.update_perf_counters = true;
            }
            // break; fall down adding multiplier sensors for each core

        default:
            for (uint32_t i = 0; i < availableCoresCount/*cpuid_info()->core_count*/; i++) {
                char key[5];
                
                snprintf(key, 5, KEY_FAKESMC_FORMAT_CPU_MULTIPLIER, i);
                
                if (!addSensor(key, TYPE_FP88, TYPE_FPXX_SIZE, kCPUSensorsMultiplierCore, i))
                    HWSensorsWarningLog("failed to add multiplier sensor");
                
                snprintf(key, 5, KEY_FAKESMC_FORMAT_CPU_FREQUENCY, i);
                
                if (!addSensor(key, TYPE_UI32, TYPE_UI32_SIZE, kCPUSensorsFrequencyCore, i))
                    HWSensorsWarningLog("failed to add frequency sensor");
                
            }
            break;
    }
    
    // energy consumption
    switch (cpuid_info()->cpuid_cpufamily) {            
        case CPUFAMILY_INTEL_SANDYBRIDGE:
        case CPUFAMILY_INTEL_IVYBRIDGE:
        case CPUFAMILY_INTEL_HASWELL:
        {
            UInt64 rapl = rdmsr64(MSR_RAPL_POWER_UNIT);
            
            UInt8 power_units = rapl & 0xf;
            UInt8 energy_units = (rapl >> 8) & 0x1f;
            UInt8 time_units = (rapl >> 16) & 0xf;
            
            HWSensorsDebugLog("RAPL units power: 0x%x energy: 0x%x time: 0x%x", power_units, energy_units, time_units);
            
            if (energy_units && (energyUnits = 1.0f / (float)(1 << energy_units))) {
                if (!addSensor(KEY_CPU_PACKAGE_TOTAL_POWER, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsPowerTotal, 0))
                    HWSensorsWarningLog("failed to add CPU package total power sensor");
                
                if (!addSensor(KEY_CPU_PACKAGE_CORE_POWER, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsPowerCores, 1))
                        HWSensorsWarningLog("failed to add CPU package cores power sensor");
                
                // Uncore sensor is only available on CPUs with uncore device (built-in GPU)
                if (cpuid_info()->cpuid_model != CPUID_MODEL_JAKETOWN && cpuid_info()->cpuid_model != CPUID_MODEL_IVYBRIDGE_EP) {
                    if (!addSensor(KEY_CPU_PACKAGE_GFX_POWER, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsPowerUncore, 2))
                        HWSensorsWarningLog("failed to add CPU package uncore power sensor");
                }
                
                switch (cpuid_info()->cpuid_cpufamily) {
                    case CPUFAMILY_INTEL_HASWELL:
                        // TODO: check DRAM availability for other platforms
                        if (cpuid_info()->cpuid_cpufamily != CPUFAMILY_INTEL_SANDYBRIDGE) {
                            if (!addSensor(KEY_CPU_PACKAGE_DRAM_POWER, TYPE_SP78, TYPE_SPXX_SIZE, kCPUSensorsPowerDram, 3))
                                HWSensorsWarningLog("failed to add CPU package DRAM power sensor");
                        }
                }
            }
            break;
        }
            
    }
    
    // two power states - off and on
	static const IOPMPowerState powerStates[2] = {
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, IOPMDeviceUsable, IOPMPowerOn, IOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    // register interest in power state changes
	PMinit();
	provider->joinPMtree(this);
	registerPowerDriver(this, (IOPMPowerState *)powerStates, 2);

    // Register service
    registerService();

    timerEventSource->setTimeoutMS(1000);

    HWSensorsInfoLog("started");

    return true;
}

IOReturn CPUSensors::setPowerState(unsigned long powerState, IOService *device)
{
    void *magic;

	switch (powerState) {
        case 0: // Power Off
                //timerEventSource->cancelTimeout();
            break;

        case 1: // Power On
                //timerEventSource->setTimeoutMS(1000);
            if (baseMultiplier > 0) {
                mp_rendezvous_no_intrs(init_cpu_turbo_counters, &magic);
            }
            break;

        default:
            break;
    }

	return(IOPMAckImplied);
}

void CPUSensors::stop(IOService *provider)
{
    PMstop();
    
    timerEventSource->cancelTimeout();

    if (IOWorkLoop *workloop = getWorkLoop()) {
        workloop->removeEventSource(timerEventSource);
    }
    
    super::stop(provider);
}

void CPUSensors::free()
{
    super::free();
}