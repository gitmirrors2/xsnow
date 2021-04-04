/* -copyright-
#-# 
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-# 	      2019,2020,2021 Willem Vermin
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
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include "flags.h"
#include "utils.h"
#include "xsnow.h"
#include "docs.h"
#include "doit.h"
#include "birds.h"
#include "windows.h"

#include "debug.h"

FLAGS Flags;
FLAGS OldFlags;
FLAGS DefaultFlags;
FLAGS VintageFlags;

static void ReadFlags(void);
static void SetDefaultFlags(void);

static long int S2Int(char *s)     // string to integer
{
   return strtol(s,NULL,0);
}
static long int S2PosInt(char *s)  //string to positive integer
{
   int x = S2Int(s);
   if (x<0) return 0;
   return x;
}


static char *FlagsFile          = NULL;
static int   FlagsFileAvailable = 1;

void SetDefaultFlags()
{
#define DOIT_I(x,d,v) Flags.x = DefaultFlags.x ;
#define DOIT_S(x,d,v) free(Flags.x); Flags.x = strdup(DefaultFlags.x);
#define DOIT_L(x,d,v) DOIT_I(x,d,v)
   DOITALL;
#include "undefall.inc"
}

// return value:
// -1: error found
// 0: all is well
// 1: did request, program can stop.
#define checkax {if(ax>=argc-1){fprintf(stderr,"** missing parameter for '%s', exiting.\n",argv[ax]);return -1;}}

void InitFlags()
{
   // to make sure that strings in Flags are malloc'd
#define DOIT_I(x,d,v)  Flags.x = 0; DefaultFlags.x=d; VintageFlags.x=v;
#define DOIT_L DOIT_I
#define DOIT_S(x,d,v)  Flags.x = strdup(""); DefaultFlags.x=strdup(d); VintageFlags.x=strdup(v);
   DOITALL;
#include "undefall.inc"
}

#define handlestring(x) checkax; free(Flags.x); Flags.x = strdup(argv[++ax])

// argument is positive long int, set Flags.y to argument
#define handle_ia(x,y) else if (!strcmp(arg,# x)) \
   do { checkax; Flags.y=S2PosInt(argv[++ax]);} while(0)

// argument is long int, set Flags.y to argument
#define handle_im(x,y) else if (!strcmp(arg,# x)) \
   do { checkax; Flags.y=S2Int(argv[++ax]);} while(0)

// argument is char*, set Flags.y to argument
#define handle_is(x,y) else if (!strcmp(arg, #x)) \
   do { handlestring(y);} while(0)

// set Flags.y to z
#define handle_iv(x,y,z) else if (!strcmp(arg,# x)) \
   do { Flags.y = z; } while(0)

int HandleFlags(int argc, char*argv[])
{
   SetDefaultFlags();
   char *arg;
   int ax;
   int pass;
   for(pass = 1; pass <=2; pass++)
   {
      if (pass == 2)
      {
	 if(Flags.Defaults || Flags.NoConfig)
	    break;
	 ReadFlags();
      }
      for (ax=1; ax<argc; ax++) 
      {
	 arg = argv[ax];
	 if(!strcmp(arg, "-h") || !strcmp(arg, "-help")) 
	 {
	    docs_usage(0);
	    return 1;
	 }
	 else if(!strcmp(arg, "-H") || !strcmp(arg, "-manpage")) 
	 {
	    docs_usage(1);
	    return 1;
	 }
	 else if (!strcmp(arg, "-v") || !strcmp(arg, "-version")) 
	 {
	    PrintVersion();
	    return 1;
	 }
	 else if (strcmp(arg, "-nokeepsnow") == 0) 
	 {
	    Flags.NoKeepSnow = 1;
	    Flags.NoKeepSWin = 1;
	    Flags.NoKeepSBot = 1;
	    Flags.NoKeepSnowOnTrees = 1;
	 }
	 else if (strcmp(arg, "-keepsnow") == 0) 
	 {
	    Flags.NoKeepSnow = 0;
	    Flags.NoKeepSWin = 0;
	    Flags.NoKeepSBot = 0;
	    Flags.NoKeepSnowOnTrees = 0;
	 }
	 else if (strcmp(arg, "-vintage") == 0) {
#define DOIT_I(x,d,v) Flags.x = VintageFlags.x;
#define DOIT_L DOIT_I
#define DOIT_S(x,d,v) free(Flags.x); Flags.x = strdup(VintageFlags.x);
	    DOITALL
#include "undefall.inc"
	 }
	 else if (strcmp(arg, "-desktop") == 0) {
	    Flags.Desktop = 1;
	 }
	 handle_ia(-allworkspaces       ,AllWorkspaces                    );
	 handle_ia(-blowofffactor       ,BlowOffFactor                    );
	 handle_ia(-checkgtk            ,CheckGtk                         );
	 handle_ia(-cpuload             ,CpuLoad                          );
	 handle_ia(-doublebuffer        ,UseDouble                        );
	 handle_ia(-flakecountmax       ,FlakeCountMax                    );
	 handle_ia(-id                  ,WindowId                         );
	 handle_ia(-window-id           ,WindowId                         );
	 handle_ia(-maxontrees          ,MaxOnTrees                       );
	 handle_ia(-moon                ,Moon                             );
	 handle_ia(-moonspeed           ,MoonSpeed                        );
	 handle_ia(-moonsize            ,MoonSize                         );
	 handle_ia(-halo                ,Halo                             );
	 handle_ia(-halobrightness      ,HaloBright                       );
	 handle_im(-offsets             ,OffsetS                          );
	 handle_im(-offsetw             ,OffsetW                          );
	 handle_im(-offsetx             ,OffsetX                          );
	 handle_im(-offsety             ,OffsetY                          );
	 handle_ia(-santa               ,SantaSize                        );
	 handle_ia(-santaspeedfactor    ,SantaSpeedFactor                 );
	 handle_ia(-scale               ,Scale                            );
	 handle_ia(-snowflakes          ,SnowFlakesFactor                 );
	 handle_ia(-snowspeedfactor     ,SnowSpeedFactor                  );
	 handle_ia(-snowsize            ,SnowSize                         );
	 handle_ia(-ssnowdepth          ,MaxScrSnowDepth                  );
	 handle_ia(-stars               ,NStars                           );
	 handle_ia(-stopafter           ,StopAfter                        );
	 handle_ia(-theme               ,ThemeXsnow                       );
	 handle_ia(-treefill            ,TreeFill                         );
	 handle_ia(-trees               ,DesiredNumberOfTrees             );
	 handle_ia(-whirlfactor         ,WhirlFactor                      );
	 handle_ia(-windtimer           ,WindTimer                        );
	 handle_ia(-wsnowdepth          ,MaxWinSnowDepth                  );


	 handle_is(-display             ,DisplayName                      );
	 handle_is(-sc                  ,SnowColor                        );
	 handle_is(-tc                  ,TreeColor                        );
	 handle_is(-treetype            ,TreeType                         );

	 handle_iv(-above               ,BelowAll                 ,0      );
	 handle_iv(-defaults            ,Defaults                 ,1      );
	 handle_iv(-noblowsnow          ,BlowSnow                 ,0      );
	 handle_iv(-blowsnow            ,BlowSnow                 ,1      );
	 handle_iv(-noconfig            ,NoConfig                 ,1      );
	 handle_iv(-fluffy              ,NoFluffy                 ,0      );
	 handle_iv(-hidemenu            ,HideMenu                 ,1      );
	 handle_iv(-nofluffy            ,NoFluffy                 ,1      );
	 handle_iv(-noisy               ,Noisy                    ,1      );
	 handle_iv(-nokeepsnowonscreen  ,NoKeepSBot               ,1      );
	 handle_iv(-keepsnowonscreen    ,NoKeepSBot               ,0      );
	 handle_iv(-nokeepsnowontrees   ,NoKeepSnowOnTrees        ,1      );
	 handle_iv(-keepsnowontrees     ,NoKeepSnowOnTrees        ,0      );
	 handle_iv(-nokeepsnowonwindows ,NoKeepSWin               ,1      );
	 handle_iv(-keepsnowonwindows   ,NoKeepSWin               ,0      );
	 handle_iv(-nomenu              ,NoMenu                   ,1      );
	 handle_iv(-nometeorites        ,NoMeteorites             ,1      );
	 handle_iv(-meteorites          ,NoMeteorites             ,0      );
	 handle_iv(-norudolph           ,Rudolf                   ,0      );
	 handle_iv(-showrudolph         ,Rudolf                   ,1      );
	 handle_iv(-nosanta             ,NoSanta                  ,1      );
	 handle_iv(-root                ,ForceRoot                ,1      );
	 handle_iv(-showsanta           ,NoSanta                  ,0      );
	 handle_iv(-snow                ,NoSnowFlakes             ,0      );
	 handle_iv(-nosnow              ,NoSnowFlakes             ,1      );
	 handle_iv(-nosnowflakes        ,NoSnowFlakes             ,1      );
	 handle_iv(-notrees             ,NoTrees                  ,1      );
	 handle_iv(-showtrees           ,NoTrees                  ,0      );
	 handle_iv(-nowind              ,NoWind                   ,1      );
	 handle_iv(-wind                ,NoWind                   ,0      );
	 handle_iv(-xwininfo            ,XWinInfoHandling         ,1      );

	 // birds:

	 handle_ia(-anarchy             ,Anarchy                          );
	 handle_ia(-birdsonly           ,BirdsOnly                        );
	 handle_ia(-birdsspeed          ,BirdsSpeed                       );
	 handle_ia(-disweight           ,DisWeight                        );
	 handle_ia(-focuscentre         ,AttrFactor                       );
	 handle_ia(-followneighbours    ,FollowWeight                     );
	 handle_ia(-followsanta         ,FollowSanta                      );
	 handle_ia(-nbirds              ,Nbirds                           );
	 handle_ia(-neighbours          ,Neighbours                       );
	 handle_ia(-prefdistance        ,PrefDistance                     );
	 handle_ia(-showbirds           ,ShowBirds                        );
	 handle_ia(-showattr            ,ShowAttrPoint                    );
	 handle_ia(-viewingdistance     ,ViewingDistance                  );

	 handle_is(-birdscolor          ,BirdsColor                       );

	 else {
	    fprintf(stderr,"** unknown flag: '%s', exiting.\n",argv[ax]);
	    fprintf(stderr," Try: xsnow -h\n");
	    return -1;
	 }
      }
   }
   if ((Flags.SantaSize < 0) || (Flags.SantaSize > MAXSANTA)) {
      printf("** Maximum Santa is %d\n",MAXSANTA);
      return -1;
   }
   /* Eskimo warning */
   if (strstr(Flags.SnowColor,"yellow") != NULL)
      printf("\nWarning: don't eat yellow snow!\n\n");
   if (!strcmp(Flags.TreeType,"all"))
   {
      free(Flags.TreeType);
      Flags.TreeType = (char*) malloc(1+2+sizeof(DefaultFlags.TreeType));
      Flags.TreeType = strdup("0,");
      strcat(Flags.TreeType,DefaultFlags.TreeType);
   }
   if (Flags.SnowSize > 40)
   {
      printf("snowsize brought back from %d to 40\n",Flags.SnowSize);
      Flags.SnowSize = 40;
   }
   return 0;
}
#undef checkax
#undef handlestring
#undef handle_iv
#undef handle_is
#undef handle_ia

static xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath){

   xmlXPathContextPtr context;
   xmlXPathObjectPtr result;

   context = xmlXPathNewContext(doc);
   if (context == NULL) {
      //printf("Error in xmlXPathNewContext\n");
      return NULL;
   }
   result = xmlXPathEvalExpression(xpath, context);
   xmlXPathFreeContext(context);
   if (result == NULL) {
      //printf("Error in xmlXPathEvalExpression\n");
      return NULL;
   }
   if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
      xmlXPathFreeObject(result);
      //printf("No result\n");
      return NULL;
   }
   return result;
}

static void makeflagsfile()
{
   if (FlagsFile != NULL || FlagsFileAvailable == 0) return;
   char *h = getenv("HOME");
   if (h == NULL)
   {
      FlagsFileAvailable = 0;
      printf("Warning: cannot create or read $HOME/%s\n",FLAGSFILE);
      return;
   }
   FlagsFile = strdup(h);
   FlagsFile = (char *)realloc (FlagsFile,strlen(FlagsFile) + 1 + strlen(FLAGSFILE) + 1);
   strcat(FlagsFile,"/");
   strcat(FlagsFile,FLAGSFILE);
   P("FlagsFile: %s\n",FlagsFile);
}

void ReadFlags()
{
   xmlXPathObjectPtr result;
   xmlChar *value;
   xmlNodeSetPtr nodeset;
   long int intval;
   xmlDocPtr doc;
   makeflagsfile();
   if (!FlagsFileAvailable)
      return;
   doc = xmlParseFile(FlagsFile);
#define DOIT_I(x,d,v) \
   /* printf("%d:DOIT_I:%s\n",__LINE__,#x); */ \
   result = getnodeset(doc, BAD_CAST "//" # x); \
   if (result) {\
      nodeset = result->nodesetval; \
      value = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1); \
      if(value == NULL) \
      intval = 0; \
      else \
      intval = strtol((char*)value,NULL,0); \
      Flags.x = intval; \
      /* printf(# x ": %ld\n",(long int)Flags.x); */ \
      xmlFree(value); \
      xmlXPathFreeObject(result); \
   } 
#define DOIT_L(x,d,v) DOIT_I(x,d,v)
#define DOIT_S(x,d,v) \
   /* printf("%d:DOIT_S:%s\n",__LINE__,#x); */ \
   result = getnodeset(doc, BAD_CAST "//" # x); \
   if (result) {\
      nodeset = result->nodesetval; \
      value = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1); \
      free(Flags.x); \
      if (value == NULL) \
      Flags.x = strdup(""); \
      else \
      Flags.x = strdup((char*)value); \
      /* printf(# x ": %s\n",Flags.x); */  \
      xmlFree(value); \
      xmlXPathFreeObject(result); \
   } 
   DOIT;
   //printf("%d\n",__LINE__);
#include "undefall.inc"
   xmlFreeDoc(doc);
   xmlCleanupParser();
}

static void myxmlNewChild(xmlNodePtr node, xmlNsPtr p, char *name,long int value,char*fmt)
{
   char svalue[256];
   sprintf(svalue,fmt,value);
   xmlNewChild(node, p, BAD_CAST name, BAD_CAST svalue);
}

void WriteFlags()
{
   xmlDocPtr doc = NULL;
   xmlNodePtr root_node = NULL;
   doc = xmlNewDoc(BAD_CAST "1.0");
   root_node = xmlNewNode(NULL, BAD_CAST "xsnow_flags");
   xmlDocSetRootElement(doc, root_node);

#define DOIT_I(x,d,v) myxmlNewChild(root_node,NULL,(char *)# x,Flags.x,(char *)"%d");
#define DOIT_L(x,d,v) DOIT_I(x,d,v)
#define DOIT_S(x,d,v) xmlNewChild(root_node,NULL,BAD_CAST # x,BAD_CAST Flags.x);
   DOIT;
#include "undefall.inc"

   makeflagsfile();
   if (!FlagsFileAvailable) 
      return;
   xmlSaveFormatFileEnc(FlagsFile , doc, "UTF-8", 1);
   xmlFreeDoc(doc);
}
