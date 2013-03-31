//
//  FadingButton.m
//  HWMonitor
//
//  Created by kozlek on 28.03.13.
//  Copyright (c) 2013 kozlek. All rights reserved.
//

#import "FadingButton.h"

#define NORMAL_OPACITY  0.85
#define HOVER_OPACITY   1.0
#define DOWN_OPACITY    0.7

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
}

@end
