GPP=g++ -std=c++11
BIN_DIR=./bin
SRC_DIR=./src
BUILD_DIR=build
SRC=$(wildcard $(SRC_DIR)/*.cpp)
OBJ=$(addprefix $(BIN_DIR)/,$(notdir $(SRC:.cpp=.o)))

all: $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(GPP) -o $(BUILD_DIR)/server $(OBJ) Servidor.cpp -pthread
	$(GPP) -o $(BUILD_DIR)/client $(OBJ) Cliente.cpp -pthread
	
./bin/%.o: ./src/%.cpp
	@mkdir -p $(BIN_DIR)
	$(GPP) -c -o $@ $<

clean:
	rm -rf *.o *~ build bin
