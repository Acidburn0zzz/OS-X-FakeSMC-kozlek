//
//  GraphsController.m
//  HWMonitor
//
//  Created by kozlek on 24.02.13.
//  Copyright (c) 2013 kozlek. All rights reserved.
//

/*
 *  Copyright (c) 2013 Natan Zalkin <natan.zalkin@me.com>. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 */

#import "GraphsController.h"

#import "HWMonitorSensor.h"
#import "HWMonitorItem.h"
#import "HWMonitorGroup.h"
#import "GraphsView.h"
#import "SensorCell.h"

#import "HWMonitorDefinitions.h"

#define GetLocalizedString(key) \
[[NSBundle mainBundle] localizedStringForKey:(key) value:@"" table:nil]

@implementation GraphsController

-(void)setUseFahrenheit:(BOOL)useFahrenheit
{
    _useFahrenheit = useFahrenheit;
    
    [_graphsTableView reloadData];
    
    for (GraphsView *graphView in _graphViews) {
        [graphView setUseFahrenheit:useFahrenheit];
    }
}

-(void)setUseSmoothing:(BOOL)useSmoothing
{
    _useSmoothing = useSmoothing;
    
    for (GraphsView *graphView in _graphViews) {
        [graphView setUseSmoothing:useSmoothing];
    }
}

-(NSArray *)colorsList
{
    return _colorsList;
}

-(HWMonitorItem *)selectedItem
{
    if ([_graphsTableView selectedRow] >= 0 && [_graphsTableView selectedRow] < [_items count]) {
        return [_items objectAtIndex:[_graphsTableView selectedRow]];
    }
    
    return nil;
}

-(id)init
{
    self = [super init];
    
    if (self) {
        _colorsList = [[NSMutableArray alloc] init];
        
        NSColorList *list = [NSColorList colorListNamed:@"Crayons"];
        
        for (NSUInteger i = [[list allKeys] count] - 1; i != 0; i--) {
            NSString *key = [[list allKeys] objectAtIndex:i];
            NSColor *color = [list colorWithKey:key];
            double intensity = (color.redComponent + color.blueComponent + color.greenComponent) / 3.0;
            double red = [color redComponent];
            double green = [color greenComponent];
            double blue = [color blueComponent];
            BOOL blackAndWhite = red == green && red == blue && green == blue;
            
            if (intensity >= 0.335 && intensity <=0.900 && !blackAndWhite)
                [_colorsList addObject:color];
        }
    }
    
    return self;
}

-(void)addGraphForSensorGroup:(HWSensorGroup)sensorsGroup fromGroupsList:(NSArray*)groupsList withTitle:(NSString*)title
{
    NSMutableArray *sensorItems = [[NSMutableArray alloc] init];
    
    NSUInteger colorIndex = 2;
    
    for (HWMonitorGroup *group in groupsList) {
        for (HWMonitorItem *item in [group items]) {
            if ([[item sensor] group] & sensorsGroup) {
                NSColor *color = [_colorsList objectAtIndex:colorIndex++];
                
                if (colorIndex >= [_colorsList count])
                    colorIndex = 0;
                
                [item setColor:color];
                
                [sensorItems addObject:item];
            }
        }
    }
    
    if ([sensorItems count]) {
        [_items addObject:title];
        [_items addObjectsFromArray:sensorItems];
        
        GraphsView *graphView = [[GraphsView alloc] init];
        
        [graphView addItemsFromList:sensorItems forSensorGroup:sensorsGroup];
        [graphView setGraphsController:self];
        [graphView setSensorGroup:sensorsGroup];
        
        [graphView setUseFahrenheit:_useFahrenheit];
        [graphView setUseSmoothing:_useSmoothing];
    
        [_graphViews addObject:graphView];
    }
}

-(void)setupWithGroups:(NSArray *)groups
{
    if (!_items) {
        _items = [[NSMutableArray alloc] init];
    }
    else {
        [_items removeAllObjects];
    }
    
    if (!_graphViews) {
        _graphViews = [[NSMutableArray alloc] init];
    }
    else {
        [_graphViews removeAllObjects];
    }
    
    if (!_hiddenItems) {
        _hiddenItems = [[NSMutableArray alloc] initWithArray:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:kHWMonitorHiddenGraphsList]];
    }
    
    [self addGraphForSensorGroup:kHWSensorGroupTemperature | kSMARTGroupTemperature fromGroupsList:groups withTitle:@"TEMPERATURES"];
    [self addGraphForSensorGroup:kHWSensorGroupFrequency fromGroupsList:groups withTitle:@"FREQUENCIES"];
    [self addGraphForSensorGroup:kHWSensorGroupTachometer fromGroupsList:groups withTitle:@"FANS"];
    [self addGraphForSensorGroup:kHWSensorGroupVoltage fromGroupsList:groups withTitle:@"VOLTAGES"];
    [self addGraphForSensorGroup:kHWSensorGroupCurrent fromGroupsList:groups withTitle:@"CURRENTS"];
    [self addGraphForSensorGroup:kHWSensorGroupPower fromGroupsList:groups withTitle:@"POWERS"];
    
    [_graphsCollectionView setContent:_graphViews];
    
    [_graphsCollectionView setMinItemSize:NSMakeSize(0, 120)];
    [_graphsCollectionView setMaxItemSize:NSMakeSize(0, 0)];
    
    [[_graphsCollectionView content] enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        [[_graphsCollectionView itemAtIndex:idx] setView:obj];
    }];
    
    [_graphsTableView reloadData];
}

- (void)captureDataToHistoryNow
{
    if ([self.window isVisible]) {
        for (NSUInteger index = 0; index < [_items count]; index++) {
            
            id item = [_items objectAtIndex:index];
            
            if ([item isKindOfClass:[HWMonitorItem class]]) {
                id cell = [_graphsTableView viewAtColumn:0 row:index makeIfNecessary:NO];
                
                if (cell && [cell isKindOfClass:[SensorCell class]]) {
                    [[cell valueField] setStringValue:[[item sensor] formattedValue]];
                }
            }
        }
    }
    
    if ([self.window isVisible] || [self backgroundMonitoring]) {
        for (GraphsView *graphView in _graphViews) {
            [graphView captureDataToHistoryNow];
        }
    }
}

- (BOOL)checkItemIsHidden:(HWMonitorItem*)item
{
    return [_hiddenItems indexOfObject:[[item sensor] name]] != NSNotFound;
}

// Events

-(IBAction)graphsTableViewClicked:(id)sender
{
    for (id graphView in _graphViews) {
        [graphView setNeedsDisplay:YES];
    }
}

-(IBAction)graphsCheckButtonClicked:(id)sender
{
    if ([sender tag] >= 0 && [sender tag] < [_items count]) {
        HWMonitorItem *item = [_items objectAtIndex:[sender tag]];
        
        if ([sender state] == NSOnState) {
            [_hiddenItems removeObject:[[item sensor] name]];
        }
        else {
            [_hiddenItems addObject:[[item sensor] name]];
        }
        
        [[[NSUserDefaultsController sharedUserDefaultsController] defaults] setObject:_hiddenItems forKey:kHWMonitorHiddenGraphsList];
    }
    
    for (id graphView in _graphViews) {
        [graphView calculateGraphBoundsFindExtremes:YES];
    }
    
    [self graphsTableViewClicked:sender];
}

// NSTableView delegate

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [_items count];
}

-(CGFloat)tableView:(NSTableView *)tableView heightOfRow:(NSInteger)row
{
    return 18;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row
{
    return ![[_items objectAtIndex:row] isKindOfClass:[NSString class]];
}

//-(BOOL)tableView:(NSTableView *)tableView isGroupRow:(NSInteger)row
//{
//    return [[_items objectAtIndex:row] isKindOfClass:[NSString class]];
//}

-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    id item = [_items objectAtIndex:row];
    
    if ([item isKindOfClass:[NSString class]]) {
        NSTableCellView *groupCell = [tableView makeViewWithIdentifier:item owner:self];
        
        [[groupCell textField] setStringValue:GetLocalizedString(item)];
        
        return groupCell;
    }
    else if ([item isKindOfClass:[HWMonitorItem class]]) {
        SensorCell *sensorCell = nil;
        
        if ([[item sensor] group] & (kHWSensorGroupTemperature | kSMARTGroupTemperature)) {
            sensorCell = [tableView makeViewWithIdentifier:@"Temperature" owner:self];
        }
        else {
            sensorCell = [tableView makeViewWithIdentifier:@"Sensor" owner:self];
        }
        
        HWMonitorSensor *sensor = [item sensor];
        
        [[sensorCell textField] setStringValue:GetLocalizedString([sensor title])];
        [[sensorCell valueField] setStringValue:[sensor formattedValue]];
        
        if ([item color] == nil) {
            NSLog(@"No color for key %@", [sensor name]);
        }
        else [[sensorCell colorWell] setColor:[item color]];
        [[sensorCell checkBox] setState:![self checkItemIsHidden:item]];
        [[sensorCell checkBox] setTag:[_items indexOfObject:item]];
        
        [sensorCell setRepresentedObject:item];
        
        return sensorCell;
    }
    
    return nil;
}

@end
