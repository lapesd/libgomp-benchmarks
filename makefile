#
# Copyright(C) 2016-2017 Pedro H. Penna <pedrohenriquepenna@gmail.com>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
#

# Directories.
export BINDIR = $(CURDIR)/bin
export INCDIR = $(CURDIR)/include
export SRCDIR = $(CURDIR)/src

# Toolchain.
export CC = gcc

# Toolchain configuration.
export CFLAGS += -I $(INCDIR)
export CFLAGS += -ansi -std=c99 -pedantic -fopenmp
export CFLAGS += -Wall -Wextra -Werror
export CFLAGS += -O3 -D_POSIX_C_SOURCE=200112L

# Libraries.
export LIBS = -L $(CURDIR)/contrib/lapesd-libgomp/src/libgomp/build/.libs -lgomp -lm

# Executable file.
export EXEC = benchmark

# Builds everything.
all:
	mkdir -p $(BINDIR)
	$(CC) $(SRCDIR)/benchmark/*.c $(CFLAGS)  -o $(BINDIR)/$(EXEC).elf $(LIBS)

# Cleans compilation files.
clean:
	rm -f $(BINDIR)/$(EXEC).elf
