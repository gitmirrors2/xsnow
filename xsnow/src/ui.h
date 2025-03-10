/* 
 -copyright-
# xsnow: let it snow on your desktop
# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#              2019,2020,2021,2022,2023,2024 Willem Vermin
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#-endcopyright-
*/
#pragma once

// required GTK version for running the ui. (from ui.xml, made using glade)
#define GTK_MAJOR 3
#define GTK_MINOR 20
#define GTK_MICRO 0

extern void  ui (void);
extern void  ui_error_x11(void);
extern void  ui_show_nflakes(int n);
extern void  ui_set_birds_header(const char *text);
extern void  ui_set_celestials_header(const char *text);
extern void  ui_show_range_etc(void);
extern void  ui_show_desktop_type(const char *s);
extern void  ui_set_sticky(int x);
extern void  ui_background(int m);
extern void  ui_ui(void);

extern GtkBuilder    *builder;

extern void  ui_gray_ww(const int m);
extern void  ui_gray_erase(const int m);
extern void  ui_gray_below(const int m);
extern void  ui_gray_birds(int m);
extern int   ui_checkgtk(void);
extern char *ui_gtk_version(void);
extern char *ui_gtk_required(void);
extern int   ui_run_nomenu(void);

extern void set_buttons(void);
extern void handle_language(int restart);
