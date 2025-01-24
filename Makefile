busy: busy.cc
	$(CXX) busy.cc -o busy -O2 -Wall -Wextra

clean:
	rm -f busy

.PHONY: clean