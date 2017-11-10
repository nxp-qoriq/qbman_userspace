# Copyright 2016-2018 NXP
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#       * Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#       * Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in the
#         documentation and/or other materials provided with the distribution.
#       * Neither the name of NXP Semiconductor nor the
#         names of its contributors may be used to endorse or promote products
#         derived from this software without specific prior written permission.
#
#
# ALTERNATIVELY, this software may be distributed under the terms of the
# GNU General Public License ("GPL") as published by the Free Software
# Foundation, either version 2 of that License or (at your option) any
# later version.
#
# THIS SOFTWARE IS PROVIDED BY NXP Semiconductor ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL NXP Semiconductor BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

ARCH            ?= aarch64
CROSS_COMPILE	:= $(ARCH)-linux-gnu-
CC		:= $(CROSS_COMPILE)gcc
AR		:= $(CROSS_COMPILE)ar

CFLAGS		+= -pthread -lm -O2
CFLAGS		+= -Iinclude
CFLAGS		+= -W -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes
CFLAGS		+= -Wmissing-declarations -Wold-style-definition
CFLAGS		+= -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual
CFLAGS		+= -Wformat-nonliteral -Wformat-security -Wundef
CFLAGS		+= -Wwrite-strings -Wno-error

LIB_DIR     	:= lib_$(ARCH)_static

TARGET = $(LIB_DIR)/libqbman.a

OBJECTS = $(patsubst %.c, %.o, $(wildcard driver/*.c))
HEADERS = $(wildcard */*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	mkdir -p $(LIB_DIR)
	ar rcs $@ $(OBJECTS)

clean:
	rm -f driver/*.o
	rm -rf $(LIB_DIR)

all: $(TARGET)

.PHONY: all clean
