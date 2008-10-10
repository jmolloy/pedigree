

OBJECTS := main.o Bitboard.o moves.o SideState.o BoardState.o Engine.o Minimax.o SearchAlgorithm.o CmdLineInterface.o Negascout.o Alphabeta.o Interface.o XboardInterface.o hashes.o StateStore.o

CXXFLAGS := -O3

all: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o chess $(OBJECTS) -lpthread

clean:
	rm $(OBJECTS)
