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
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "xsnow-constants.h"
#include "docs.h"
#include "version.h"
#include "flags.h"
#include "safe_malloc.h"

// return char* with all occurences of needle in s replaced with rep
// s, needle, rep will not be modified
static char *replace_all(const char *s, const char *needle, const char *rep);
static void manout(const char*flag, const char*txt, ...);
static int doman;
static void printdescription(void);

void printdescription()
{
   printf("Xsnow shows an animation of Santa and snow on your desktop.\n");
   printf("Xsnow can also run in one or more windows, see options -xwininfo, -id .\n");
   printf("(These options only work satisfactorily in an X11 environment.)\n");
   printf("Xsnow depends on an X11 environment. This is forced by setting the\n");
   printf("environment variable GDK_BACKEND=x11 before initializing the GTK.\n");
   printf("Hopefully, this will ensure that xsnow also runs in a Wayland environment\n");
   printf("for some time.\n");
   if(doman)
      printf(".PP\n");
   printf("If xsnow is misbehaving, try to remove the file $HOME/.xsnowrc.\n");
}

#define F(x) DefaultFlags.x

void docs_usage(int man)
{

   if (man)
   {
      doman = 1;
      printf(".\\\" DO NOT MODIFY THIS FILE! It was created by xsnow -manpage .\n");
      printf(".TH XSNOW \"6\" \"2024\" \"xsnow\\-" VERSION "\" \"User Commands\"\n");
      printf(".SH NAME\n");
      printf(".\\\" Turn of hyphenation:\n");
      printf(".hy 0\n");
      printf("xsnow \\- Snow and Santa on your desktop\n");
      printf(".SH SYNOPSIS\n");
      printf(".B xsnow\n");
      printf("[\\fIOPTION\\fR]...\n");
      printf(".PP\n");
      printf(".SH DESCRIPTION\n");
      printdescription();
      printf(".PP\n"); 
      printf(".SS \"General options:\n");
   }
   else
   {
      doman = 0;
      printf("XSNOW 2024 xsnow-" VERSION " User Commands\n");
      printf("NAME\n");
      printf("xsnow - Snow and Santa on your desktop\n");
      printf("SYNOPSIS\n");
      printf("xsnow ");
      printf("[OPTION...\n");
      printf("\n");
      printdescription();
      printf("\n"); 
      printf("General options:\n");
   }

   manout(" ","Below:");
   manout(".","<n> denotes an unsigned decimal (e.g 123)");
   manout(" ","or octal (e.g. 017) or hex (e.g. 0x50009) number.");
   manout(".","<c> denotes a string like \"red\" or \"#123456\".");
   manout(".","<f> denotes a file name, like \"/home/rick/Pictures/background.jpg\".");
   manout(" "," ");
   if (!doman)
      printf("\n");
   manout("-h, -help"               ,"print this text.");
   manout("-H, -manpage"            ,"print man page.");
   manout("-v, -version"            ,"prints version of xsnow.");
   manout("-changelog"              ,"prints ChangeLog.");
#ifdef SELFREP
   manout("-selfrep"                ,"put tar ball on stdout, so you can do:");
   manout("."                       ,"xsnow -selfrep > xsnow.tar.gz");
#endif
   manout("-display <c>"            ,"Drop the snowflakes on the given display.");
   manout(" "                       ,"Make sure the display is nearby, so you can hear them enjoy...");
   manout("-screen <n>"             ,"If you have multiple monitors: snow in monitor n.");
   manout("."                       ,"-1: use all monitors (default: %d)",F(Screen));
   manout("."                       ,"Note: for this to work, Xinerama has to be functional.");
   manout("-outline <n>"            ,"1: draw outline around snow window. 0: no outline.");
   manout("."                       ,"Default: %d.",F(Outline));
   manout("-vintage"                ,"Run xsnow in vintage settings.");
   manout("-defaults"               ,"Do not read config file (see FILES).");
   manout("-noconfig"               ,"Do not read or write config file (see FILES).");
   manout("-hidemenu"               ,"Start with hidden interactive menu.");
   manout("-nomenu"                 ,"Do not show interactive menu.");
   manout("-lang <c>"               ,"Set language, example: -lang it, see LANGUAGES below. Default: %s.",F(Language));
   manout("-scale <n>"              ,"Apply scalefactor (default: %d).",F(Scale));
   manout("-doublebuffer <n>"       ,"1: use double buffering; 0: do not use double buffering (default: %d).",F(UseDouble));
   manout(" "                       ,"Only effective with '-root' or '-id' or '-xwininfo'.");
   manout("-transparency <n>"       ,"Transparency in % (default: %d)",F(Transparency));
   manout("-theme <n>"              ,"1: use xsnow theme for menu; 0: use system theme (default: %d)",F(ThemeXsnow));
   manout("-checkgtk <n>"           ,"0: Do not check gtk version before starting the user interface.");
   manout(" "                       ,"1: Check gtk version before starting the user interface.");
   manout(" "                       ,"(default: %d).",F(CheckGtk));
   manout("-id <n>, -window-id <n>" ,"Snow in window with id (for example from xwininfo).");
   manout("--window-id <n>"         ,"see -id.");
   manout("-desktop"                ,"Act as if window is a desktop.");
   manout("-allworkspaces <n>"      ,"0: use one desktop for snow, 1: use all desktops (default: %d).",F(AllWorkspaces));
   manout("-above"                  ,"Snow above your windows. Default is to snow below your windows.");
   manout(" "                       ,"NOTE: in some environments this results in an un-clickable desktop.");
   manout("-xwininfo  "             ,"Use a cursor to point at the window you want the snow to be fallen in.");
   manout("-stopafter <n>"          ,"Stop xsnow after so many seconds.");
   manout("-root, --root "          ,"Force to paint on (virtual) root window.");
   manout("."                       ,"Use this for xscreensaver:");
   manout("."                       ,"Make sure xscreensaver is running, either as a start-up application");
   manout("."                       ,"or from the command line, e.g:");
   manout("."                       ,"   nohup xscreensaver &");
   manout("."                       ,"or");
   manout("."                       ,"    nohup xscreensaver -no-capture-stderr &");
   manout("."                       ,"Run the program xscreensaver-demo to create the file ~/.xscreensaver");
   manout("."                       ,"In the file ~.xscreensaver add after the line 'programs:' this line:");
   manout("."                       ,"    xsnow -root");
   manout("."                       ,"Use the program xscreensaver-demo to select xsnow as screensaver.");
   manout("."                       ,"You probably want to select: Mode: Only One Screen Saver.");
   manout("-bg <f>     "            ,"file to be used as background when running under xscreensaver.");
   manout("-noisy     "             ,"Write extra info about some mouse clicks, X errors etc, to stdout.");
   manout("-cpuload <n>"            ,"How busy is your system with xsnow:");
   manout(" "                       ,"the higher, the more load on the system (default: %d).",F(CpuLoad));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Snow options:\n");
   }
   else
   {
      printf("\n  Snow options:\n\n");
   }
   manout("-snowflakes <n>"       ,"The higher, the more snowflakes are generated per second. Default: %d.",F(SnowFlakesFactor));
   manout("-blowsnow"             ,"(Default) Animate blow-off snow.");
   manout("-noblowsnow"           ,"Do not animate blowing snow from trees or windows");
   manout("-sc <c>  "             ,"Use the given string as color for the flakes (default: %s).",F(SnowColor));
   manout("-sc2 <c> "             ,"Use the given string as second color for the flakes (default: %s).",F(SnowColor2));
   manout("-enablesc2 <n> "       ,"1: enable usage of second color for the flakes (default: %d).",F(UseColor2));
   manout("-snowspeedfactor <n>"  ,"Multiply the speed of snow with this number/100 (default: %d).",F(SnowSpeedFactor));
   manout("-snowsize <n>"         ,"Set size of (non-vintage) snow flakes (default: %d).",F(SnowSize));
   manout("-snow       "          ,"(Default) Show snow.");
   manout("-nosnow -nosnowflakes" ,"Do not show snow.");
   manout("-flakecountmax <n>"    ,"Maximum number of active flakes (default: %d).",F(FlakeCountMax));
   manout("-blowofffactor <n>"    ,"The higher, the more snow is generated in blow-off scenarios (default: %d).",F(BlowOffFactor));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Tree options:\n");
   }
   else
   {
      printf("\n  Scenery options:\n\n");
   }
   manout("-treetype <n>[,<n> ...]" ,"Choose tree types: minimum 0, maximum %d (default: %s).", MAXTREETYPE,F(TreeType));
   manout(" "                       ,"Thanks to Carla Vermin for numbers >=3!"); 
   manout(" "                       ,"Credits: Image by b0red on Pixabay.");
   manout("-treetype all"           ,"(Default) Use all non-vintage available tree types.");
   manout("-tc <c>"                 ,"Use the given string as the color for the vintage tree (default: %s).",F(TreeColor));
   manout(" "                       ,"Works only for treetype 0.");
   manout("-notrees"                ,"Do not display the trees.");
   manout("-showtrees"              ,"(Default) Display the trees.");
   manout("-trees <n>"              ,"Desired number of trees. Default %d.",F(DesiredNumberOfTrees));
   manout("-treefill <n>"           ,"Region in percents of the height of the window where trees grow (default: %d).",F(TreeFill));
   manout("-treescale <n>"          ,"Scale scenery (default: %d).",F(TreeScale));
   manout("-treeoverlap"            ,"Allow scenery items to overlap each other (default).");
   manout("-notreeoverlap"          ,"Do not allow scenery items to overlap each other.");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Santa options:\n");
   }
   else
   {
      printf("\n  Santa options:\n\n");
   }
   manout("-showsanta"            ,"(Default) Display Santa running all over the screen.");
   manout("-nosanta"              ,"Do not display Santa running all over the screen.");
   manout("-showrudolph"          ,"(Default) With Rudolph.");
   manout("-norudolph"            ,"No Rudolph.");
   manout("-santa <n>"            ,"The minimum size of Santa is 0, the maximum size is %d. Default is %d.",MAXSANTA,F(SantaSize));
   manout(" "                     ,"Thanks to Thomas Linder for the (big) Santa 2!");
   manout(" "                     ,"Santa 3 is derived from Santa 2, and shows the required eight reindeer.");
   manout(" "                     ,"The appearance of Santa 4 may be a surprise, thanks to Carla Vermin for this one.");
   manout("-santaspeedfactor <n>" ,"The speed Santa should not be excessive if he doesn't want to get");
   manout(" "                     ,"fined. The appropriate speed for the Santa chosen");
   manout(" "                     ,"will be multiplied by santaspeedfactor/100 (default: %d).",F(SantaSpeedFactor));
   manout("-santascale <n>"       ,"The scale to be used when drawing Santa (default: %d).",F(SantaScale));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Celestial options:\n");
   }
   else
   {
      printf("\n  Celestial options:\n\n");
   }
   manout("-wind     "            ,"(Default) It will get windy now and then.");
   manout("-nowind   "            ,"By default it gets windy now and then. If you prefer quiet weather");
   manout(" "                     ,"specify -nowind.");
   manout("-whirlfactor <n>"      ,"This sets the whirl factor, i.e. the maximum adjustment of the");
   manout(" "                     ,"horizontal speed. The default value is %d.",F(WhirlFactor));
   manout("-windtimer <n>"        ,"With -windtimer you can specify how often it gets  windy. It's");
   manout(" "                     ,"sort of a period in seconds, default value is %d.",F(WindTimer));
   manout("-stars <n>"            ,"The number of stars (default: %d).",F(NStars));
   manout("-meteors"              ,"(Default) Show meteors.");
   manout("-nometeors"            ,"Do not show meteors.");
   manout("-meteorfrequency"      ,"Frequency of falling of meteors, 0..100 (default: %d).",F(MeteorFrequency));
   manout("-moon <n>"             ,"1: show moon, 0: do not show moon (default: %d).",F(Moon));
   manout("."                     ,"Picture of moon thanks to  Pedro Lasta on Unsplash.");
   manout("."                     ,"https://unsplash.com/photos/wCujVcf0JDw");
   manout("-moonspeed <n>"        ,"Speed of moon in pixels/minute (default: %d).",F(MoonSpeed));
   manout("-moonsize <n>"         ,"Relative size of moon (default: %d).",F(MoonSize));
   manout("-mooncolor <n>"        ,"Color of moon 0: yellow-ish; 1: white-ish (default: %d).",F(MoonColor));
   manout("-halo <n>"             ,"1: show halo around moon, 0: do not show halo (default: %d).",F(Halo));
   manout("-halobrightness <n>"   ,"Brightness of halo (default: %d).",F(HaloBright));
   manout("-aurora <n>"           ,"To show (1) or not to show(0) aurora (default: %d).",F(Aurora));
   manout("."                     ,"  On most desktops aurora works, but not on all. Try!");
   manout("-auroraleft"           ,"Place aurora in top left of screen.");
   manout("-auroramiddle"         ,"Place aurora in top middle of screen.");
   manout("-auroraright"          ,"Place aurora in top right of screen (default).");
   manout("-aurorawidth <n>"      ,"Width of aurora in percentage of screen width (default: %d).",F(AuroraWidth));
   manout("-aurorabase <n>"       ,"Height of aurora's base line in percentage of screen height (default: %d).",F(AuroraBase));
   manout("-auroraheight <n>"     ,"Height of aurora (default: %d).",F(AuroraHeight));
   manout("-auroraspeed <n>"      ,"Animation speed of aurora (default: %d).",F(AuroraSpeed));
   manout("."                     ,"   10: about real value, 100: timelapse.");
   manout("-aurorabrightness <n>" ,"Brightness of aurora (default: %d).",F(AuroraBrightness));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Fallen snow options:\n");
   }
   else
   {
      printf("\n  Fallen snow options:\n\n");
   }
   manout("-wsnowdepth <n>"      ,"Maximum thickness of snow on top of windows (default: %d).",F(MaxWinSnowDepth));
   manout("-ssnowdepth <n>"      ,"Maximum thickness of snow at the bottom of the screen (default: %d).",F(MaxScrSnowDepth));
   manout("-maxontrees <n>"      ,"Maximum number of flakes on trees. Default %d.",F(MaxOnTrees));
   manout("-keepsnowonwindows"   ,"(Default) Keep snow on top of the windows.");
   manout("-nokeepsnowonwindows" ,"Do not keep snow on top of the windows.");
   manout("-keepsnowonscreen"    ,"(Default) Keep snow at the bottom of the screen.");
   manout("-nokeepsnowonscreen"  ,"Do not keep snow at the bottom of the screen.");
   manout("-keepsnowontrees"     ,"(Default) Keep snow on trees.");
   manout("-nokeepsnowontrees"   ,"Do not keep snow on trees.");
   manout("-keepsnow"            ,"(Default) Have snow sticking anywhere.");
   manout("-nokeepsnow"          ,"Do not have snow sticking anywhere.");
   manout("-fluffy"              ,"(Default) Create fluff on fallen snow.");
   manout("-nofluffy"            ,"Do not create fluff on fallen snow.");
   manout("-offsetx <n>"         ,"Correction for window-manager provided x-coordinate of window. Default %d.",F(OffsetX));
   manout("-offsety <n>"         ,"Correction for window-manager provided  y-coordinate of window. Default %d.",F(OffsetY));
   manout("-offsetw <n>"         ,"Correction for window-manager provided width of window. Default %d.",F(OffsetW));
   manout("-offsets <n>"         ,"Correction for bottom coordinate of your screen. A negative value lifts");
   manout(" "                    ,"the xsnow screen up. Default %d.",F(OffsetS));
   manout("-ignoretop <n>"       ,"Do not collect snow on window > 0.8*width of screen and closer than");
   manout(" "                    ,"<n> pixels from the top. Sometimes an hidden window is sitting there,");
   manout(" "                    ,"but treated as a normal window by xsnow. Default %d.",F(IgnoreTop));
   manout("-ignorebottom <n>"    ,"Analog to -ignoretop, but now for the bottom. Default %d.",F(IgnoreBottom));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Birds options:\n");
   }
   else
   {
      printf("\n  Birds options:\n\n");
   }
   manout("-anarchy <n>"          ,"Anarchy factor ( 0..100 default: %d).",F(Anarchy));
   manout("-birdscolor <c>  "     ,"Use the given string as color for the birds (default: %s).",F(BirdsColor));
   manout("-birdsonly <n>"        ,"Show only birds ( 0/1 default: %d).",F(BirdsOnly));
   manout("-birdsspeed <n>"       ,"Speed of birds ( 0..300 default: %d).",F(BirdsSpeed));
   manout("-disweight <n>"        ,"Eagerness to keep desired distance ( 0..100 default: %d).",F(DisWeight));
   manout("-focuscentre <n>"      ,"Eagerness to fly to the focus ( 0..300 default: %d).",F(AttrFactor));
   manout("-followneighbours <n>" ,"Eagerness to follow neighbours ( 0..100 default: %d).",F(FollowWeight));
   manout("-nbirds <n>"           ,"Number of birds ( 0..400 default: %d).",F(Nbirds));
   manout("-neighbours <n>"       ,"Number of neighbours to watch ( 0..20 default: %d).",F(Neighbours));
   manout("-prefdistance <n>"     ,"Preferred distance to neighbours ( 0..100 default: %d).",F(PrefDistance));
   manout("-showbirds <n>"        ,"Show birds ( 0/1 default: %d).",F(ShowBirds));
   manout("-showattr <n>"         ,"Show attraction point ( 0/1 default: %d).",F(ShowAttrPoint));
   manout("-attrspace <n>"        ,"Vertical space to be used by the attraction point (default: %d).",F(AttrSpace));
   manout("-followsanta <n>"      ,"Birds like Santa ( 0/1 default: %d).",F(FollowSanta));
   manout("-viewingdistance <n>"  ,"Viewing distance ( 0..95 default: %d).",F(ViewingDistance));
   manout("-birdsscale <n>"       ,"Scalefactor used painting the birds (default: %d).",F(BirdsScale));

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"LANGUAGES\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   LANGUAGES\n\n");
   }
   manout(" ","Xsnow comes with some translations to non-english languages.");
   manout(".","The translations are done with the aid of ");
   manout(" ","translate.google.com (implemented in package 'trans'),");
   manout(" ","so there will be room for improvement. Any suggestions are welcome: contact@ratrabbit.nl ."); 

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"FILES\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   FILES\n\n");
   }
   manout("$HOME/.xsnowrc", "Settings are read from and written to this file.");
   manout(" ","See flags -noconfig and -defaults how to influence this behaviour.");
   manout(".","    NOTE: the following settings are not read or written:");
   manout(".","          -above  -defaults  -desktop  -fullscreen -noconfig -id");
   manout(".","          -nomenu -stopafter -xwininfo -display    -noisy    -checkgtk");
   manout(".","    NOTE: the file also contains the screen locations of a number of buttons,");
   manout(".","          which can be used for testing purposes.");
   manout(" "," ");
   manout("$HOME/.xsnowstop", "Xsnow checks regularly the existence of this file.");
   manout(" ","If present, xsnow will remove the file and stop.");
   manout(" "," ");
   manout("$HOME/xsnow/pixmaps/tree.xpm", "If present, xsnow will try this file for displaying");
   manout(" ", "the trees. The format must be xpm (X PixMap) format, see");
   manout(" ", "https://en.wikipedia.org/wiki/X_PixMap .");
   manout(" "," ");
   manout("$HOME/xsnow/pixmaps/santa<n>.xpm", "where <n> = 1,2,3,4.");
   manout(" ", "If present, xsnow will try this files (4 of them) for displaying");
   manout(" ", "Santa. The format must be xpm (X PixMap) format, see");
   manout(" ", "https://en.wikipedia.org/wiki/X_PixMap .");
   manout(".", "    NOTE: To show: activate the first Santa in the menu.");
   manout(" "," ");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"SIGNALS\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   SIGNALS\n\n");
   }
   manout(" ","On receiving the SIGUSR1 signal, xsnow writes the file ~/.xsnowrc.");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"EXAMPLES\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   EXAMPLES\n\n");
   }
   manout (".","    $ xsnow -defaults        # run with defaults.");
   manout (".","    $ xsnow                  # run using values from the config file.");
   manout (".","    $ xsnow -treetype 1,2    # use tree types 1 and 2.");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"WINDOW MANAGER ISSUES\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   WINDOW MANAGER ISSUES\n\n");
   }
   manout(" ","In general, xsnow works better when using a compositing window manager");
   manout(" ","like xcompmgr, compton or picom.");
   manout(" ","However, with some window managers (FVWM for example), the xsnow-window");
   manout(" ","is transparent, but not click-through.");
   manout(" ","Flags to be tried in this case include: -root, -doublebuffer, -xwininfo, -id.");
   manout(".","Here follow some window managers with their issues:");
   manout("."," ");
   manout("Tiling window managers","Here you need to float windows with class=Xsnow.");
   manout("AWESOME","Without compositor: no issues.");
   manout(".","With compositor: no click-through xsnow window,");
   manout(" ","and issues with multi-monitor setup.");
   manout("BSPWM","No issues if you add to your bspwmrc (the bspwm configuration file):");
   manout(".","    bspc rule -a Xsnow state=floating border=off");
   manout("CINNAMON","No issues.");
   manout("DWM","No issues, except the \"Below Windows\" setting in the \"settings\" panel.");
   manout("ENLIGHTENMENT","With one monitor: no issuses.");
   manout(".","With more montors: probems with showing in 'all monitors'");
   manout("FLUXBOX","Without compositor: no issues.");
   manout(".","With compositor: no click-through xsnow window");
   manout("FVWM","Without compositor: no issues.");
   manout(".","With compositor: no click-through xsnow window");
   manout("GNOME on Xorg","No issues.");
   manout("GNOME on Wayland","Most windows don't catch snow.");
   manout("HERBSTLUFTWM","No issues.");
   manout("I3","Without compositor: windows don't catch snow, use the next line in \"config\":");
   manout(".","    for_window [class=\"Xsnow\"] floating enable;border none");
   manout(".","With compositor: unworkable.");
   manout("JVM","No issues.");
   manout("LXDE","With compositor: no issues.");
   manout(".","Without compositor: works with one monitor.");
   manout(".","Maybe you need to run with the flag -xwininfo");
   manout("LXQT","Without compositor: unworkable.");
   manout(" ","With compositor: no issues.");
   manout("MATE","No issues.");
   manout("OPENBOX","No issues.");
   manout("PLASMA (KDE)","No issues.");
   manout("SPECTRWM","Various issues. In any case you need in spectrwm.conf:");
   manout(".","    quirk[Xsnow] = FLOAT");
   manout("TWM","Without compositor: no issues.");
   manout(".","With compositor: no click-through xsnow window and");
   manout(".","you need to tweak settings->lift snow on windows.");
   manout("WINDOW MAKER","Without compositor: no issues.");
   manout(" ","With compositor: no click-through xsnow window");
   manout("XFCE","No issues when compositing is on, unworkable when compositing is off."); 
   manout(".","See settings -> Window Manager Tweaks -> Compositor");
   manout("XMONAD","No issues if you add to your xmonad.hs:");
   manout("."," import XMonad.Hooks.EwmhDesktops");
   manout("."," xmonad $ ewmh $ defaultConfig");
   manout("."," in the ManageHook section:");
   manout(".","    className = ? \"Xsnow\" --> doFloat");


   if(doman)
   {
      printf(".PP\n"); printf(".SS \"BUGS\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   BUGS\n\n");
   }
   manout(".","- Xsnow needs a complete rewrite: the code is a mess.");
   manout(".","- The flags are not consistent, caused by trying to be");
   manout(" ","    compatible with older versions.");
   manout(".","- Xsnow stresses the Xserver too much.");
   manout(".","- Xsnow does run in Wayland, but will not snow on all windows.");
   manout(".","- Xsnow tries to create a click-through window. This is not successful");
   manout(" ","  in for example FVWM/xcompmgr. In that case, xsnow tries to keep");
   manout(" ","  the snow window below all others, resulting in a transient effect");
   manout(" ","  when you click on the desktop. Sadly, no FVWM menu will appear...");
   manout(".","- Remnants of fluffy snow can persist after removing the");
   manout(" ","    fallen snow. These will gradually disappear, so no big deal.");
   manout(".","- Remnants of meteors can persist after passage of Santa.");
   manout(" ","    These will eventually be wiped out by snow or Santa.");
   manout(".","- Xsnow tries to adapt its snowing window if the display");
   manout(" ","    settings are changed while xsnow is running.");
   manout(" ","    This does not function always well.");
   manout(".","- Xsnow does not play well with 'xcompmgr -a'. In some environments");
   manout(" ","    (Raspberry 64 bit) xcompmgr is started with the flag '-a',");
   manout(" ","    resulting in a black snow window. Remedy:");
   manout(" ","    In a terminal window type:");
   manout(" ","      killall xcompmgr");
   manout(" ","      nohup xcompmgr -n &");
   manout(" ","    and try again.");
   manout(".","- In XFCE, compositing must be enabled for xsnow.");
   manout(" ","    Settings -> Window Manager Tweaks -> Compositor -> Enable display compositing");

   manout(".","- In multi-screen environments, it depends on the display settings");
   manout(" ","    if it is snowing on all screens. Experiment!");

   manout(".","");
   manout(".","Please report your comments via:");
   manout(".","   https://ratrabbit.nl/ratrabbit/contact ."); 

   if(doman)
   {
      printf(".PP\n"); printf(".SH HOMEPAGE\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   HOMEPAGE\n");
   }
   manout(" ","https://ratrabbit.nl/ratrabbit/xsnow");

   if(doman)
   {
      printf(".PP\n"); printf(".SH COPYRIGHT\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   COPYRIGHT\n");
   }
   manout(" ","This is free software; see the source for copying conditions.");
   manout(" ","There is NO warranty; not even for MERCHANTABILITY or FITNESS");
   manout(" ","FOR A PARTICULAR PURPOSE.");
}

char *replace_all(const char *s, const char *needle, const char *rep)
{
   if (strlen(needle) == 0)
      return strdup(s);
   char *result         = strdup("");
   assert(result);
   const char *haystack = s;          // startpoint to search for needle
   while(1)
   {
      const char *q = strstr(haystack,needle);
      if (q == NULL)  // no needle in haystack
      {            // cat haystack to result
	 result = (char *)realloc(result,strlen(result)+strlen(haystack)+1);
	 assert(result);
	 result = strcat(result,haystack);
	 break;
      }
      else      // needle is in haystack
      {         // cat first part of haystack + rep to result
	 result   = (char *)realloc(result,strlen(result)+strlen(haystack)+strlen(rep)+1);
	 REALLOC_CHECK(result);
	 result   = strncat(result, haystack, q-haystack);
	 result   = strcat(result, rep);
	 haystack = q+strlen(needle);
      }
   }
   return result;
}

//
// doman == 1: output in man page format
// flag: " ": normal continuation line
//       otherwize : skip to new paragraph and use bold format
// txt: Line to output
//
void manout(const char*flag, const char*txt, ...)
{
   va_list args;
   va_start(args,txt);

   if (doman)
   {
      char *mantxt  = replace_all(txt, "-","\\-");
      char *manflag = replace_all(flag,"-","\\-");
      if (!strcmp(manflag," "))
      {
	 //printf("%s\n",mantxt);
	 vprintf(mantxt,args);
	 printf("\n");
      }
      else if(!strcmp(manflag,"."))
      {
	 //printf(".br\n%s\n",mantxt);
	 printf(".br\n");
	 vprintf(mantxt,args);
	 printf("\n");
      }
      else
      {
	 printf(".TP\n"); printf("\\fB%s\\fR\n",manflag);
	 //printf("%s\n",mantxt);
	 vprintf(mantxt,args);
	 printf("\n");
      }
      free(mantxt);
      free(manflag);
   }
   else
   {
      if (!strcmp(flag," "))
      {
	 //printf("\t\t  %s\n",txt);
	 printf("\t\t  ");
	 vprintf(txt,args);
	 printf("\n");
      }
      else if(!strcmp(flag,"."))
      {
	 //printf("\t\t  %s\n",txt);
	 printf("\t\t  ");
	 vprintf(txt,args);
	 printf("\n");
      }
      else
      {
	 //printf("%s\t: %s\n",flag,txt);
	 printf("%s\t: ",flag);
	 vprintf(txt,args);
	 printf("\n");
      }
   }
   va_end(args);
}

void docs_changelog()
{
#include "changelog.inc"
}
