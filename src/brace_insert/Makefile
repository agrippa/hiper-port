# This should only be run with the same compiler as LLVM/Clang was compiled with
# (which may not be the system default). You can force it to be a custom
# compiler by setting a GXX environment variable, e.g.:
#
# GXX=/usr/local/bin/g++ make
#

GXX ?= g++
EXE=brace_insert

LLVM_CONFIG=$(shell find ${LLVM_INSTALL}/**/bin -name llvm-config)
GXX_FLAGS=-O0 -g `${LLVM_CONFIG} --cxxflags`
INCLUDES=-I${LLVM_HOME}/tools/clang/include -I${LLVM_INSTALL}/tools/clang/include
ARCH=$(shell uname -s)

ifeq ($(ARCH),Darwin)
LIBS=-lclangAST -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit \
	 -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema \
	 -lclangEdit -lclangASTMatchers -lclangRewrite -lclangRewriteFrontend \
	 -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers \
	 -lclangStaticAnalyzerCore -lclangSerialization \
	 -lclangTooling `${LLVM_CONFIG} --ldflags --libs --system-libs`
else
LIBS=-Wl,--start-group -lclangAST -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit \
	 -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema \
	 -lclangEdit -lclangASTMatchers -lclangRewrite -lclangRewriteFrontend \
	 -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers \
	 -lclangStaticAnalyzerCore -lclangSerialization \
	 -lclangTooling -Wl,--end-group `${LLVM_CONFIG} --ldflags --libs --system-libs`
endif

OBJS=Driver.o BraceInserter.o

${EXE}: ${OBJS}
	${GXX} ${GXX_FLAGS} ${INCLUDES} ${OBJS} -o ${EXE} ${LIBS}

%.o: %.cpp
	${GXX} ${GXX_FLAGS} ${INCLUDES} $< --compile -fPIC -o $@


clean:
	rm -f *.o ${EXE}
