all: drinks_bar atom_supplier molecule_requester

drinks_bar: drinks_bar.cpp
		g++ -std=c++17 -Wall -Wextra -o drinks_bar drinks_bar.cpp

atom_supplier: atom_supplier.cpp
		g++ -std=c++17 -Wall -Wextra -o atom_supplier atom_supplier.cpp

molecule_requester: molecule_requester.cpp
		g++ -std=c++17 -Wall -Wextra -o molecule_requester molecule_requester.cpp

clean-ports:
	sudo fuser -k 5555/tcp || true
	sudo fuser -k 6666/udp || true

clean:
		rm -f drinks_bar atom_supplier molecule_requester
