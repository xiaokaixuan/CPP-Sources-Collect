DIR_Root     = .
DIR_BIN      = ./bin

OBJ_C = $(wildcard ${DIR_Root}/*.cpp)
SRC_H = $(wildcard *.hpp)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(patsubst %.cpp,${DIR_BIN}/%.o,$(notdir ${OBJ_C})))

TARGET = v4l2_Test

LIBS := $(shell pkg-config --libs opencv4)
# LIBS := $(shell /opt/atk-dlrv1126b-toolchain/bin/pkg-config --libs opencv4)

CXX = g++
# CXX = /opt/atk-dlrv1126b-toolchain/bin/aarch64-buildroot-linux-gnu-g++

CFLAGS += -pipe -O2 -Wall
CFLAGS += $(shell pkg-config --cflags opencv4)
# CFLAGS += $(shell /opt/atk-dlrv1126b-toolchain/bin/pkg-config --cflags opencv4)

${TARGET}:${DIR_BIN} ${OBJ_O}
	$(CXX) $(CFLAGS) $(OBJ_O) -o $@ $(LIBS) -lpthread
    
${DIR_BIN}/%.o:$(DIR_Root)/%.cpp $(SRC_H)
	$(CXX) $(CFLAGS) -c  $< -o $@
    
${DIR_BIN}:
	test -d ${DIR_BIN} || mkdir ${DIR_BIN}
	
.PHONY: clean
clean :
	rm -rf $(DIR_BIN) $(TARGET)

