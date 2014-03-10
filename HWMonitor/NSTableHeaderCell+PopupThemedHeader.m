//
//  NSTableHeaderCell+PopupThemedHeader.m
//  HWMonitor
//
//  Created by Kozlek on 08.02.14.
//  Copyright (c) 2014 kozlek. All rights reserved.
//

#import "NSTableHeaderCell+PopupThemedHeader.h"
#import "HWMColorTheme.h"
#import "HWMEngine.h"
#import "HWMConfiguration.h"

@implementation NSTableHeaderCell (PopupThemedHeader)

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    HWMColorTheme *colorTheme = [HWMEngine defaultEngine].configuration.colorTheme;

    if (colorTheme) {

        NSGradient *gradient = nil;

        if (colorTheme.useDarkIcons) {
            gradient = [[NSGradient alloc] initWithColorsAndLocations:
                        [colorTheme.groupStartColor shadowWithLevel:0.05], 0.1,
                        colorTheme.groupEndColor, 0.9, nil];
        }
        else {
            gradient = [[NSGradient alloc] initWithColorsAndLocations:
                        [colorTheme.groupStartColor highlightWithLevel:0.1], 0.1,
                        colorTheme.groupEndColor, 0.9, nil];
        }

        [gradient drawInRect:cellFrame angle:90];

        if (self.title && self.title.length) {
            NSMutableDictionary *attibutes = [[NSMutableDictionary dictionaryWithDictionary:[[self attributedStringValue]attributesAtIndex:0 effectiveRange:NULL]] mutableCopy];

            [attibutes setObject:colorTheme.itemValueTitleColor forKey:NSForegroundColorAttributeName];

            NSMutableParagraphStyle * paragraphStyle = [[NSMutableParagraphStyle alloc] init];

            [paragraphStyle setLineBreakMode:NSLineBreakByClipping];
            [paragraphStyle setAlignment:NSCenterTextAlignment];

            [attibutes setObject:paragraphStyle forKey:NSParagraphStyleAttributeName];

            [[self stringValue] drawInRect:cellFrame withAttributes:attibutes];
        }
    }
    else {
        [super drawWithFrame:cellFrame inView:controlView];
    }
}

@end
