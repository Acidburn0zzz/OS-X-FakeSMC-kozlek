//
//  PopupAtaSmartReportControllerViewController.m
//  HWMonitor
//
//  Created by Kozlek on 07.02.14.
//  Copyright (c) 2014 kozlek. All rights reserved.
//

#import "PopupAtaSmartReportController.h"

#import "NSPopover+Message.h"
#import "HWMEngine.h"
#import "HWMConfiguration.h"
#import "HWMColorTheme.h"

#import "Localizer.h"

@interface PopupAtaSmartReportController ()

@end

@implementation PopupAtaSmartReportController

-(void)setSensor:(HWMAtaSmartSensor *)sensor
{
    _sensor = sensor;

    COICOPopoverView *container = (COICOPopoverView *)[self view];

    [container setBackgroundColour:self.sensor.engine.configuration.colorTheme.useDarkIcons.boolValue ?
     [self.sensor.engine.configuration.colorTheme.listBackgroundColor colorWithAlphaComponent:0.85] :
     nil /*[self.colorTheme.listBackgroundColor shadowWithLevel:0.05]*/];

}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];

    if (self) {

    }

    return self;
}

-(void)awakeFromNib
{
    [Localizer localizeView:_tableView];
}

- (void)copy:(id)sender;
{
    NSArray	*selectedObjects = [self.arrayController selectedObjects];

    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];

	[pasteboard clearContents];

    NSMutableString *entry = [[NSMutableString alloc] init];

    // Copy column names
    [_tableView.tableColumns enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSCell *headerCell = [obj headerCell];

        if (headerCell.title) {
            [entry appendString:headerCell.title];

            if (idx < _tableView.tableColumns.count - 1) {
                [entry appendString:@", "];
            }
        }
    }];

    if (![pasteboard writeObjects:@[entry]]) {
        NSBeep();
        return;
    }

    // Copy attributes
	for (NSDictionary *item in selectedObjects ) {

        entry = [NSString stringWithFormat:@"%d, %@, %@, %d, %d, %d, %@",
                           [item[@"id"] unsignedCharValue],
                           item[@"name"],
                           item[@"critical"],
                           [item[@"value"] unsignedShortValue],
                           [item[@"worst"] unsignedShortValue],
                           [item[@"threshold"] unsignedShortValue],
                           item[@"raw"]];

		if (![pasteboard writeObjects:@[entry]]) {
            NSBeep();
            break;
        }
    }
}

@end
