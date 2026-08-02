# This makefile is for all platforms.

CC      = cc
CXX     = c++
CFLAGS  = -g -O3 -Wall -std=c++17 -Wno-psabi -pthread -MMD -MD -I. -I/usr/local/include
LIBS    = -lpthread -lutil -lmosquitto
LDFLAGS = -g -L/usr/local/lib

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

# The tests link against every object except the one holding the host's main(),
# and RemoteControl, which calls back into it.
TEST_SRCS = $(wildcard tests/*.cpp)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_DEPS = $(TEST_SRCS:.cpp=.d)
HOST_OBJS = $(filter-out MMDVM-Host.o RemoteControl.o,$(OBJS))

all:	MMDVM-Host

MMDVM-Host:	GitVersion.h $(OBJS)
		$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o MMDVM-Host

MMDVM-Host-Test:	GitVersion.h $(HOST_OBJS) $(TEST_OBJS)
		$(CXX) $(HOST_OBJS) $(TEST_OBJS) $(LDFLAGS) $(LIBS) -o MMDVM-Host-Test

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<
-include $(DEPS)
-include $(TEST_DEPS)

# Both of these have to be declared phony properly, because "tests" is also the
# name of a directory here and make would otherwise call the target up to date.
.PHONY: tests bench

# The tests that run anywhere.
tests: MMDVM-Host-Test
		./MMDVM-Host-Test

# Everything, including the checks that read the installed .ini file and talk to
# the gateways on this machine. Add --ini to point at a different .ini file.
bench: MMDVM-Host-Test
		./MMDVM-Host-Test --bench

.PHONY install:
install: all
		install -m 755 MMDVM-Host /usr/local/bin/

.PHONY install-service:
install-service: install /etc/MMDVM-Host.ini
		@useradd --user-group -M --system mmdvm --shell /bin/false || true
		@usermod --groups dialout --append mmdvm || true
		@mkdir /var/log/mmdvm || true
		@chown mmdvm:mmdvm /var/log/mmdvm
		@cp ./linux/systemd/mmdvmhost.service /lib/systemd/system/
		@systemctl enable mmdvmhost.service

/etc/MMDVM-Host.ini:
		@cp -n MMDVM-Host.ini /etc/MMDVM-Host.ini
		@sed -i 's/FilePath=./FilePath=\/var\/log\/mmdvm\//' /etc/MMDVM-Host.ini
		@sed -i 's/Daemon=0/Daemon=1/' /etc/MMDVM-Host.ini
		@chown mmdvm:mmdvm /etc/MMDVM-Host.ini

.PHONY uninstall-service:
uninstall-service:
		@systemctl stop mmdvmhost.service || true
		@systemctl disable mmdvmhost.service || true
		@rm -f /usr/local/bin/MMDVM-Host || true
		@rm -f /lib/systemd/system/mmdvmhost.service || true

clean:
		$(RM) MMDVM-Host MMDVM-Host-Test *.o *.d *.bak *~ GitVersion.h tests/*.o tests/*.d

# Export the current git version if the index file exists, else 000...
GitVersion.h:
ifneq ("$(wildcard .git/index)","")
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
else
	echo "const char *gitversion = \"0000000000000000000000000000000000000000\";" > $@
endif
