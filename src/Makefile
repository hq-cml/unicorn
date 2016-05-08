DEBUG  = -g 

# I/O复用模式：
# epoll(默认) | select 
# [__MSD_EPOLL_MODE__ / __MSD_SELECT_MODE__]
IO_MULTI_MODE = -D__UNC_EPOLL_MODE__

CC     = gcc
CFLAGS = $(DEBUG) $(IO_MULTI_MODE) -Wall
LIB    = -ldl -rdynamic -lpthread
INC    = -I../inc

TARGET = Unicorn 

OBJECT  = unc_ae.o unc_anet.o unc_dlist.o unc_string.o unc_so.o unicorn.o
           
all: $(TARGET) 

#不关心头文件，每次均执行make clean
$(OBJECT): %.o:%.c 
	$(CC) $(CFLAGS) $(INC) $< -c -o $@
    
$(TARGET):$(OBJECT)
	$(CC) $(CFLAGS) $(INC) $(LIB) $(OBJECT) $(OBJECT1) -o $@

.PHONY :clean install
install:
	install $(TARGET) ../bin/

clean:
	rm -f *.o
	rm $(TARGET) 
