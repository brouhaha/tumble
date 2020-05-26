# tumble: build a PDF file from image files
# Makefile
# Copyright 2001, 2002, 2003, 2017 Eric Smith <spacewar@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.  Note that permission is
# not granted to redistribute this program under the terms of any
# other version of the General Public License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111  USA
#
#  2010-09-02 [JDB] Allow building in a directory separate from the source and
#                   add PNG and blank-page support files.  Also change the
#                   "include" of dependencies to suppress an error if the
#                   dependency files are not present.


# Conditionals:  uncomment the following defines as nessary.  Note that a
# "0" value is considered true by make, so to disable conditionals comment
# them out or set them to a null string.

#DEBUG=1
#EFENCE=1
#STATIC=1
CTL_LANG=1


CFLAGS = -Wall -Wno-unused-function -Wno-unused-but-set-variable -I/usr/include/netpbm
LDFLAGS =
LDLIBS = -ltiff -ljpeg -lnetpbm -lz -lm

ifdef DEBUG
CFLAGS := $(CFLAGS) -g
LDFLAGS := $(LDFLAGS) -g
else
CFLAGS := $(CFLAGS) -O3
endif

ifdef EFENCE
LDLIBS := $(LDLIBS) -lefence -lpthread
endif

ifdef STATIC
LDLIBS := -Wl,-static $(LDLIBS)
endif


LEX = flex
YACC = bison
YFLAGS = -d -v


# -----------------------------------------------------------------------------
# You shouldn't have to change anything below this point, but if you do please
# let me know why so I can improve this Makefile.
# -----------------------------------------------------------------------------

VERSION = 0.36

PACKAGE = tumble

TARGETS = tumble

CSRCS = tumble.c semantics.c tumble_input.c \
	tumble_tiff.c tumble_jpeg.c tumble_pbm.c tumble_png.c tumble_blank.c \
	bitblt.c bitblt_table_gen.c bitblt_g4.c g4_table_gen.c \
	pdf.c pdf_util.c pdf_prim.c pdf_name_tree.c \
	pdf_bookmark.c pdf_page_label.c \
	pdf_text.c pdf_g4.c pdf_jpeg.c pdf_png.c
OSRCS = scanner.l parser.y
HDRS = tumble.h tumble_input.h semantics.h bitblt.h bitblt_tables.h \
	pdf.h pdf_private.h pdf_util.h pdf_prim.h pdf_name_tree.h
MISC = COPYING README INSTALL Makefile

DISTFILES = $(MISC) $(HDRS) $(CSRCS) $(OSRCS)
DISTNAME = $(PACKAGE)-$(VERSION)

BIN_DISTFILES = COPYING README $(TARGETS)


AUTO_CSRCS = bitblt_tables.c g4_tables.c
AUTO_HDRS = bitblt_tables.h g4_tables.h

ifdef CTL_LANG
AUTO_CSRCS += scanner.c parser.tab.c
AUTO_HDRS += parser_tab.h
AUTO_MISC = parser.output
endif


CDEFINES = -DTUMBLE_VERSION=$(VERSION)

ifdef CTL_LANG
CDEFINES += -DCTL_LANG
endif

CFLAGS := $(CFLAGS) $(CDEFINES)


-include Maketest


all: $(TARGETS) $(TEST_TARGETS)


TUMBLE_OBJS = tumble.o semantics.o tumble_input.o \
		tumble_tiff.o tumble_jpeg.o tumble_pbm.o tumble_png.o tumble_blank.o \
		bitblt.o bitblt_g4.o bitblt_tables.o g4_tables.o \
		pdf.o pdf_util.o pdf_prim.o pdf_name_tree.o \
		pdf_bookmark.o pdf_page_label.o \
		pdf_text.o pdf_g4.o pdf_jpeg.o pdf_png.o 

ifdef CTL_LANG
TUMBLE_OBJS += scanner.o parser.tab.o
endif

tumble: $(TUMBLE_OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@


bitblt_tables.h: bitblt_table_gen
	./bitblt_table_gen -h >bitblt_tables.h

bitblt_tables.c: bitblt_table_gen
	./bitblt_table_gen -c >bitblt_tables.c

bitblt_table_gen: bitblt_table_gen.o

g4_tables.h: g4_table_gen
	./g4_table_gen -h >g4_tables.h

g4_tables.c: g4_table_gen
	./g4_table_gen -c >g4_tables.c

g4_table_gen: g4_table_gen.o


dist: $(DISTFILES)
	-rm -rf $(DISTNAME)
	mkdir $(DISTNAME)
	for f in $(DISTFILES); do ln $$f $(DISTNAME)/$$f; done
	tar --gzip -chf $(DISTNAME).tar.gz $(DISTNAME)
	-rm -rf $(DISTNAME)


bin-dist-rh: $(BIN_DISTFILES) /etc/redhat-release
	tar --gzip -chf $(DISTNAME)-rh$(shell sed 's/^Red Hat Linux release \([0-9][0-9.]*\) (.*)/\1/' </etc/redhat-release).tar.gz $(BIN_DISTFILES)

bin-dist-fc: $(BIN_DISTFILES) /etc/fedora-release
	tar --gzip -chf $(DISTNAME)-fc$(shell sed 's/^Fedora Core release \([0-9][0-9.]*\) (.*)/\1/' </etc/fedora-release).tar.gz $(BIN_DISTFILES)

clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_CSRCS) $(AUTO_HDRS) $(AUTO_MISC)

very_clean:
	rm -f *.o *.d $(TARGETS) $(AUTO_CSRCS) $(AUTO_HDRS) $(AUTO_MISC) \
		*~ *.pdf

wc:
	wc -l $(HDRS) $(CSRCS) $(OSRCS) $(MISC)

ls-lt:
	ls -lt $(HDRS) $(CSRCS) $(OSRCS) $(MISC)


%.tab.c %.tab.h %.output: %.y
	$(YACC) $(YFLAGS) $<


ALL_CSRCS = $(CSRCS) $(AUTO_CSRCS) $(TEST_CSRCS)

DEPENDS = $(ALL_CSRCS:.c=.d)

%.d: %.c
	$(CC) -M -MG $(CFLAGS) $< | sed -e 's@ \([A-Za-z]\):/@ /\1/@g' -e 's@^\(.*\)\.o:@\1.d \1.o:@' > $@

-include $(DEPENDS)

CHANGELOG.html: CHANGELOG.md
	cmark $< >$@
