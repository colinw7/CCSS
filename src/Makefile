CC = g++
AR = ar
RM = rm

CDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib
BIN_DIR = ../bin

all: dirs $(LIB_DIR)/libCCSS.a

dirs:
	@if [ ! -e ../obj ]; then mkdir ../obj; fi
	@if [ ! -e ../lib ]; then mkdir ../lib; fi

SRC = \
CCSS.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
-std=c++14 \
$(CDEBUG) \
-I. \
-I$(INC_DIR) \
-I../../CXML/include \
-I../../CFile/include \
-I../../CStrUtil/include \
-I../../CRegExp/include \
-I../../CUtil/include \

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(LIB_DIR)/libCCSS.a

.SUFFIXES: .cpp

$(OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(LIB_DIR)/libCCSS.a: $(OBJS)
	$(AR) crv $(LIB_DIR)/libCCSS.a $(OBJS)
