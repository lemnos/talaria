/*
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------
 * 
 * Original Authors: Aetnaeus (https://github.com/lemnos)
 *
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Atom create_atom(Display *dpy, char *name) {
    Atom at = XInternAtom(dpy, name, False);
    if(at == None) {
        fprintf(stderr, "X11 Error: Atom %s does not exist\n", name);
        exit(1);
    }

    return at;
}

static Atom get_atom(Display *dpy, char *name) {
    Atom at = XInternAtom(dpy, name, True);
    if(at == None) {
        fprintf(stderr, "X11 Error: Atom %s does not exist\n", name);
        exit(1);
    }

    return at;
}

static char* get_utf8_property(Display *dpy, Window win, Atom prop) {
    unsigned char *data = NULL;
    Atom utf8 = get_atom(dpy, "UTF8_STRING");

    int fmt;
    Atom type;
    unsigned long n, remaining;


    XGetWindowProperty(dpy,
    	win,
    	prop,
    	0, 0,
    	False,
    	utf8,
    	&type,
    	&fmt,
    	&n,
    	&remaining,
    	&data);

    int len = remaining/4+1;
    data = malloc(len*4);

    if(type != utf8)
        return NULL;

    XGetWindowProperty(dpy,
    	win,
    	prop,
    	0, len,
    	False,
    	utf8,
    	&type,
    	&fmt,
    	&n,
    	&remaining,
    	&data);

    return (char*)data;
}

char *x11_get_primary_selection(Display *dpy, Window win) {
    static Atom utf8 = None;
    static Atom primary = None;
    static Atom result_prop = None;

    if(utf8 == None) {
        utf8 = get_atom(dpy, "UTF8_STRING");
        primary = get_atom(dpy, "PRIMARY");
        result_prop = create_atom(dpy, "PRIMARY_SELECTION");
    }

    XConvertSelection(dpy, primary, utf8, result_prop, win, CurrentTime);
    char *res;
    for(res = NULL; !res; res = get_utf8_property(dpy, win, result_prop));
    return res;
}
