# Headerbar UI plugin for the DeaDBeeF audio player
#
# Copyright (C) 2015 Nicolai Syvertsen <saivert@gmail.com>
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


OUT?=ddb_misc_headerbar_GTK3.so

OUTDIR?=out

CFLAGS?=`pkg-config --cflags gtk+-3.0`

LIBS?=`pkg-config --libs gtk+-3.0`

CC?=gcc
CFLAGS+=-Wall -O2 -fPIC -std=c99 -D_GNU_SOURCE
LDFLAGS+=-shared

SOURCES=headerbarui.c appmenu.c resources.c
OBJ?=$(patsubst %.c, $(OUTDIR)/%.o, $(SOURCES))

define compile
	echo $(CC) $(CFLAGS) $1 $2 $< -c -o $@
	$(CC) $(CFLAGS) $1 $2 $< -c -o $@
endef

define link
	echo $(CC) $(LDFLAGS) $1 $2 $3 -o $@
	$(CC) $(LDFLAGS) $1 $2 $3 -o $@
endef

all: plugin

plugin: mkdir $(SOURCES) $(OUTDIR)/$(OUT)

mkdir:
	@echo "Creating build directory for GTK+3 version"
	@mkdir -p $(OUTDIR)

resources.c: headerbarui.gresource.xml headerbar.ui
	glib-compile-resources --target=resources.c --sourcedir=. --generate-source headerbarui.gresource.xml
	glib-compile-resources --target=resources.h --sourcedir=. --generate-header headerbarui.gresource.xml

$(OUTDIR)/$(OUT): $(OBJ)
	@echo "Linking GTK+3 version"
	@$(call link, $(OBJ), $(LIBS))
	@echo "Done!"

$(OUTDIR)/%.o: %.c
	@echo "Compiling $(subst $(OUTDIR)/,,$@)"
	@$(call compile, $(CFLAGS))

clean:
	@echo "Cleaning files from previous build..."
	@rm -r -f $(OUTDIR)

install:
	@install $(OUTDIR)/$(OUT) $(HOME)/.local/lib64/deadbeef/
