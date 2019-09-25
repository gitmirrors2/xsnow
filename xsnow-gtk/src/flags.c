/* -copyright-
#-#
#-# xsnow: let it snow on your desktop
#-# Copyright (C) 1984,1988,1990,1993-1995,2000-2001 Rick Jansen
#-#               2019 Willem Vermin
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
#include "xsnow.h"
#include "docs.h"
#include "version.h"
#include "doit.h"

static void ReadFlags(void);
static void SetDefaultFlags(void);

static long int S2Int(char *s)     // string to integer
{
   return strtol(s,0,0);
}
static long int s2posint(char *s)  //string to positive integer
{
   int x = S2Int(s);
   if (x<0) return 0;
   return x;
}
void PrintVersion()
{
   printf("Xsnow-%s\n" "December 14th 2001 by Rick Jansen \n"
	 "March 2019 by Willem Vermin\n"
	 , VERSION);
   printf("configured with:");
#ifdef USEX11
   printf(" USEX11");
#endif
#ifdef DRAWFRAME
   printf(" DRAWFRAME");
#endif
   printf("\n");
}
#if 0
static void printversion()
{
   printf("Xsnow-%s\n" "December 14th 2001 by Rick Jansen \n"
	 "March 2019 by Willem Vermin\n"
	 , VERSION);
}
#endif

char *flagsfile = 0;

void SetDefaultFlags()
{
   //flags.window_id                = WINDOW_ID; 
#define DOIT_I(x) flags.x = DEFAULT_ ## x ;
#define DOIT_S(x) free(flags.x); flags.x = strdup(DEFAULT_ ## x);
#define DOIT_L(x) DOIT_I(x)
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S
}

// return value:
// -1: error found
// 0: all is well
// 1: did request, program can stop.
#define checkax {if(ax>=argc-1){fprintf(stderr,"** missing parameter for '%s', exiting.\n",argv[ax]);return -1;}}

void InitFlags()
{
   // to make sure that strings in flags are malloc'd
#define DOIT_I(x)  flags.x = 0;
#define DOIT_S(x)  flags.x = strdup("");
#define DOIT_L(x) DOIT_I(x)
   DOITALL;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S
   flags.UseX11 = 0;    // todo, now for testing
}

#define handlestring(x) checkax; free(flags.x); flags.x = strdup(argv[++ax])

#define handle_ia(x,y) else if (!strcmp(arg,# x)) \
   do { checkax; flags.y=s2posint(argv[++ax]);} while(0)

#define handle_im(x,y) else if (!strcmp(arg,# x)) \
   do { checkax; flags.y=S2Int(argv[++ax]);} while(0)

#define handle_iv(x,y,z) else if (!strcmp(arg,# x)) \
   do { flags.y = z; } while(0)

#define handle_is(x,y) else if (!strcmp(arg, #x)) \
   do { handlestring(y);} while(0)

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
	 if(flags.defaults || flags.noconfig)
	    break;
	 ReadFlags();
      }
      for (ax=1; ax<argc; ax++) {
	 arg = argv[ax];
	 if(!strcmp(arg, "-h") || !strcmp(arg, "-help")) {
	    docs_usage(0);
	    return 1;
	 }
	 else if(!strcmp(arg, "-H") || !strcmp(arg, "-manpage")) {
	    docs_usage(1);
	    return 1;
	 }
	 else if (strcmp(arg, "-version") == 0) {
	    PrintVersion();
	    return 1;
	 }
	 else if (strcmp(arg, "-nokeepsnow") == 0) {
	    flags.NoKeepSnow = 1;
	    flags.NoKeepSWin = 1;
	    flags.NoKeepSBot = 1;
	    flags.NoKeepSnowOnTrees = 1;
	 }
	 else if (strcmp(arg, "-vintage") == 0) {
	    flags.snowflakesfactor         = VINTAGE_snowflakesfactor;
	    flags.NoBlowSnow               = VINTAGE_NoBlowSnow;
	    flags.nstars                   = VINTAGE_nstars;
	    flags.desired_number_of_trees  = VINTAGE_desired_number_of_trees;
	    flags.NoKeepSnowOnTrees        = VINTAGE_NoKeepSnowOnTrees;
	    flags.NoMeteorites             = VINTAGE_NoMeteorites;
	    flags.NoRudolf                 = VINTAGE_NoRudolf;
	    flags.SantaSize                = VINTAGE_SantaSize;
	    free(flags.TreeType);
	    flags.TreeType                 = strdup(VINTAGE_TreeType);
	 }
	 else if (strcmp(arg, "-bg") == 0) {
	    handlestring(bgcolor);
	    flags.usebg   = 1;
	 }
	 else if (strcmp(arg, "-desktop") == 0) {
	    flags.desktop = 1;
	 }
	 else if (strcmp(arg, "-fullscreen") == 0) {
	    flags.fullscreen = 1;
	 }
	 else if (strcmp(arg, "-above") == 0) {
	    flags.below = 0;
	 }
	 else if(strcmp(arg,"-kdebg") == 0) {
	    flags.KDEbg  = 1;
	 }
	 else if(strcmp(arg,"-kde") == 0) {
	    KDEFLAGS;
	 }
	 else if(strcmp(arg,"-fvwm") == 0) {
	    FVWMFLAGS;
	 }
	 else if(strcmp(arg,"-gnome") == 0) {
	    GNOMEFLAGS;
	 }
	 handle_ia(-blowofffactor,blowofffactor);
	 handle_ia(-cpuload,cpuload);
	 handle_ia(-flakecountmax,flakecountmax);
	 handle_ia(-gnome,usealpha);
	 handle_ia(-id,window_id);
	 handle_ia(-maxontrees,maxontrees);
	 handle_im(-offsets,offset_s);
	 handle_im(-offsetw,offset_w);
	 handle_im(-offsetx,offset_x);
	 handle_im(-offsety,offset_y);
	 handle_ia(-santa,SantaSize);
	 handle_ia(-santaspeedfactor,SantaSpeedFactor);
	 handle_ia(-snowflakes,snowflakesfactor);
	 handle_ia(-snowspeedfactor,SnowSpeedFactor);
	 handle_ia(-ssnowdepth,MaxScrSnowDepth);
	 handle_ia(-stars,nstars);
	 handle_ia(-stopafter,stopafter);
	 handle_ia(-treefill,treefill);
	 handle_ia(-trees,desired_number_of_trees);
	 handle_ia(-whirlwactor,WhirlFactor);
	 handle_ia(-windtimer,WindTimer);
	 handle_ia(-wsnowdepth,MaxWinSnowDepth);

	 handle_is(-display,display_name);
	 handle_is(-sc,snowColor);
	 handle_is(-tc,trColor);
	 handle_is(-treetype,TreeType);

	 handle_iv(-defaults,defaults,1);
	 handle_iv(-exposures,exposures,True);
	 handle_iv(-noblowsnow,NoBlowSnow,1);
	 handle_iv(-noconfig,noconfig,1);
	 handle_iv(-noexposures,exposures,False);
	 handle_iv(-nofluffy,NoFluffy,1);
	 handle_iv(-nokeepsnowonscreen,NoKeepSBot,1);
	 handle_iv(-nokeepsnowontrees,NoKeepSnowOnTrees,1);
	 handle_iv(-nokeepsnowontrees,NoKeepSnowOnTrees,1);
	 handle_iv(-nokeepsnowonwindows,NoKeepSWin,1);
	 handle_iv(-nomenu,nomenu,1);
	 handle_iv(-nometeorites,NoMeteorites,1);
	 handle_iv(-noquiet,quiet,0);
	 handle_iv(-norudolf,NoRudolf,1);
	 handle_iv(-nosanta,NoSanta,1);
	 handle_iv(-nosnowflakes,NoSnowFlakes,1);
	 handle_iv(-notrees,NoTrees,1);
	 handle_iv(-nowind,NoWind,1);
	 handle_iv(-showstats,showstats,1);
	 handle_iv(-xwininfo,xwininfohandling,1);
	 else {
	    fprintf(stderr,"** unknown flag: '%s', exiting.\n",argv[ax]);
	    fprintf(stderr," Try: xsnow -h\n");
	    return -1;
	 }
      }
   }
   if ((flags.SantaSize < 0) || (flags.SantaSize > MAXSANTA)) {
      printf("** Maximum Santa is %d\n",MAXSANTA);
      return -1;
   }
   /* Eskimo warning */
   if (strstr(flags.snowColor,"yellow") != NULL)
      printf("\nWarning: don't eat yellow snow!\n\n");
   if (!strcmp(flags.TreeType,"all"))
   {
      free(flags.TreeType);
      flags.TreeType = strdup(ALLTREETYPES);
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
   if (flagsfile != 0) return;
   flagsfile = strdup(getenv("HOME"));
   flagsfile = realloc (flagsfile,strlen(flagsfile) + 1 + strlen(FLAGSFILE) + 1);
   strcat(flagsfile,"/");
   strcat(flagsfile,FLAGSFILE);
   //printf("%d: flagsfile: %s\n",__LINE__,flagsfile);
}

void ReadFlags()
{
   xmlXPathObjectPtr result;
   xmlChar *value;
   xmlNodeSetPtr nodeset;
   long int intval;
   xmlDocPtr doc;
   makeflagsfile();
   doc = xmlParseFile(flagsfile);
#define DOIT_I(x) \
   /* printf("%d:DOIT_I:%s\n",__LINE__,#x); */ \
   result = getnodeset(doc, BAD_CAST "//" # x); \
   if (result) {\
      nodeset = result->nodesetval; \
      value = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1); \
      if(value == NULL) \
      intval = 0; \
      else \
      intval = strtol((char*)value,0,0); \
      flags.x = intval; \
      /* printf(# x ": %ld\n",(long int)flags.x); */ \
      xmlFree(value); \
      xmlXPathFreeObject(result); \
   } 
#define DOIT_L(x) DOIT_I(x)
#define DOIT_S(x) \
   /* printf("%d:DOIT_S:%s\n",__LINE__,#x); */ \
   result = getnodeset(doc, BAD_CAST "//" # x); \
   if (result) {\
      nodeset = result->nodesetval; \
      value = xmlNodeListGetString(doc, nodeset->nodeTab[0]->xmlChildrenNode, 1); \
      free(flags.x); \
      if (value == NULL) \
      flags.x = strdup(""); \
      else \
      flags.x = strdup((char*)value); \
      /* printf(# x ": %s\n",flags.x); */  \
      xmlFree(value); \
      xmlXPathFreeObject(result); \
   } 
   DOIT;
   //printf("%d\n",__LINE__);
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S
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

#define DOIT_I(x) myxmlNewChild(root_node,NULL,# x,flags.x,"%d");
#define DOIT_L(x) DOIT_I(x)
#define DOIT_S(x) xmlNewChild(root_node,NULL,BAD_CAST # x,BAD_CAST flags.x);
   DOIT;
#undef DOIT_I
#undef DOIT_L
#undef DOIT_S

   makeflagsfile();
   xmlSaveFormatFileEnc(flagsfile , doc, "UTF-8", 1);
   xmlFreeDoc(doc);
}
