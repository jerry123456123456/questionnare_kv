
CC = gcc
FLAGS = -I ./Ntyco/core/ -L ./Ntyco/ -lntyco -lpthread -luring -ldl
SRCS = kvstore.c ntyco.c proactor.c kvs_array.c kvs_rbtree.c kvs_hash.c
TESTCASE_SRCS = testcase.c
TARGET = kvstore
SUBDIR = ./Ntyco/
TESTCASE = testcase

OBJS = $(SRCS:.c=.o)


all: $(SUBDIR) $(TARGET) $(TESTCASE)

$(SUBDIR): ECHO
	make -C $@

ECHO:
	@echo $(SUBDIR)

$(TARGET): $(OBJS) 
	$(CC) -o $@ $^ $(FLAGS)

$(TESTCASE): $(TESTCASE_SRCS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(FLAGS) -c $^ -o $@

clean: 
	rm -rf $(OBJS) $(TARGET) $(TESTCASE)



