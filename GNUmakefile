# Copyright (C) 2025 Theo Niessink <theo@taletn.com>
# This work is free. You can redistribute it and/or modify it under the
# terms of the Do What The Fuck You Want To Public License, Version 2,
# as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

CXXFLAGS = -O2 -Wall

all : mididump

mididump : mididump.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean :
	rm -f mididump
