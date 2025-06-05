CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -Werror
TARGETS = molecule_supplier molecule_requester

all: $(TARGETS)

molecule_supplier: molecule_supplier.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

molecule_requester: molecule_requester.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# Sends atoms via TCP using netcat + sleep to ensure the server receives it
add-hydrogen:
	(echo "ADD HYDROGEN 10"; sleep 1) | nc 127.0.0.1 5555

add-oxygen:
	(echo "ADD OXYGEN 5"; sleep 1) | nc 127.0.0.1 5555

run-server:
	./molecule_supplier 5555 6666

run-client:
	./molecule_requester 127.0.0.1 6666

clean-ports:
	sudo fuser -k 5555/tcp || true
	sudo fuser -k 6666/udp || true

clean:
	rm -f $(TARGETS)
