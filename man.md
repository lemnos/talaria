% TALARIA(1)
% Aetnaeus
% December 2018

# NAME

talaria - a simple line oriented X11 menu

# SYNOPSIS 

talaria [-d delim] [-f field] [-m] [-x xoff] [-y yoff] [-w width] [-h height] 
[-u unit height] [-b border size] [-l history file] [-v] [-p generator program] 

# DESCRIPTION

talaria is simple a program which produces a dynamically filterable menu from lines in
STDIN and prints the chosen option to STDOUT. It is designed to be flexible and
provides options for controlling menu placement, color themes, and delimeters.

# OPTIONS

**-d** *delim* 
: Use *delim* instead of the NULL byte as the delimeter.

**-f** *field*
: Use the *field*th item on each input line (separated by *delim*) as the visible menu item. 

**-m**
: Do not allow arbitrary input (which is possible by default) (mnemonic *m*andatory)

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
