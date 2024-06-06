DIR_Root     = .
DIR_BIN      = ./bin

OBJ_C = $(wildcard ${DIR_Root}/*.cpp)
SRC_H = $(wildcard *.hpp)
OBJ_O = $(patsubst %.c,${DIR_BIN}/%.o,$(patsubst %.cpp,${DIR_BIN}/%.o,$(notdir ${OBJ_C})))

TARGET = v4l2_Test

LIB = -I/usr/include/opencv4 /usr/lib/arm-linux-gnueabihf/libopencv_*.so

CXX = g++
CFLAGS += -pipe -O2 -Wall

${TARGET}:${DIR_BIN} ${OBJ_O}
	$(CXX) $(CFLAGS) $(OBJ_O) -o $@ $(LIB) -lpthread
    
${DIR_BIN}/%.o:$(DIR_Root)/%.cpp $(SRC_H)
	$(CXX) $(CFLAGS) -c  $< -o $@
    
${DIR_BIN}:
	test -d ${DIR_BIN} || mkdir ${DIR_BIN}
	
.PHONY: clean
clean :
	rm -rf $(DIR_BIN) $(TARGET)

