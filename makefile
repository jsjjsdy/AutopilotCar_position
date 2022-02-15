#指定项目.c存在的文件夹
SRC_DIR=  ./ ./demo 
#设置依赖文件夹搜索顺序，“：”为分隔符
vpath %.c ./: ./demo: ./inc: ./lib/gcc
#输出文件名
OUTPUT:=  ntripclient
#输出目录
OUTPUT_DIR= ./demo
#使用标准编译,-g:表示可以使用gdb调试,-Wall:所有错误和警告在编译的时候都打出来,-O2:-代码优化
CC:=	 gcc -g -Wall 
CXX:=	 g++
LIB_DIR:= -L ./lib/gcc 
#包含路径
INCLUDE_DIR:=	-I ./demo
INCLUDE_DIR+=	-I ./inc
INCLUDE_DIR+=	-I ./lib
INCLUDE_DIR+=	-I ./lib/gcc
INCLUDE_DIR+=	-I ./doc

#编译链库
LIB:= -lrt -lpthread -lm -lsixents-core-sdk
#遍历所有子目录中的.c文件
SRC = $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))
#指定.o文件的存放目录
OBJ_DIR= ./obj
#目标文件
OBJ:= $(patsubst %.c,%.o,$(SRC))
#去.o文件地址，只保留文件名
OBJ_WITHOUT_DIR = $(notdir $(OBJ))
#将生成的.o文件与指定目录相绑定
OBJ_WITH_DIR = $(addprefix $(OBJ_DIR)/,$(OBJ_WITHOUT_DIR))
#以下不需要变动
all: clean $(OBJ_WITHOUT_DIR) $(OUTPUT)

$(OUTPUT):$(OBJ_WITH_DIR)
	$(CC) -o $(OUTPUT_DIR)/$@ $^ $(LIB_DIR) $(LIB)
%.o:%.c
	$(CC) $(INCLUDE_DIR) $(LIB_DIR) $(LIB) -c $< -o $(OBJ_DIR)/$@

clean:
	rm -rf $(OUTPUT_DIR)/ntripclient  $(OBJ_DIR)/*.o

.PHONY: all clean
