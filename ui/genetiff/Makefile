CC = gcc
LDFLAGS =
CFLAGS = 
pmodules = utilities.o image_lib.o
cmodules = tiff_adapt.o seq.o tiff_io.o tiff_image.o 
modules = $(pmodules) $(cmodules)
TESTS = seqtest
LIBS = libreader.so

all: checkos $(LIBS)

.PHONY: checkos
checkos:
ifndef OS
OS =
else
RM = del 
endif

tests: $(TESTS)

%.o: %.p
	awk -f manager.awk $< > $*.c
	$(CC) $(CFLAGS) -c $*.c

libreader.so: $(modules)
ifneq ($(OS), Windows_NT)
	@echo Windows not detected.  Building $@
	libtool -dynamic -o $@ $+ -lc $(LDFLAGS)
else
	@echo Windows detected.  Building $(@:.so=.dll)
	$(CC) -shared $(CFLAGS) -o $(@:.so=.dll) $+ $(LDFLAGS)  
endif

seqtest: $(filter-out seq.o,$(modules)) seq.c
	$(CC) $(CFLAGS) -DSEQ_TESTING -DSEQ_PARANOID seq.c $(filter-out seq.o,$(modules)) -o seqtest

.PHONY: clean
clean: checkos
	-$(RM) *.o $(pmodules:.o=.c)
	-$(RM) *.so
	-$(RM) *.dll
	-$(RM) *.exe
	-$(RM) *.pyc
	-$(RM) -r *.dSYM
	-$(RM) $(LIBS)
	-$(RM) $(TESTS)

superclean: clean
	-$(RM) *.tif
