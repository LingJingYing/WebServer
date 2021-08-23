BUILD_DIR = ./build
DEST_DIR = ./bin
LIB_DIR = ./lib
DEST_EXE_NAME = main

all:
	@cd $(BUILD_DIR);  make -j4
clean/cmake:
	@rm $(BUILD_DIR)/* -rf
clean/lib:
	@rm $(LIB_DIR)/* -rf
clean/bin:
	@rm $(DEST_DIR)/* -rf
clean/all: clean/cmake clean/lib clean/bin
build:
	-@mkdir $(BUILD_DIR) $(LIB_DIR) $(DEST_DIR); cd $(BUILD_DIR); cmake ..

.PHONY:build all clean/cmake clean/lib clean/bin clean/all