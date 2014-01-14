//
//  PopupFanCell.m
//  HWMonitor
//
//  Created by Kozlek on 14.12.13.
//  Copyright (c) 2013 kozlek. All rights reserved.
//

#import "PopupFanCell.h"
#import "HWMSmcFanSensor.h"
#import "Localizer.h"
#import "HWMEngine.h"
#import "HWMConfiguration.h"
#import "HWMColorTheme.h"
#import "NSPopover+Message.h"

@implementation PopupFanController

@synthesize objectValue;

#define ROUND_50(x) (((int)x / 50) * 50)

-(void)awakeFromNib
{
    float min = [[self.objectValue valueForKey:@"min"] floatValue];
    float max = [[self.objectValue valueForKey:@"max"] floatValue];
    float speed = [[self.objectValue valueForKey:@"speed"] floatValue];

    NSInteger rounded = ROUND_50(speed);

    [_targetSlider setMinValue:min];
    [_targetSlider setMaxValue:max];
    [_targetSlider setIntegerValue:rounded];

    [_targetTextField setIntegerValue:rounded];
    [_targetTextField setFont:[NSFont fontWithName:@"Let's go Digital Regular" size:20]];

    COICOPopoverView *container = (COICOPopoverView *)[self view];

    [container setBackgroundColour:self.colorTheme.useDarkIcons.boolValue ?
     [self.colorTheme.listBackgroundColor highlightWithLevel:0.30] :
     nil /*[self.colorTheme.listBackgroundColor shadowWithLevel:0.05]*/];

    NSColor *textColor = self.colorTheme.useDarkIcons.boolValue ?
    self.colorTheme.itemValueTitleColor :
    [self.colorTheme.itemValueTitleColor highlightWithLevel:0.35];

    [_targetTextField setTextColor:textColor];
}

- (void)sliderHasMoved:(id)sender
{
    [_targetTextField setIntegerValue:ROUND_50(_targetSlider.integerValue)];
     
    SEL sel = @selector(sliderHasBeenReleased:);
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:sel object:sender];
    
    [self performSelector:sel withObject:sender afterDelay:0.0];
}

- (void)sliderHasBeenReleased:(id)sender
{
    [(HWMSmcFanSensor *)self.objectValue setSpeed:[NSNumber numberWithInteger:ROUND_50(_targetSlider.integerValue)]];
}

@end;

@implementation PopupFanCell

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];

    for (NSTrackingArea *area in [self trackingAreas]) {
		[self removeTrackingArea:area];
    }

    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways;

    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil]];
}

-(void)mouseEntered:(NSEvent *)theEvent
{
    if ([[self.objectValue engine] configuration].enableFanControl.boolValue && _popover == nil) {
        [self performSelector:@selector(showFanController:) withObject:self afterDelay:0.5];
    }
}

-(void)mouseDown:(NSEvent *)theEvent
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

//    if ([[self.objectValue engine] configuration].enableFanControl.boolValue) {
//        [self performSelector:@selector(showFanController:) withObject:self afterDelay:0.0];
//    }

    [super mouseDown:theEvent];
}

-(void)mouseExited:(NSEvent *)theEvent
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    
    [super mouseExited:theEvent];
}

-(void)popoverDidClose:(NSNotification *)notification
{
    _popover = nil;
}

-(void)showFanController:(id)sender
{
    HWMSmcFanSensor *fan = self.objectValue;

    if ([fan number] && [fan min] && [fan max] && [fan speed]) {

        [PopupSensorCell destroyGlobalPopover];

        PopupFanController *controller = [[PopupFanController alloc] initWithNibName:@"PopupFanController" bundle:[NSBundle mainBundle]];

        [controller setObjectValue:self.objectValue];
        [controller setColorTheme:self.colorTheme];

        [Localizer localizeView:controller.view];

        _popover = [PopupSensorCell globalPopover];

        [_popover setContentViewController:controller];
        [_popover setAnimates:YES];
        [_popover setBehavior:NSPopoverBehaviorTransient];
        [_popover showRelativeToRect:self.frame ofView:self preferredEdge:NSMinXEdge];
        [_popover setDelegate:self];
    }
}

@end
