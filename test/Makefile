BIN := messaging-test
INCLUDE := -I../include -D_REENTRANT
LIBS = -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt \
			 -lboost_unit_test_framework-mt -lboost_signals-mt -lpthread -lstdc++

include ../makefiles/Makefile.common

test: $(BIN)
	./$<

