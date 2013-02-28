//
//  PopupView.m
//  HWMonitor
//
//  Created by kozlek on 23.02.13.
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

#import "PopupView.h"
#import "HWMonitorDefinitions.h"

@implementation PopupView

- (void)drawRect:(NSRect)dirtyRect
{
    NSRect contentRect = NSInsetRect([self bounds], LINE_THICKNESS, LINE_THICKNESS);
    
    NSRect headerRect = contentRect;
    headerRect.size.height = kHWMonitorButtonsHeight + ARROW_HEIGHT - LINE_THICKNESS * 4; // Buttons row height
    headerRect.origin.y = contentRect.size.height - headerRect.size.height;
    NSBezierPath *headerPath = [NSBezierPath bezierPath];
    
    [headerPath moveToPoint:NSMakePoint(_arrowPosition, NSMaxY(headerRect))];
    [headerPath lineToPoint:NSMakePoint(_arrowPosition + ARROW_WIDTH / 2, NSMaxY(headerRect) - ARROW_HEIGHT)];
    [headerPath lineToPoint:NSMakePoint(NSMaxX(headerRect) - CORNER_RADIUS, NSMaxY(headerRect) - ARROW_HEIGHT)];
    
    NSPoint topRightCorner = NSMakePoint(NSMaxX(headerRect), NSMaxY(headerRect) - ARROW_HEIGHT);
    [headerPath curveToPoint:NSMakePoint(NSMaxX(headerRect), NSMaxY(headerRect) - ARROW_HEIGHT - CORNER_RADIUS)
         controlPoint1:topRightCorner controlPoint2:topRightCorner];
    
    [headerPath lineToPoint:NSMakePoint(NSMaxX(headerRect), NSMinY(headerRect) + CORNER_RADIUS)];
    
    NSPoint bottomRightCorner = NSMakePoint(NSMaxX(headerRect), NSMinY(headerRect));
   
    [headerPath lineToPoint:bottomRightCorner];
    [headerPath lineToPoint:NSMakePoint(NSMinX(headerRect), NSMinY(headerRect))];
    
    [headerPath lineToPoint:NSMakePoint(NSMinX(headerRect), NSMaxY(headerRect) - ARROW_HEIGHT - CORNER_RADIUS)];
    
    NSPoint topLeftCorner = NSMakePoint(NSMinX(headerRect), NSMaxY(headerRect) - ARROW_HEIGHT);
    [headerPath curveToPoint:NSMakePoint(NSMinX(headerRect) + CORNER_RADIUS, NSMaxY(headerRect) - ARROW_HEIGHT)
         controlPoint1:topLeftCorner controlPoint2:topLeftCorner];
    
    [headerPath lineToPoint:NSMakePoint(_arrowPosition - ARROW_WIDTH / 2, NSMaxY(headerRect) - ARROW_HEIGHT)];
    [headerPath closePath];
       
    [[[NSGradient alloc]
      initWithStartingColor:[NSColor colorWithCalibratedWhite:0.5 alpha:FILL_OPACITY]
      endingColor:[NSColor colorWithCalibratedWhite:0.1 alpha:FILL_OPACITY]]
     drawInBezierPath:headerPath angle:270] ;
    
    [NSGraphicsContext saveGraphicsState];
    
    NSBezierPath *contentPath = [NSBezierPath bezierPath];

    [contentPath moveToPoint:NSMakePoint(NSMaxX(headerRect), NSMinY(headerRect) - CORNER_RADIUS + LINE_THICKNESS * 4)];

    [contentPath lineToPoint:NSMakePoint(NSMaxX(contentRect), NSMinY(contentRect) + CORNER_RADIUS)];
     
    bottomRightCorner = NSMakePoint(NSMaxX(contentRect), NSMinY(contentRect));
    [contentPath curveToPoint:NSMakePoint(NSMaxX(contentRect) - CORNER_RADIUS, NSMinY(contentRect))
          controlPoint1:bottomRightCorner controlPoint2:bottomRightCorner];
     
    [contentPath lineToPoint:NSMakePoint(NSMinX(contentRect) + CORNER_RADIUS, NSMinY(contentRect))];
     
    [contentPath curveToPoint:NSMakePoint(NSMinX(contentRect), NSMinY(contentRect) + CORNER_RADIUS)
         controlPoint1:contentRect.origin controlPoint2:contentRect.origin];
    
    [contentPath lineToPoint:NSMakePoint(NSMinX(headerRect), NSMinY(headerRect) - CORNER_RADIUS + LINE_THICKNESS * 4)];
     
    [contentPath closePath];
    
    [[NSColor colorWithDeviceWhite:1 alpha:FILL_OPACITY] setFill];
    [contentPath fill];
        
    NSBezierPath *clip = [NSBezierPath bezierPathWithRect:[self bounds]];
    [clip appendBezierPath:headerPath];
    [clip appendBezierPath:contentPath];
    [clip addClip];
    
    [headerPath setLineWidth:LINE_THICKNESS * 2];
    [headerPath setFlatness:0.3];
    [[NSColor colorWithCalibratedWhite:0.3 alpha:1.0] setStroke];
    [headerPath stroke];
    
    [contentPath setLineWidth:LINE_THICKNESS * 2];
    [contentPath setFlatness:0.3];
    [[NSColor colorWithCalibratedWhite:1.0 alpha:1.0] setStroke];
    [contentPath stroke];
    
    [NSGraphicsContext restoreGraphicsState];
}

- (void)setArrowPosition:(NSInteger)value
{
    _arrowPosition = value;
    [self setNeedsDisplay:YES];
}


@end
