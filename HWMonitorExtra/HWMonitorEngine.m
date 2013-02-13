//
//  HWMonitorEngine.m
//  HWSensors
//
//  Created by kozlek on 23/02/12.
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


#include "HWMonitorEngine.h"

#include "smc.h"
#include "FakeSMCDefinitions.h"

@implementation HWMonitorEngine

#define GetLocalizedString(key) \
[_bundle localizedStringForKey:(key) value:@"" table:nil]

@synthesize bundle = _bundle;

@synthesize sensors = _sensors;
@synthesize keys = _keys;

@synthesize useFahrenheit = _useFahrenheit;
@synthesize useBSDNames = _useBSDNames;

+ (HWMonitorEngine*)engineWithBundle:(NSBundle*)bundle;
{
    HWMonitorEngine *me = [[HWMonitorEngine alloc] init];
    
    if (me) {
        [me setBundle:bundle];
        [me rebuildSensorsList];
    }
    
    return me;
}

+ (NSString*)copyTypeFromKeyInfo:(NSArray*)info
{
    if (info && [info count] == 2) {
        //NSString *type = (NSString*)[info objectAtIndex:0];
        
        //NSLog(@"%s", [type cStringUsingEncoding:NSASCIIStringEncoding]);
        
        return [NSString stringWithString:(NSString*)[info objectAtIndex:0]];
    }
    
    return nil;
}

+ (NSData*)copyValueFromKeyInfo:(NSArray*)info
{
    if (info && [info count] == 2)
        return [NSData dataWithData:(NSData *)[info objectAtIndex:1]];
    
    return nil;
}

-(void)setUseBSDNames:(BOOL)useBSDNames
{
    if (_useBSDNames != useBSDNames) {
        _useBSDNames = useBSDNames;
        
        for (HWMonitorSensor *sensor in [self sensors])
            if ([sensor disk])
                [sensor setTitle:_useBSDNames ? [[sensor disk] bsdName] : [[[sensor disk] productName] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]]];
    }
}

-(BOOL)useBSDNames
{
    return _useBSDNames;
}

- (NSArray*)populateInfoForKey:(NSString*)key
{
    NSArray * info = nil;
    
    if (_connection || kIOReturnSuccess == SMCOpen(&_connection)) {
        SMCVal_t val;
        UInt32Char_t name;
        
        strncpy(name, [key cStringUsingEncoding:NSASCIIStringEncoding], 5);
        
        if (kIOReturnSuccess == SMCReadKey(_connection, name, &val)) {
            info = [NSArray arrayWithObjects:
                    [NSString stringWithCString:val.dataType encoding:NSASCIIStringEncoding],
                    [NSData dataWithBytes:val.bytes length:val.dataSize],
                    nil];
        }
        
        //SMCClose(_connection);
        //_connection = 0;
    }
    
    return info;
}

- (HWMonitorSensor*)addSensorWithKey:(NSString*)key title:(NSString*)title group:(NSUInteger)group
{
    HWMonitorSensor *sensor = nil;
    NSString *type = nil;
    NSData *value = nil;
    //BOOL smartSensor = FALSE;
    
    switch (group) {
        case kSMARTSensorGroupTemperature:
        case kSMARTSensorGroupRemainingLife:
        case kSMARTSensorGroupRemainingBlocks:
            //smartSensor = TRUE;
            break;
            
        default: {
            NSArray *info = [self populateInfoForKey:key];
            
            if (!info || [info count] != 2)
                return nil;
            
            type = [HWMonitorEngine copyTypeFromKeyInfo:info];
            value = [HWMonitorEngine copyValueFromKeyInfo:info];
            
            if (!type || [type length] == 0 || !value)
                return nil;
            
            switch (group) {
                case kHWSensorGroupTemperature:
                    [sensor setLevel:kHWSensorLevelDisabled];
                    break;
                default:
                    [sensor setLevel:kHWSensorLevelUnused];
                    break;
            }
            
            break;
        }
    }
    
    sensor = [HWMonitorSensor sensor];
    
    [sensor setEngine:self];
    [sensor setName:key];
    [sensor setType:type];
    [sensor setTitle:title];
    [sensor setData:value];
    [sensor setGroup:group];
    
    /*if (!smartSensor && _hideDisabledSensors && [[sensor value] isEqualToString:@"-"]) {
        [sensor setEngine:nil];
        sensor = nil;
        return nil;
    }*/
    
    [_sensors addObject:sensor];
    [_keys setObject:sensor forKey:key];
        
    return sensor;
}

- (HWMonitorSensor*)addSMARTSensorWithGenericDisk:(ATAGenericDisk*)disk group:(NSUInteger)group
{
    NSData * value = nil;
    
    switch (group) {
        case kSMARTSensorGroupTemperature:
            value = [disk getTemperature];

            UInt16 t = 0;
            
            [value getBytes:&t length:2];
            
            // Don't add sensor if value is insane
            if (t == 0 || t > 99)
                return nil;
            
            break;
            
        case kSMARTSensorGroupRemainingLife:
            value = [disk getRemainingLife];
            
            UInt64 life = 0;
            
            [value getBytes:&life length:[value length]];
            
            if (life > 100)
                return nil;
            
            break;
            
        case kSMARTSensorGroupRemainingBlocks:
            value = [disk getRemainingBlocks];
            
            UInt64 blocks = 0;
            
            [value getBytes:&blocks length:[value length]];
            
            if (blocks == 0xffffffffffff)
                return nil;
            
            break;
    }
    
    if (value) {
        HWMonitorSensor *sensor = [self addSensorWithKey:[NSString stringWithFormat:@"%@%lx", [disk serialNumber], group] title:_useBSDNames ? [disk bsdName] : [[disk productName] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]] group:group];
        
        [sensor setData:value];
        [sensor setDisk:disk];
        if ([disk isExceeded]) [sensor setLevel:kHWSensorLevelExceeded];
        
        return sensor;
    }
    
    return nil;
}

- (id)init;
{
    self = [super init];
    
    _smartReporter = [NSATASmartReporter smartReporterByDiscoveringDrives];
    _sensors = [[NSMutableArray alloc] init];
    _keys = [[NSMutableDictionary alloc] init];
    _bundle = [NSBundle mainBundle];
    _sensorsLock = [[NSLock alloc] init];
    
    return self;
}

- (id)initWithBundle:(NSBundle*)mainBundle;
{
    self = [super init];
    
    _smartReporter = [NSATASmartReporter smartReporterByDiscoveringDrives];
    _sensors = [[NSMutableArray alloc] init];
    _keys = [[NSMutableDictionary alloc] init];
    _bundle = mainBundle;
    _sensorsLock = [[NSLock alloc] init];
    
    return self;
}

- (void)dealloc
{
    if (_connection) {
        SMCClose(_connection);
        _connection = 0;
    }
}

- (void)rebuildSensorsList
{
    [_sensorsLock lock];
    
    [_sensors removeAllObjects];
    [_keys removeAllObjects];
    
    //Temperatures
    
    for (int i=0; i<0xf; i++) {
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_CPU_DIODE_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"CPU Core %X"),i + 1] group:kHWSensorGroupTemperature];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_CPU_ANALOG_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"CPU Core %X"),i + 1] group:kHWSensorGroupTemperature];
    }
    
    [self addSensorWithKey:@KEY_CPU_PACKAGE_TEMPERATURE title:GetLocalizedString(@"CPU Core Package") group:kHWSensorGroupTemperature];
    [self addSensorWithKey:@KEY_CPU_HEATSINK_TEMPERATURE title:GetLocalizedString(@"CPU Heatsink") group:kHWSensorGroupTemperature];
    [self addSensorWithKey:@KEY_CPU_PROXIMITY_TEMPERATURE title:GetLocalizedString(@"CPU Proximity") group:kHWSensorGroupTemperature];
    
    [self addSensorWithKey:@KEY_NORTHBRIDGE_TEMPERATURE title:GetLocalizedString(@"Northbridge") group:kHWSensorGroupTemperature];
    for (int i=1; i<0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_NORTHBRIDGE_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"Northbridge %X"),i + 1] group:kHWSensorGroupTemperature];
    
    [self addSensorWithKey:@KEY_PCH_DIE_TEMPERATURE title:GetLocalizedString(@"Platform Controller Hub") group:kHWSensorGroupTemperature];
    [self addSensorWithKey:@KEY_MCH_DIODE_TEMPERATURE title:GetLocalizedString(@"Memory Controller Hub") group:kHWSensorGroupTemperature];
    
    [self addSensorWithKey:@KEY_AMBIENT_TEMPERATURE title:GetLocalizedString(@"Ambient") group:kHWSensorGroupTemperature];
    for (int i=1; i<0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_AMBIENT_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"Ambient %X"),i + 1] group:kHWSensorGroupTemperature];
    
    for (int i=1; i<0x4; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_DIMM_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"DIMM %X"),i + 1] group:kHWSensorGroupTemperature];
    
    // GPU
    [self addSensorWithKey:@KEY_GPU_DIODE_TEMPERATURE title:GetLocalizedString(@"GPU Core") group:kHWSensorGroupTemperature];
    [self addSensorWithKey:@KEY_GPU_HEATSINK_TEMPERATURE title:GetLocalizedString(@"GPU Heatsink") group:kHWSensorGroupTemperature];
    [self addSensorWithKey:@KEY_GPU_PROXIMITY_TEMPERATURE title:GetLocalizedString(@"GPU") group:kHWSensorGroupTemperature];
    for (int i=1; i<0xf; i++) {
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_GPU_DIODE_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Core"),i + 1] group:kHWSensorGroupTemperature];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_GPU_HEATSINK_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Heatsink"),i + 1] group:kHWSensorGroupTemperature];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_GPU_PROXIMITY_TEMPERATURE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X"),i + 1] group:kHWSensorGroupTemperature];
    }
    
    if ([_smartReporter drives]) {
        for (int i = 0; i < [[_smartReporter drives] count]; i++) {
            ATAGenericDisk * disk = [[_smartReporter drives] objectAtIndex:i];
            
            if (disk) { 
                // Hard Drive Temperatures
                [self addSMARTSensorWithGenericDisk:disk group:kSMARTSensorGroupTemperature];
                
                if (![disk isRotational]) {
                    // SSD Remaining Life
                    [self addSMARTSensorWithGenericDisk:disk group:kSMARTSensorGroupRemainingLife];
                    // SSD Remaining Blocks
                    [self addSMARTSensorWithGenericDisk:disk group:kSMARTSensorGroupRemainingBlocks];
                }
            }
        }
    }
    
    // Multipliers
    for (int i=0; i<0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_CPU_MULTIPLIER,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"CPU Core %X Multiplier"),i + 1] group:kHWSensorGroupMultiplier];
    
    [self addSensorWithKey:@KEY_FAKESMC_CPU_PACKAGE_MULTIPLIER title:GetLocalizedString(@"CPU Package Multiplier") group:kHWSensorGroupMultiplier];
    
    //Frequencies
    for (int i=0; i<0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_CPU_FREQUENCY,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"CPU Core %X"),i + 1] group:kHWSensorGroupFrequency];
    
    [self addSensorWithKey:@KEY_FAKESMC_CPU_PACKAGE_FREQUENCY title:GetLocalizedString(@"CPU Package") group:kHWSensorGroupFrequency];
    
    [self addSensorWithKey:@KEY_FAKESMC_GPU_FREQUENCY title:GetLocalizedString(@"GPU Core") group:kHWSensorGroupFrequency];
    [self addSensorWithKey:@KEY_FAKESMC_GPU_SHADER_FREQUENCY title:GetLocalizedString(@"GPU Shaders") group:kHWSensorGroupFrequency];
    [self addSensorWithKey:@KEY_FAKESMC_GPU_ROP_FREQUENCY title:GetLocalizedString(@"GPU ROP") group:kHWSensorGroupFrequency];
    [self addSensorWithKey:@KEY_FAKESMC_GPU_MEMORY_FREQUENCY title:GetLocalizedString(@"GPU Memory") group:kHWSensorGroupFrequency];
    for (int i=1; i<0xf; i++) {
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPU_FREQUENCY,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Core"),i + 1] group:kHWSensorGroupFrequency];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPU_SHADER_FREQUENCY,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Shaders"),i + 1] group:kHWSensorGroupFrequency];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPU_ROP_FREQUENCY,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X ROP"),i + 1] group:kHWSensorGroupFrequency];
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPU_MEMORY_FREQUENCY,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Memory"),i + 1] group:kHWSensorGroupFrequency];
    }
    
    // Fans
    for (int i=0; i<0xf; i++) {
        NSString * caption = [[NSString alloc] initWithData:[HWMonitorEngine copyValueFromKeyInfo:[self populateInfoForKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_FAN_ID,i]]] encoding: NSUTF8StringEncoding];
        
        if ([caption length] == 0)
            caption = [[NSString alloc] initWithFormat:GetLocalizedString(@"Fan %X"),i + 1];
        
        if (![caption hasPrefix:@"GPU "])
            [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_FAN_SPEED,i] title:GetLocalizedString(caption) group:kHWSensorGroupTachometer];
    }
    
    // GPU Fans
    for (int i=0; i < 0xf; i++) {
        NSString * caption = [[NSString alloc] initWithData:[HWMonitorEngine copyValueFromKeyInfo:[self populateInfoForKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_FAN_ID,i]]] encoding: NSUTF8StringEncoding];
        
        if ([caption hasPrefix:@"GPU "]) {
            UInt8 cardIndex = [[caption substringFromIndex:4] intValue] - 1;
            
            if (cardIndex==0) {
                [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPUPWM,cardIndex] title:GetLocalizedString(@"GPU PWM") group:kHWSensorGroupPWM];
                [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_FAN_SPEED,i] title:GetLocalizedString(@"GPU Fan") group:kHWSensorGroupTachometer];
            }
            else {
                [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FAKESMC_FORMAT_GPUPWM,cardIndex] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X PWM"),cardIndex] group:kHWSensorGroupPWM];
                [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_FAN_SPEED,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X Fan"),cardIndex] group:kHWSensorGroupTachometer];
            }
        }
    }
    
    // Voltages
    [self addSensorWithKey:@KEY_CPU_VCORE_VOLTAGE title:GetLocalizedString(@"CPU Vcore") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_CPU_1V5_S0_VOLTAGE title:GetLocalizedString(@"CPU 1.5V S0") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_CPU_1V5_S0_VOLTAGE title:GetLocalizedString(@"CPU 1.8V S0") group:kHWSensorGroupVoltage];
    
    [self addSensorWithKey:@KEY_CPU_VOLTAGE title:GetLocalizedString(@"CPU") group:kHWSensorGroupVoltage];
    for (int i = 1; i <= 0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_CPU_VOLTAGE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"CPU %X"),i + 1] group:kHWSensorGroupVoltage];
    
    [self addSensorWithKey:@KEY_MEMORY_VOLTAGE title:GetLocalizedString(@"Memory") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_MAIN_3V3_VOLTAGE title:GetLocalizedString(@"Main 3.3V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_AUXILIARY_3V3V_VOLTAGE title:GetLocalizedString(@"Auxiliary 3.3V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_MAIN_5V_VOLTAGE title:GetLocalizedString(@"Main 5V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_STANDBY_5V_VOLTAGE title:GetLocalizedString(@"Standby 5V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_MAIN_12V_VOLTAGE title:GetLocalizedString(@"Main 12V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_PCIE_12V_VOLTAGE title:GetLocalizedString(@"PCIe 12V") group:kHWSensorGroupVoltage];
    [self addSensorWithKey:@KEY_POWERBATTERY_VOLTAGE title:GetLocalizedString(@"CMOS Battery") group:kHWSensorGroupVoltage];
    
    [self addSensorWithKey:@KEY_CPU_VRMSUPPLY_VOLTAGE title:GetLocalizedString(@"VRM Supply") group:kHWSensorGroupVoltage];
    for (int i = 1; i <= 0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_CPU_VRMSUPPLY_VOLTAGE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"VRM Supply %X"),i + 1] group:kHWSensorGroupVoltage];
    
    [self addSensorWithKey:@KEY_POWERSUPPLY_VOLTAGE title:GetLocalizedString(@"Power Supply") group:kHWSensorGroupVoltage];
    for (int i = 1; i <= 0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_POWERSUPPLY_VOLTAGE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"Power Supply %X"),i + 1] group:kHWSensorGroupVoltage];
    
    [self addSensorWithKey:@KEY_GPU_VOLTAGE title:GetLocalizedString(@"GPU") group:kHWSensorGroupVoltage];
    for (int i = 1; i <= 0xf; i++)
        [self addSensorWithKey:[[NSString alloc] initWithFormat:@KEY_FORMAT_GPU_VOLTAGE,i] title:[[NSString alloc] initWithFormat:GetLocalizedString(@"GPU %X"),i + 1] group:kHWSensorGroupVoltage];
    
    [_sensorsLock unlock];
}

- (NSArray*)updateSmartSensors
{
    [_sensorsLock lock];
    
    NSMutableArray *list = [[NSMutableArray alloc] init];
    
    for (HWMonitorSensor *sensor in [self sensors]) {
        if ([sensor disk]) {
            switch ([sensor group]) {
                case kSMARTSensorGroupTemperature:
                    [sensor setData:[[sensor disk] getTemperature]];
                    [list addObject:sensor];
                    break;
                    
                case kSMARTSensorGroupRemainingLife:
                    [sensor setData:[[sensor disk] getRemainingLife]];
                    [list addObject:sensor];
                    break;
                    
                case kSMARTSensorGroupRemainingBlocks:
                    [sensor setData:[[sensor disk] getRemainingBlocks]];
                    [list addObject:sensor];
                    break;
                        
                default:
                    break;
            }
        }
    }
    
    [_sensorsLock unlock];
    
    return list;
}

- (NSArray*)updateSmcSensors
{
    [_sensorsLock lock];
    
    NSMutableArray *list = [[NSMutableArray alloc] init];
    
    for (HWMonitorSensor *sensor in [self sensors]) {
        if (![sensor disk])
            [list addObject:sensor];
    }
    
    if (_connection || kIOReturnSuccess == SMCOpen(&_connection)) {
        for (HWMonitorSensor *sensor in list) {
            SMCVal_t val;
            UInt32Char_t name;
            
            strncpy(name, [[sensor name] cStringUsingEncoding:NSASCIIStringEncoding], 5);
            
            if (kIOReturnSuccess == SMCReadKey(_connection, name, &val)) {
                [sensor setType:[NSString stringWithCString:val.dataType encoding:NSASCIIStringEncoding]];
                [sensor setData:[NSData dataWithBytes:val.bytes length:val.dataSize]];
            }
        }
    }
    
    [_sensorsLock unlock];
    
    return list;
}

-(NSArray*)updateSmcSensorsList:(NSArray *)sensors
{
    if (!sensors) return nil; // [self updateSmcSensors];
    
    [_sensorsLock lock];
    
    NSMutableArray *list = [[NSMutableArray alloc] init];
    
    for (id object in sensors) {
        if ([object isKindOfClass:[HWMonitorSensor class]] && [[self sensors] containsObject:object] && ![object disk])
            [list addObject:object];
    }
    
    if (_connection || kIOReturnSuccess == SMCOpen(&_connection)) {
        for (HWMonitorSensor *sensor in list) {
            SMCVal_t val;
            UInt32Char_t name;
            
            strncpy(name, [[sensor name] cStringUsingEncoding:NSASCIIStringEncoding], 5);
            
            if (kIOReturnSuccess == SMCReadKey(_connection, name, &val)) {
                [sensor setType:[NSString stringWithCString:val.dataType encoding:NSASCIIStringEncoding]];
                [sensor setData:[NSData dataWithBytes:val.bytes length:val.dataSize]];
            }
        }
    }
    
    [_sensorsLock unlock];
    
    return list;
}

- (NSArray*)getAllSensorsInGroup:(NSUInteger)group
{
    [_sensorsLock lock];
    
    NSMutableArray * list = [[NSMutableArray alloc] init];
    
    for (HWMonitorSensor *sensor in [self sensors])
        if (group & [sensor group])
            [list addObject:sensor];
    
    [_sensorsLock unlock];
    
    return [list count] > 0 ? [NSArray arrayWithArray:list] : nil;
}

@end
