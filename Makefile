# Copyright (C) 2025 Theo Niessink <theo@taletn.com>
# This work is free. You can redistribute it and/or modify it under the
# terms of the Do What The Fuck You Want To Public License, Version 2,
# as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

CPPFLAGS = /O2 /W3 /D _CRT_SECURE_NO_WARNINGS /nologo

all : mididump.exe

mididump.exe : mididump.cpp
	$(CPP) $(CPPFLAGS) $**

clean :
	@for %i in (mididump.obj mididump.exe) do @if exist %i del %i | echo del %i
