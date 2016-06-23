CC = g++
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = .
LIB_DIR = ../lib
BIN_DIR = ../bin

all: dirs $(BIN_DIR)/CCSSTest

dirs:
	@if [ ! -e ../bin ]; then mkdir ../bin; fi

SRC = \
CCSSTest.cpp \

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = \
$(CDEBUG) \
-std=c++11 \
-I. \
-I$(INC_DIR) \
-I../../CConfig/include \
-I../../CImageLib/include \
-I../../CFont/include \
-I../../CFile/include \
-I../../CStrUtil/include \
-I../../CUtil/include \
-I../../CMath/include \
-I/usr/include/freetype2 \

LFLAGS = \
$(LDEBUG) \
-L$(LIB_DIR) \
-L../../CCSS/lib \
-L../../CXML/lib \
-L../../COS/lib \
-L../../CFile/lib \
-L../../CStrUtil/lib \
-L../../CUtil/lib \
-L../../CRGBName/lib \
-L../../CRegExp/lib \

LIBS = \
-lCCSS -lCXML -lCFile \
-lCUtil -lCOS -lCRGBName -lCRegExp -lCStrUtil \
-ltre

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CSVGTest

.SUFFIXES: .cpp

.cpp.o:
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CCSSTest: $(OBJS) $(LIB_DIR)/libCCSS.a
	$(CC) $(LDEBUG) -o $(BIN_DIR)/CCSSTest $(OBJS) $(LFLAGS) $(LIBS)
