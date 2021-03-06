% TALARIA(1)
% Aetnaeus
% December 2018

# NAME

talaria - a simple line oriented X11 menu

# SYNOPSIS 

talaria [-d delim] [-f field] [-m] [-x xoff] [-y yoff] [-w width] [-h height] 
[-u unit height] [-b border size] [-l history file] [-v] [generator program]

# DESCRIPTION

talaria is a program which produces a simple dynamically filterable menu from
lines in STDIN and prints the chosen option to STDOUT. It is designed to be
flexible and provides options for controlling menu placement, color themes, and
delimeters. The delimeter and field options how talaria should extract the menu
options it displays during the selection process, the full corresponding line
from STDIN is printed on STDOUT after the item has been selected.

# OPTIONS

**-d** *delim* 
: Use *delim* instead of the NULL byte as the delimeter.

**-f** *field*
: Use the *field*th item on each input line (separated by *delim*) as the visible menu item. 

**-m**
: Do not allow arbitrary input (which is possible by default) (mnemonic *m*andatory)

**-r**
: Matching interprets the input as a regular expression.

**-x** *xoff*
: The x offset of the talaria window, this overrides whatever is in the config file if specified.
 If *xoff* is negative it is interpreted as being *xoff* units from the right edge of the screen. -0
 will place the window at the right edge of the screen.
 
**-y** *yoff*
: The y offset of the talaria window, this overrides whatever is in the config file if specified.
 If *yoff* is negative it is interpreted as being *yoff* units from the bottom of the screen. -0 
 will place the window at the bottom of the screen.

**-w** *width*
: Specifies the width of the menu window, overrides the default config file value.

**-h** *height*
: Specifies the height of the menu window, overrides the default config file value.

**-u** *unit height*
: Specifies the height of each menu item as well as the input bar. 

**-b** *border size*
: Specifies the width of the border, overrides the default config file value.

**-l** *history file*
: Specifies a file which will be used to record the chosen menu entry.
If the file is not empty (i.e from a previous invocation) its contents 
are used to populate the menu history as well as item order.

**-v**
: Prints version information.

# Config Options

The appearance of talaria is fully configurable and can be modfied by
overriding the values listed below in **~/.talariarc**.  All non color values
are numbers which represent pixels. Colors are of the form #XXXXXXXX or #XXXXXX
the latter is a standard RGB hex color, the former adds a transparency
dimension (where 0 is transparent and 255 is opaque).

*border_sz*: Thickness of the border in pixels. \
*x*: X offset. \
*y*: Y offset. \
*w*: Width. \
*h*: Height. \
*input_separator_height*: The height of the bar between the main input field and the menu options. \
*input_height*: The height of the input field as well as all menu items (same as unit height). \
*border_radius*: Border radius. \
*border_color*: Border color. \
*background_color*: Background color. \
*foreground_color*: Foreground color. \
*input_separator_color*: Input separator. \
*cursor_color*: Cursor color. \
*font_family*: Xft font family. \
*menu_divider_height*: Height of the bar which separates menu items. \
*menu_sel_background_color*: Background color of the selected menu item. \
*menu_sel_foreground_color*: Foreground color of the selected menu item.

# SCRIPTING PROTOCOL

Talaria supports advanced scripting using a simple line based stdin/stdout
protocol. A program consumes lines from standard input indicating how talaria
should behave and produces the results on standard output to affect program
behavior in real time. This allows programs and scripts to dictate menu items
at run time using traditional streams without requiring complex socket
manipulation or a dedicated library. The protocol is as follows

1. A compliant script must begin by printing "TALARIA V1".
2. The script must then consume lines of the following form

  - FILTER:\<filter\>

       Upon receipt of this the script must send a group of lines corresponding to menu items to STDOUT. This list
   must be terminated by an empty line.

  - SELECT:\<string\>

       This corresponds to a menu selection by the user, the script should terminate after receiving this and 
       performing the appropriate action

All filtering, sorting and history is left to the script. Since users expect instantaneous behavior filtering should
not take longer than a few miliseconds to reduce perceived latency.

## Example

A simple dictionary mike look like the following.

```
#!/usr/bin/python3

import sys
import os

words = open('/etc/dictionaries-common/words', 'r').read().split('\n')

print("TALARIA V1")
while True:
    typ,filt = sys.stdin.readline().strip().split(':', maxsplit=1)

    sys.stdout.write('\n'.join(w for w in words if filt in w and w))
    sys.stdout.write('\n\n')

    sys.stdout.flush()
```

The above can run by passing the script to talaria like so `talaria <dict.py>`

Alternatively talaria can be specified as the interpeter itself so the script can be launched directly. To do this
the real interpreter must be specified after talaria as below.

```
#!/usr/bin/talaria /usr/bin/python3
<script body>
```
