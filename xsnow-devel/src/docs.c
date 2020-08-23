/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019,2020 Willem Vermin
#-# 
#-# This program is free software: you can redistribute it and/or modify
#-# it under the terms of the GNU General Public License as published by
#-# the Free Software Foundation, either version 3 of the License, or
#-# (at your option) any later version.
#-# 
#-# This program is distributed in the hope that it will be useful,
#-# but WITHOUT ANY WARRANTY; without even the implied warranty of
#-# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#-# GNU General Public License for more details.
#-# 
#-# You should have received a copy of the GNU General Public License
#-# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#-# 
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xsnow.h"
#include "docs.h"
#include "version.h"

// return char* with all occurences of needle in s replaced with rep
// s, needle, rep will not be modified
static char *replace_all(const char *s, const char *needle, const char *rep);
static void manout(const char*flag, const char*txt);
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

void docs_usage(int man)
{

   if (man)
   {
      doman = 1;
      printf(".\\\" DO NOT MODIFY THIS FILE! It was created by xsnow -manpage .\n");
      printf(".TH XSNOW \"6\" \"2020\" \"xsnow\\-" VERSION "\" \"User Commands\"\n");
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
      printf("XSNOW 2019 xsnow-" VERSION " User Commands\n");
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

   manout(" ","Below: <n> denotes an unsigned decimal (e.g 123)");
   manout(" ","or octal (e.g. 017) or hex (e.g. 0x50009) number.");
   manout(" ","<c> denotes a color name like \"red\" or \"#123456\".");
   manout(" "," ");
   if (!doman)
      printf("\n");
   manout("-h, -help"                 ,"print this text.");
   manout("-H, -manpage"              ,"print man page.");
   manout("-v, -version"              ,"prints version of xsnow.");
   manout("-display name"             ,"Drop the snowflakes on the given display.");
   manout(" "                         ,"Make sure the display is nearby, so you can hear them enjoy...");
   manout("-vintage"                  ,"Run xsnow in vintage settings.");
   manout("-defaults"                 ,"Do not read config file (see FILES).");
   manout("-noconfig"                 ,"Do not read or write config file (see FILES).");
   manout("-nomenu"                   ,"Do not show interactive menu.");
   manout("-id <n>"                   ,"Snow in window with id (for example from xwininfo).");
   manout("-desktop"                  ,"Act as if window is a desktop.");
   manout("-allworkspaces <n>"        ,"0: use one desktop for snow, 1: use all desktops (default: " EQ(DEFAULT_AllWorkspaces) ").");
   manout("-fullscreen"               ,"Snow on full screen window: panels, task bars etc. will be not accessible.");
   manout("-above"                    ,"Snow above your windows. Default is to snow below your windows.");
   manout("-xwininfo  "               ,"Use a cursor to point at the window you want the snow to be fallen in.");
   manout("-bg <c>"                   ,"Use color <c> to erase obsolete drawings (snow, santa, ...).");
   manout(" "                         ,"Useful in for example KDE: create mono colored background, and specify");
   manout(" "                         ,"the same color here, e.g: -bg \"#123456\" (default: " EQ(DEFAULT_BGColor) ".)");
   manout("-exposures"                ,"Use XClearArea(...,exposures=True) when erasing.");
   manout("-noexposures"              ,"Use XClearArea(...,exposures=False) when erasing.");
   manout(" "                         ,"Exposures have effect with '-alpha 0' or '-xwininfo'.");
   manout("-fvwm"                     ,"prepare for FVWM and the like: no background, -alpha 0, -noexposures .");
   manout("-gnome"                    ,"prepare for GNOME, KDE and the like: no background, -alpha 1, -noexposures .");
   manout("-stopafter <n>"            ,"Stop xsnow after so many seconds.");
   manout("-noquiet"                  ,"Print during running info about disappeared windows, blown fuses etc.");
   manout("-wantwindow"               ,"Specify your favorite window:");
   manout("            default"       ,"If possible, use GTK-Cairo window for Santa snow and scenery.");
   manout("            transparent"   ,"If possible, use transparent X11-window for Santa, snow and scenery.");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Snow options:\n");
   }
   else
   {
      printf("\n  Snow options:\n\n");
   }
   manout("-snowflakes <n>"           ,"The higher, the more snowflakes are generated per second. Default: " EQ(DEFAULT_SnowFlakesFactor) ".");
   manout("-noblowsnow"               ,"Do not animate blowing snow from trees or windows.");
   manout("-sc <c>  "                 ,"Use the given string as color for the flakes (default: " EQ(DEFAULT_SnowColor) ").");
   manout("-snowspeedfactor <n>"      ,"Multiply the speed of snow with this number/100 (default:" EQ(DEFAULT_SnowSpeedFactor) ").");
   manout("-nosnowflakes"             ,"Do not show falling snowflakes. (Weird!)");
   manout("-flakecountmax <n>"        ,"Maximum number of active flakes (default " EQ(DEFAULT_FlakeCountMax) ").");
   manout("-blowofffactor <n>"        ,"The higher, the more snow is generated in blow-off scenarios (default: " EQ(DEFAULT_BlowOffFactor) ").");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Tree options:\n");
   }
   else
   {
      printf("\n  Tree options:\n\n");
   }
   manout("-treetype <n>[,<n> ...]"   ,"Choose tree types: minimum 0, maximum " EQ(MAXTREETYPE) " (default " EQ(DEFAULT_TreeType) ").");
   manout(" "                         ,"Thanks to Carla Vermin for numbers >=3!"); 
   manout(" "                         ,"Credits: Image by b0red on Pixabay.");
   manout("-treetype all"             ,"Use all available tree types.");
   manout("-tc <c>"                   ,"Use the given string as the color for the default trees (default: " EQ(DEFAULT_TreeColor) ").");
   manout(" "                         ,"Works only for treetype 0.");
   manout("-notrees"                  ,"Do not display the trees.");
   manout("-trees <n>"                ,"Desired number of trees. Default " EQ(DEFAULT_DesiredNumberOfTrees) ".");
   manout("-treefill <n>"             ,"Region in percents of the height of the window where trees grow (default: " EQ(DEFAULT_TreeFill) ").");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Santa options:\n");
   }
   else
   {
      printf("\n  Santa options:\n\n");
   }
   manout("-nosanta"                  ,"Do not display Santa running all over the screen.");
   manout("-norudolph"                ,"No Rudolph.");
   manout("-santa <n>"                ,"The minimum size of Santa is 0, the maximum size is " EQ(MAXSANTA) ". Default is " EQ(DEFAULT_SantaSize) ".");
   manout(" "                         ,"Thanks to Thomas Linder for the (big) Santa 2!");
   manout(" "                         ,"Santa 3 is derived from Santa 2, and shows the required eight reindeer.");
   manout(" "                         ,"The appearance of Santa 4 may be a surprise, thanks to Carla Vermin for this one.");
   manout("-santaspeedfactor <n>"     ,"The speed Santa should not be excessive if he doesn't want to get");
   manout(" "                         ,"fined. The appropriate speed for the Santa chosen");
   manout(" "                         ,"will be multiplied by santaspeedfactor/100 (default: " EQ(DEFAULT_SantaSpeedFactor) ").");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Wind options:\n");
   }
   else
   {
      printf("\n  Wind options:\n\n");
   }
   manout("-nowind   "                ,"By default it gets windy now and then. If you prefer quiet weather");
   manout(" "                         ,"specify -nowind.");
   manout("-whirlfactor <n>"          ,"This sets the whirl factor, i.e. the maximum adjustment of the");
   manout(" "                         ,"horizontal speed. The default value is " EQ(DEFAULT_WhirlFactor) ".");
   manout("-windtimer <n>"            ,"With -windtimer you can specify how often it gets  windy. It's");
   manout(" "                         ,"sort of a period in seconds, default value is " EQ(DEFAULT_WindTimer) ".");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Fallen snow options:\n");
   }
   else
   {
      printf("\n  Fallen snow options:\n\n");
   }
   manout("-wsnowdepth <n>"           ,"Maximum thickness of snow on top of windows (default: " EQ(DEFAULT_MaxWinSnowDepth) ").");
   manout("-ssnowdepth <n>"           ,"Maximum thickness of snow at the bottom of the screen (default: " EQ(DEFAULT_MaxScrSnowDepth) ").");
   manout("-maxontrees <n>"           ,"Maximum number of flakes on trees. Default " EQ(DEFAULT_MaxOnTrees) ".");
   manout("-nokeepsnowonwindows"      ,"Do not keep snow on top of the windows.");
   manout("-nokeepsnowonscreen"       ,"Do not keep snow at the bottom of the screen.");
   manout("-nokeepsnowontrees"        ,"Do not keep snow on trees.");
   manout("-nokeepsnow"               ,"Do not have snow sticking anywhere.");
   manout("-nofluffy"                 ,"Do not create fluff on fallen snow.");
   manout("-offsetx <n>"              ,"Correction for window-manager provided of x-coordinate of window. Default " EQ(DEFAULT_OffsetX) ".");
   manout("-offsety <n>"              ,"Correction for window-managr provided of y-coordinate of window. Default " EQ(DEFAULT_OffsetY) ".");
   manout("-offsetw <n>"              ,"Correction for window-manager provided of width of window. Default " EQ(DEFAULT_OffsetW) ".");
   manout("-offsets <n>"              ,"Correction for bottom coordinate of your screen. A negative value lifts");
   manout(" "                         ,"the xsnow screen up. Default " EQ(DEFAULT_OffsetS) ".");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Birds options:\n");
   }
   else
   {
      printf("\n  Birds options:\n\n");
   }
   manout("-anarchy <n>"           ,"Anarchy factor ( 0..100 default: " EQ(DEFAULT_Anarchy) ").");
   manout("-birdscolor <c>  "      ,"Use the given string as color for the birds (default: " EQ(DEFAULT_BirdsColor) ").");
   manout("-birdsonly <n>"         ,"Show only birds ( 0/1 default: " EQ(DEFAULT_BirdsOnly) ").");
   manout("-birdsspeed <n>"        ,"Speed of birds ( 0..300 default: " EQ(DEFAULT_BirdsSpeed) ").");
   manout("-disweight <n>"         ,"Eagerness to keep desired distance ( 0..100 default: " EQ(DEFAULT_DisWeight) ").");
   manout("-focuscentre <n>"       ,"Eagerness to fly to the focus ( 0..300 default: " EQ(DEFAULT_AttrFactor) ").");
   manout("-followneighbours <n>"  ,"Eagerness to follow neighbours ( 0..100 default: " EQ(DEFAULT_FollowWeight) ").");
   manout("-nbirds <n>"            ,"Number of birds ( 0..400 default: " EQ(DEFAULT_Nbirds) ").");
   manout("-neighbours <n>"        ,"Number of neighbours to watch ( 0..20 default: " EQ(DEFAULT_Neighbours) ").");
   manout("-prefdistance <n>"      ,"Preferred distance to neighbours ( 0..100 default: " EQ(DEFAULT_PrefDistance) ").");
   manout("-showbirds <n>"         ,"Show birds ( 0/1 default: " EQ(DEFAULT_ShowBirds) ").");
   manout("-showattr <n>"          ,"Show attraction point ( 0/1 default: " EQ(DEFAULT_ShowAttrPoint) ").");
   manout("-viewingdistance <n>"   ,"Viewing distance ( 0..95 default: " EQ(DEFAULT_ViewingDistance) ").");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"Other options:\n");
   }
   else
   {
      printf("\n  Other options:\n\n");
   }
   manout("-stars <n>"                ,"The number of stars (default: " EQ(DEFAULT_NStars) ").");
   manout("-nometeorites"             ,"Do not show meteorites.");
   manout("-cpuload <n>"              ,"How busy is your system with xsnow:");
   manout(" "                         ,"the higher, the more load on the system (default: " EQ(DEFAULT_CpuLoad) ").");

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
   manout (" "," ");
   manout("$HOME/xsnow/pixmaps/tree.xpm", "If present, xsnow will try this file for displaying");
   manout(" ", "the trees. The format must be xpm (X PixMap) format, see");
   manout(" ", "https://en.wikipedia.org/wiki/X_PixMap .");
   manout("$HOME/xsnow/pixmaps/santa<n>.xpm", "where <n> = 1,2,3,4.");
   manout(" ", "If present, xsnow will try this files (4 of them) for displaying");
   manout(" ", "Santa. The format must be xpm (X PixMap) format, see");
   manout(" ", "https://en.wikipedia.org/wiki/X_PixMap .");

   if(doman)
   {
      printf(".PP\n"); printf(".SS \"EXAMPLES\n");
      printf(".br\n");
   }
   else
   {
      printf("\n   EXAMPLES\n\n");
   }
   manout(".","    $ xsnow -defaults        # run with defaults.");
   manout(".","    $ xsnow                  # run using values from the config file.");
   manout(".","    $ xsnow -treetype 1,2    # use tree types 1 and 2.");
   manout(".","    $ xsnow -kde -kdebg -bg blue4  # for the KDE environment.");

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
   manout(".","- Xsnow stresses the Xserver too much.");
   manout(".","- The -xwininfo and -id flags do not function properly.");
   manout(".","- Xsnow does run in Wayland, but will not snow on all windows.");
   manout(".","- Remnants of fluffy snow can persist after removing the");
   manout(" ","  fallen snow. These will gradually disappear, so no big deal.");
   manout(".","- Xsnow tries to adapt its snowing window if the display");
   manout(" ","  settings are changed while xsnow is running.");
   manout(" ","  This does not function always well.");

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
   const char *haystack = s;          // startpoint to search for needle
   while(1)
   {
      const char *q = strstr(haystack,needle);
      if (q == 0)  // no needle in haystack
      {            // cat haystack to result
	 result = (char *)realloc(result,strlen(result)+strlen(haystack)+1);
	 result = strcat(result,haystack);
	 break;
      }
      else      // needle is in haystack
      {         // cat first part of haystack + rep to result
	 result   = (char *)realloc(result,strlen(result)+strlen(haystack)+strlen(rep)+1);
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
void manout(const char*flag, const char*txt)
{

   if (doman)
   {
      char *mantxt  = replace_all(txt, "-","\\-");
      char *manflag = replace_all(flag,"-","\\-");
      if (!strcmp(manflag," "))
	 printf("%s\n",mantxt);
      else if(!strcmp(manflag,"."))
	 printf(".br\n%s\n",mantxt);
      else
      {
	 printf(".TP\n"); printf("\\fB%s\\fR\n",manflag);
	 printf("%s\n",mantxt);
      }
      free(mantxt);
      free(manflag);
   }
   else
   {
      if (!strcmp(flag," "))
	 printf("\t\t  %s\n",txt);
      else if(!strcmp(flag,"."))
	 printf("%s\n",txt);
      else
	 printf("%s\t: %s\n",flag,txt);
   }
}

