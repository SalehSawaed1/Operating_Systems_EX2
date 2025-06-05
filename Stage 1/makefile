# Updated Makefile for Stage 1 with Code Coverage support

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic --coverage

# Source files
WAREHOUSE_SRC = atom_warehouse.cpp
SUPPLIER_SRC = atom_supplier.cpp

# Output binaries
WAREHOUSE_BIN = atom_warehouse
SUPPLIER_BIN = atom_supplier

# Default target
all: $(WAREHOUSE_BIN) $(SUPPLIER_BIN)

# Compile atom_warehouse (TCP server)
$(WAREHOUSE_BIN): $(WAREHOUSE_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Compile atom_supplier (TCP client)
$(SUPPLIER_BIN): $(SUPPLIER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Run server on port 5555
run-server: $(WAREHOUSE_BIN)
	./$(WAREHOUSE_BIN) 5555

# Run client connecting to localhost:5555
run-client: $(SUPPLIER_BIN)
	./$(SUPPLIER_BIN) 127.0.0.1 5555

# Generate coverage report for warehouse
coverage: $(WAREHOUSE_BIN)
	./$(WAREHOUSE_BIN) 5555 & sleep 2; pkill -f $(WAREHOUSE_BIN) || true
	gcov $(WAREHOUSE_SRC)

# Clean up coverage and build artifacts
clean:
	rm -f $(WAREHOUSE_BIN) $(SUPPLIER_BIN) *.gcno *.gcda *.gcov

clean-ports:
	sudo fuser -k 5555/tcp || true
	sudo fuser -k 6666/udp || true
