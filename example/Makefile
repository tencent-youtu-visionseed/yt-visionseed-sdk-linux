SDK_DIR=../yt-visionseed-sdk
INCLUDE_DIR=$(SDK_DIR)/include
LIB_DIR=$(SDK_DIR)/lib
BUILD_DIR=build

CFLAGS=-g3 -std=c++11 -pthread -DPB_FIELD_32BIT -DPB_ENABLE_MALLOC -DYTMSG_FULL

SRC=main.cpp
OBJS=$(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

.PHONY: clean

all: $(BUILD_DIR)/main

$(BUILD_DIR)/main: $(OBJS) $(LIB_DIR)/yt-visionseed.a
	g++ $(CFLAGS) $(OBJS) -o $(BUILD_DIR)/main -I$(INCLUDE_DIR) $(LIB_DIR)/yt-visionseed.a

$(BUILD_DIR)/%.o: %.cpp
	g++ $(CFLAGS) -I$(INCLUDE_DIR) -o $@ -c $<

$(BUILD_DIR)/%.o: %.c
	g++ $(CFLAGS) -I$(INCLUDE_DIR) -o $@ -c $<

clean:
	-rm $(BUILD_DIR)/*
