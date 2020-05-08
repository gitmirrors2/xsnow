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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include "kdesetbg.h"

static size_t mywrite(int fd, const void *buf, size_t count)
{
   size_t r = count;
   while (r != 0)
   {
      ssize_t rc = write(fd,(char*)buf + (count - r), r);
      if (rc < 0)
	 return rc;
      r -= rc;
   }
   return count;
}

int kdesetbg(const char *color)
{
   DBusConnection *connection;
   DBusError error;
   DBusMessage *message;
   DBusMessageIter iter;
   DBusBusType type = DBUS_BUS_SESSION;
   const char *address = NULL;

   char *s;
   if (color)
   {
      s = strdup("/tmp/colorXXXXXX.xpm");
      int f = mkstemps(s,4);
      // printf("%d: %s\n",__LINE__,s);
      if (f<0)
      {
	 printf("Cannot create %s\n",s);
	 return 1;
      }
      const char *xpm1 =
	 "/* XPM */\n"
	 "static char *x[] = {\n"
	 "\"1 1 1 1\",\n"
	 "\". c ";
      const char *xpm2 =
	 "\",\n"
	 "\".\"\n};\n";
      mywrite(f,xpm1, strlen(xpm1));
      mywrite(f,color,strlen(color));
      mywrite(f,xpm2, strlen(xpm2));
      close(f);
   }
   else
      s = strdup("");

   dbus_error_init (&error);

   connection = dbus_bus_get (type, &error);

   if (connection == NULL)
   {
      fprintf (stderr, "Failed to open connection to \"%s\" message bus: %s\n",
	    (address != NULL) ? address :
	    ((type == DBUS_BUS_SYSTEM) ? "system" : "session"),
	    error.message);
      dbus_error_free (&error);
      return -1;
   }

   message = dbus_message_new_method_call (NULL,
	 "/PlasmaShell",
	 "org.kde.PlasmaShell",
	 "evaluateScript");
   dbus_message_set_auto_start (message, TRUE);

   if (message == NULL)
   {
      fprintf (stderr, "Couldn't allocate D-Bus message\n");
      exit (1);
   }

   dbus_message_set_destination (message, "org.kde.plasmashell");

   dbus_message_iter_init_append (message, &iter);

   const char *c = 
      "var Desktops = desktops();\n"
      "for (i=0;i<Desktops.length;i++) {\n"
      "        d = Desktops[i];\n"
      "        d.wallpaperPlugin = \"org.kde.image\";\n"
      "        d.currentConfigGroup = Array(\"Wallpaper\",\n"
      "                                    \"org.kde.image\",\n"
      "                                    \"General\");\n"
      "        d.writeConfig(\"Image\", \"file://%s\");\n"
      "}\n";
   char *cc;
   cc  = malloc(strlen(c) + strlen(s) +1);
   sprintf(cc,c,s);
   // printf("%d: %s\n",__LINE__,cc);
   dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &cc);
   dbus_connection_send (connection, message, NULL);
   dbus_connection_flush (connection);
   dbus_message_unref (message);
   dbus_connection_unref (connection);
   free(cc);
   free(s);
   return 0;
}

