CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address

CXX = g++ $(CXXFLAGS)
LDFLAGS =  -fsanitize=address

SRC = main.cpp
OBJ = $(SRC:.cc=.o)
EXEC = main.out

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LBLIBS)

clean:
	rm -rf $(OBJ) $(EXEC)