# Makefile

# 查找当前目录下所有的 .c 文件
SOURCES := $(wildcard *.c)

# 将所有的 .c 文件转换为 .o 文件
OBJECTS := $(SOURCES:.c=.o)

# 将文件名从 .c 转换为对应的可执行文件名
EXECUTABLES := $(SOURCES:.c=)

CFLAGS=-I. -g
LDFLAGS=-lpthread

CC=gcc

# 默认目标
# all: $(EXECUTABLES)
all: vxlan

# 编译 .c 文件为可执行文件
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# vxlan: main.c
vxlan: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 清理目标文件
clean:
	rm -f vxlan $(OBJECTS)


