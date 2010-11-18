YACC = bison
YFLAGS = -o y.tab.c -t
CC = gcc -g

all: validate

bogus: schema test

#keep the intermediate files around for debug
# param is really only useful for debugging
# Instructions:
# gdb param
# > br main
# > r
# ...step to after "sts=yyparse()"
# > p g_param
# ...loaded parameters will be displayed
param.o: param.c
param.y param.h: schema parameters.schema
	./schema parameters.schema param.y param.h

validate.o: param.h
validate: validate.c param.o
	$(CC) $(CFLAGS) $(LDFLAGS) $+ -o $@

debug: schema
	gdb ./schema

clean:
	-$(RM) schema
	-$(RM) schema.c
	-$(RM) param
	-$(RM) validate
	-$(RM) param.y
	-$(RM) param.c
	-$(RM) param.h
	-$(RM) -rf *.dSYM
	-$(RM) *.o
