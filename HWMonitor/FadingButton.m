//
//  FadingButton.m
//  HWMonitor
//
//  Created by kozlek on 28.03.13.
//  Copyright (c) 2013 kozlek. All rights reserved.
//

#import "FadingButton.h"

#define NORMAL_OPACITY  0.8
#define HOVER_OPACITY   0.95
#define DOWN_OPACITY    0.6

@implementation FadingButton

- (void)fadeIn:(id)sender
{
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.1];
    [[self animator] setAlphaValue:HOVER_OPACITY];
    [NSAnimationContext endGrouping];
}

- (void)fadeOut:(id)sender
{
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:0.1];
    [[self animator] setAlphaValue:NORMAL_OPACITY];
    [NSAnimationContext endGrouping];
}

- (id)init
{
    self = [super init];
    
    if (self) {
        [self setAlphaValue:NORMAL_OPACITY];
    }
    
    return self;
}

- (void)dealloc
{
    for (NSTrackingArea *area in [self trackingAreas]) {
		[self removeTrackingArea:area];
    }
}

- (void)awakeFromNib
{
    [self setAlphaValue:NORMAL_OPACITY];
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    
    for (NSTrackingArea *area in [self trackingAreas]) {
		[self removeTrackingArea:area];
    }
    
    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow;
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil]];
}

-(void)mouseEntered:(NSEvent *)theEvent
{
    [self setAlphaValue:HOVER_OPACITY];
    
    [super mouseEntered:theEvent];
}

-(void)mouseExited:(NSEvent *)theEvent
{
    [self setAlphaValue:NORMAL_OPACITY];
    
    [super mouseExited:theEvent];
}

-(void)mouseDown:(NSEvent *)theEvent
{
    [self setAlphaValue:DOWN_OPACITY];

    [super mouseDown:theEvent];
    
    [self setAlphaValue:NORMAL_OPACITY];
    
    if (self.menu) {
        NSRect frame = [self convertRect:self.bounds toView:nil];
        
        NSEvent *event = [NSEvent
                          mouseEventWithType:NSRightMouseDown
                          location: NSMakePoint(frame.origin.x - self.bounds.size.width / 2, frame.origin.y - self.bounds.size.height / 2)
                          modifierFlags: theEvent.modifierFlags
                          timestamp: theEvent.timestamp
                          windowNumber:theEvent.windowNumber
                          context:theEvent.context
                          eventNumber:theEvent.eventNumber
                          clickCount:theEvent.clickCount
                          pressure:theEvent.pressure];
        [NSMenu
         popUpContextMenu:self.menu
         withEvent:event
         forView:self];
    }
}

@end
