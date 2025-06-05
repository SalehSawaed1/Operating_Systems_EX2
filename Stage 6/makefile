CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic

# Executable names
SERVER = drinks_bar
SUPPLIER = atom_supplier
REQUESTER = molecule_requester

# Source files
SERVER_SRC = drinks_bar.cpp
SUPPLIER_SRC = atom_supplier.cpp
REQUESTER_SRC = molecule_requester.cpp

all: $(SERVER) $(SUPPLIER) $(REQUESTER)

$(SERVER): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(SUPPLIER): $(SUPPLIER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(REQUESTER): $(REQUESTER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

run-server:
	./$(SERVER) -T 5555 -U 6666 -s /tmp/stream_sock -d /tmp/dgram_sock -f inventory.txt -t 60

clean:
	rm -f $(SERVER) $(SUPPLIER) $(REQUESTER) inventory.txt
	rm -f /tmp/stream_sock /tmp/dgram_sock
