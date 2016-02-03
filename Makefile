NAME=rogueliek
VERSION=0.1

SOURCEDIR=src/$(NAME)

BINDIR=bin

RM=rm -f
CFLAGS=-g -Wall -D_DEBUG -DCC_USE_ALL
LDLIBS=-lccFont -lccNoise -lccRandom -lccTrigonometry -lccore -lGL -lGLU -lGLEW -lm -lX11 -lXrandr -lXinerama -lXi -lpthread -llua5.3 -lpng

UCNAME=$(shell echo $(NAME) | tr a-z A-Z)
CFLAGS+=-D$(UCNAME)_VERSION=$(VERSION) 

SRCS=$(shell find $(SOURCEDIR) -name '*.c')
OBJS=$(subst .c,.o,$(SRCS))

all: $(BINDIR)/$(NAME)

$(BINDIR)/$(NAME): $(OBJS) .depend
	@(mkdir -p $(BINDIR))
	$(CC) $(LDFLAGS) -o $(BINDIR)/$(NAME) $(OBJS) $(LDLIBS)

.PHONY: clean
clean:
	$(RM) $(OBJS)

.PHONY: dist-clean
dist-clean: clean
	$(RM) *~ .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CC) $(CFLAGS) -MM $(SRCS) >>./.depend;

include .depend
