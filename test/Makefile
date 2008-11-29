BIN := messaging-test
INCLUDE := -I../include -DPHOENIX_LIMIT=4 -D_REENTRANT
LIBS = -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt \
			 -lboost_unit_test_framework-mt -lpthread -lstdc++

include ../makefiles/Makefile.common

test: $(BIN)
	./$<

