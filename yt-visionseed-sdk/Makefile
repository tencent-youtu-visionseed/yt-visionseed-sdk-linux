INCLUDE_DIR=include
SRC_DIR=src
LIB_DIR=lib
BUILD_DIR=build

CFLAGS=-O3 -std=c++11 -pthread -DPB_FIELD_32BIT -DPB_ENABLE_MALLOC -DYTMSG_FULL -fPIC

SRC=$(wildcard $(SRC_DIR)/*.c)
SRC+=$(wildcard $(SRC_DIR)/*.cpp)

OBJS=$(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

.PHONY: clean

all: $(LIB_DIR)/yt-visionseed.a

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CFLAGS) -I$(INCLUDE_DIR) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	g++ $(CFLAGS) -I$(INCLUDE_DIR) -o $@ -c $<

$(LIB_DIR)/yt-visionseed.a: $(OBJS)
	ar rcs $@ $^

clean:
	-rm $(BUILD_DIR)/*
	-rm $(LIB_DIR)/*
