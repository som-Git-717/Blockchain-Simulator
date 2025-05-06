CXX = g++
CXXFLAGS = -w -std=c++17 -O3 -g -I. -I/opt/homebrew/opt/openssl/include
LDFLAGS = -lpthread -L/opt/homebrew/opt/openssl/lib -lcrypto -lssl

CONSENSUS_DIRS = poa pob pos poet pow

all: $(CONSENSUS_DIRS:%=%.out)
pow: pow.out
poa: poa.out
pob: pob.out
pos: pos.out
poet: poet.out
%.out:
	$(CXX) $(CXXFLAGS) \
		$*/main.cpp \
		$*/block.cpp \
		$*/blockchain.cpp \
		$*/transaction.cpp \
		$*/sha256.cpp \
		$*/$*_consensus.cpp \
		-o $@ $(LDFLAGS)

clean:
	rm -f *.out
	for dir in $(CONSENSUS_DIRS); do rm -f $$dir/*.o $$dir/*.out; done
