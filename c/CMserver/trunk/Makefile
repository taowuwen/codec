
CM_VERSION=1.0.0.1
CM_OSNAME=$(shell uname -p)

########

CC=gcc
GCC=g++
DEBUG?=-g
#OPTIMIZATION?=-O2
WARN?=-Wall -Wshadow -Wpointer-arith -Wmissing-declarations -Wnested-externs
#WARN?=-Wall -Wshadow -Wpointer-arith -Wmissing-declarations

#Ultrasparc64 users experiencing SIGBUS should try the following gcc options
#(thanks to Robert Gibbon)
PLATOPTS_SPARC64=-mcpu=ultrasparc -pipe -fomit-frame-pointer -ffast-math -finline-functions -fweb -frename-registers -mapp-regs

CM_DEFINES=
CM_OBJS_OPT=
LIBS_EDGE_OPT=

#CM_OPTION_AES?="yes"
CM_OPTION_AES=no

ifeq ($(CM_OPTION_AES), "yes")
    CM_DEFINES+="-DP2P_HAVE_AES"
    LIBS_EDGE_OPT+=-lcrypto
endif

CFLAGS+=$(DEBUG) $(OPTIMIZATION) $(OPTIONS) $(PLATOPTS) $(CM_DEFINES)

INSTALL=install
MKDIR=mkdir -p

INSTALL_PROG=$(INSTALL) -m755
INSTALL_DOC=$(INSTALL) -m644


# DESTDIR set in debian make system
PREFIX?=$(DESTDIR)/usr
#BINDIR=$(PREFIX)/bin
SBINDIR=$(PREFIX)/sbin
MANDIR?=$(PREFIX)/share/man
MAN1DIR=$(MANDIR)/man1
MAN7DIR=$(MANDIR)/man7
MAN8DIR=$(MANDIR)/man8

CM_LIB=n2n.a
CM_OBJS=n2n.o n2n_keyfile.o wire.o minilzo.o twofish.o \
         transform_null.o transform_tf.o transform_aes.o \
         tuntap_freebsd.o tuntap_netbsd.o tuntap_linux.o tuntap_osx.o version.o
LIBS_EDGE+=$(LIBS_EDGE_OPT)
LIBS_SN=

#For OpenSolaris (Solaris too?)
ifeq ($(shell uname), SunOS)
LIBS_EDGE+=-lsocket -lnsl
LIBS_SN+=-lsocket -lnsl
endif

APPS=edge
APPS+=CMserver



all: $(APPS)

edge: edge.cpp $(CM_LIB) n2n_wire.h n2n.h Makefile
	$(MAKE) -C json
	cp json/libjson.a ./
	$(GCC) $(CFLAGS) edge.cpp $(CM_LIB) $(LIBS_EDGE) libjson.a -o edge
test: test.cpp $(CM_LIB) n2n_wire.h n2n.h Makefile
	$(GCC) $(CFLAGS) test.cpp $(CM_LIB) $(LIBS_EDGE) -o test

CMserver: sn.cpp $(CM_LIB) n2n.h Makefile
	$(GCC) $(CFLAGS) sn.cpp $(CM_LIB) $(LIBS_SN) libjson.a -o CMserver

benchmark: benchmark.cpp $(CM_LIB) n2n_wire.h n2n.h Makefile
	$(GCC) $(CFLAGS) benchmark.c $(CM_LIB) $(LIBS_SN) -o benchmark

.c.o: n2n.h n2n_keyfile.h n2n_transforms.h n2n_wire.h twofish.h Makefile
	$(CC) $(CFLAGS) $(WARN) -c $<
.cpp.o: n2n.h n2n_keyfile.h n2n_transforms.h n2n_wire.h twofish.h Makefile
	$(GCC) $(CFLAGS) -c $<


$(CM_LIB): $(CM_OBJS)
	ar rcs $(CM_LIB) $(CM_OBJS)
#	$(RANLIB) $@

version.o: Makefile
	$(CC) $(CFLAGS) -DP2P_VERSION='"$(CM_VERSION)"' -DP2P_OSNAME='"$(CM_OSNAME)"' -c version.c

clean:
	$(MAKE) -C json clean
	rm -rf $(CM_OBJS) $(CM_LIB) $(APPS) libjson.a test *.dSYM *~

install:  CMserver
	echo "MANDIR=$(MANDIR)"
	$(MKDIR) $(SBINDIR) $(MAN1DIR) $(MAN7DIR) $(MAN8DIR)
	$(INSTALL_PROG) p2pserver $(SBINDIR)/

