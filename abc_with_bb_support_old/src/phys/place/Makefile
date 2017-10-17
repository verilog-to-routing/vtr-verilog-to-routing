TARGETS = place_test BookshelfView.class

CFLAGS = -g -pedantic -Wall

STATIC_LIBS = libhmetis.a
DYNAMIC_LIBS = -lm

OBJECTS = place_test.o place_qpsolver.o place_base.o place_pads.o place_genqp.o place_gordian.o \
	place_partition.o place_legalize.o place_bin.o


# For hMetis free code, uncomment the following lines
#
# CFLAGS = -g -pedantic -Wall -DNO_HMETIS
# STATIC_LIBS =


all: $(TARGETS)

%.o: %.c *.h
	gcc $(CFLAGS) -c -o $@ $<

place_test: $(OBJECTS)
	gcc *.o $(STATIC_LIBS) $(DYNAMIC_LIBS) -o place_test

BookshelfView.class: BookshelfView.java
	javac BookshelfView.java

clean:
	rm -rf *.o place_test *.class *~
