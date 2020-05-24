#!/usr/bin/make -f

CC= gcc
CXX= g++
RM= rm -f

PKGCONFIG= pkg-config
PACKAGES= gl

CFLAGS= \
	-Wall \
	-fstack-protector-strong \
	-Wall \
	-Wformat \
	-Werror=format-security \
	-Wdate-time \
	-D_FORTIFY_SOURCE=2 \
	$(shell $(PKGCONFIG) --cflags $(PACKAGES))

LDFLAGS= \
	-Wl,--as-needed \
	-Wl,--no-undefined \
	-Wl,--no-allow-shlib-undefined

CSTD=-std=c11
CPPSTD=-std=c++17

OPTS= -O2 -g

DEFS=

INCS= \
	-I.

LIBS= \
	-lm -lfltk -lfltk_gl \
	$(shell $(PKGCONFIG) --libs $(PACKAGES))

PINOCCHIO_SRCS= \
	Pinocchio/attachment.cpp Pinocchio/discretization.cpp Pinocchio/indexer.cpp \
	Pinocchio/lsqSolver.cpp Pinocchio/mesh.cpp Pinocchio/graphutils.cpp \
	Pinocchio/intersector.cpp Pinocchio/matrix.cpp Pinocchio/skeleton.cpp Pinocchio/skeleton_csv.cpp \
	Pinocchio/embedding.cpp Pinocchio/pinocchioApi.cpp Pinocchio/PinnocchioCallback.cpp \
	Pinocchio/refinement.cpp Pinocchio/quatinterface.cpp

PINOCCHIO_OBJS= $(subst .cpp,.o,$(PINOCCHIO_SRCS))

BINARY= PinocchioDemo
BINARY_SRCS= DemoUI/DemoUI.cpp DemoUI/MyWindow.cpp DemoUI/defmesh.cpp \
	DemoUI/processor.cpp DemoUI/motion.cpp DemoUI/filter.cpp
BINARY_OBJS= $(subst .cpp,.o,$(BINARY_SRCS))

ATTACHWEIGHTS_SRCS= AttachWeights.cpp
ATTACHWEIGHTS_OBJS= $(subst .cpp,.o,$(ATTACHWEIGHTS_SRCS))

all: $(BINARY) AttachWeights

$(BINARY): $(PINOCCHIO_OBJS) $(BINARY_OBJS)
	$(CXX) $(CPPSTD) $(CSTD) $(LDFLAGS) -o $@ $(BINARY_OBJS) $(PINOCCHIO_OBJS) $(LIBS)

AttachWeights: $(PINOCCHIO_OBJS) $(ATTACHWEIGHTS_OBJS)
	$(CXX) $(CPPSTD) $(CSTD) $(LDFLAGS) -o $@ $(ATTACHWEIGHTS_OBJS) $(PINOCCHIO_OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CPPSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(CFLAGS)

%.o: %.cc
	$(CXX) $(CPPSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(CFLAGS)

%.o: %.c
	$(CC) $(CSTD) $(OPTS) -o $@ -c $< $(DEFS) $(INCS) $(CFLAGS)

depend: .depend

.depend: $(PINOCCHIO_SRCS) $(BINARY_SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPSTD) $(DEFS) $(INCS) $(CFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(PINOCCHIO_OBJS) $(BINARY_OBJS) $(ATTACHWEIGHTS_OBJS) $(BINARY) AttachWeights
	$(RM) -fv *~ .depend core *.out *.bak
	$(RM) -fv *.o *.a *~
	$(RM) -fv */*.o */*.a */*~

test: $(BINARY) AttachWeights
	./$(BINARY) data/test.obj
	./$(BINARY) data/test.obj -motion data/crossWalk.txt
	./AttachWeights data/test.obj -skelOut skeleton.csv -weightOut weights.csv -objOut model.obj

include .depend

.PHONY: all depend clean
