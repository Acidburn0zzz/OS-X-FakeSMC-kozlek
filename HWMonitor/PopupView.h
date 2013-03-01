//
//  PopupView.h
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

#define ARROW_WIDTH         23
#define ARROW_HEIGHT        11
#define ARROW_OFFSET        4
#define CORNER_RADIUS       6.0f
#define LINE_THICKNESS      1.0f
#define FILL_OPACITY        0.95f
#define STROKE_OPACITY      1.0f

#import "ColorTheme.h"

@interface PopupView : NSView
{
    ColorTheme *_colorTheme;
}

@property (nonatomic, setter = setArrowPosition:) NSInteger arrowPosition;
@property (nonatomic, setter = setColorTheme:) ColorTheme *colorTheme;

@end
