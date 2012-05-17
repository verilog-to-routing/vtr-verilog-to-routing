CC = gcc
AR = ar

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -Iinclude
LIB_FLAGS = rcs

EXE = read_arch

FLAGS = $(OPT_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)
#FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)
FLAGS += -MD -MP

SRC = read_xml_arch_file.c read_xml_util.c ezxml.c ReadLine.c util.c
OBJS = $(SRC:.c=.o)

DEPS = $(OBJS:.o=.d) main.d

libvpr_6.a: $(OBJS)
	$(AR) $(LIB_FLAGS) $@ $(OBJS)
	cp libvpr_6.a libvpr.a

# Standalone executable to test architecture reader
$(EXE): main.o libvpr_6.a
	$(CC) main.o -o $(EXE) $(INC_FLAGS) -L. -lm -lvpr_6
	cp libvpr_6.a libvpr.a
 
 
%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 
 
-include $(DEPS)



clean :
	rm -f libvpr_6.a
	rm -f libvpr.a
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f read_arch

