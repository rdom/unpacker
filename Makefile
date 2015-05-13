ARCH         := $(shell root-config --arch)
PLATFORM     := $(shell root-config --platform)

CXX           =
ObjSuf        = o
SrcSuf        = cxx
ExeSuf        =
DllSuf        = so
OutPutOpt     = -o # keep whitespace after "-o"

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLDFLAGS  := $(shell root-config --ldflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)
HASTHREAD    := $(shell root-config --has-thread)

CXX           = g++
CXXFLAGS      = -g -Wall -fPIC -Wunused-but-set-variable
LD            = g++
LDFLAGS       = -O3
SOFLAGS       = -shared

CXXFLAGS     += $(ROOTCFLAGS)
LDFLAGS      += $(ROOTLDFLAGS)
LIBS          = $(ROOTLIBS) $(SYSLIBS)
GLIBS         = $(ROOTGLIBS) $(SYSLIBS)

UNPACKERS       := $(wildcard *.$(SrcSuf)) Dict.$(SrcSuf)
UNPACKERO       := $(UNPACKERS:.$(SrcSuf)=.$(ObjSuf))
UNPACKER        = unpacker(ExeSuf)

OBJS          = $(UNPACKERO)

PROGRAMS      = $(UNPACKER)

.SUFFIXES: .$(SrcSuf) .$(ObjSuf) .$(DllSuf)

all:            $(PROGRAMS)

$(UNPACKER):    $(UNPACKERO)
		$(LD) $(LDFLAGS) $^ $(GLIBS) $(OutPutOpt)$@
		@echo "$@ done"

.$(SrcSuf).$(ObjSuf):
	$(CXX) $(CXXFLAGS) -c $<

Dict.$(SrcSuf): HldData.h LinkDef.h
	@echo "Generating dictionary $@..."
	rootcint -f $@ -c $(CXXFLAGS) -c $^	

clean:
	rm -f *.o *Dict.*

distclean:		clean
	rm -f unpacker
