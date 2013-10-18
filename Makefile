#### BUILD modalities
# use 
#   $ make BUILD=release 
# to compile different binaries
BUILD = debug
cxxflags.debug = \
 -Wall -std=c++11
linkflags.debug = \
 -lm
cxxflags.trace = \
 -Wall -std=c++11 -DTRACE_ON
linkflags.trace = \
 -lm
cxxflags.release = \
 -Wall -std=c++11 -DNDEBUG -O3
linkflags.release = \
 -lm
cxxflags.gprof = \
 -Wall -std=c++11 -DNDEBUG -O3 -g -pg
linkflags.gprof = \
 -lm -g -pg

# for g++ <= 4.6
cxxflags.debug0x = \
 -Wall -std=c++0x
linkflags.debug0x = \
 -lm
cxxflags.trace0x = \
 -Wall -std=c++0x -DTRACE_ON
linkflags.trace = \
 -lm
cxxflags.release0x = \
 -Wall -std=c++0x -DNDEBUG -O3
linkflags.release0x = \
 -lm
cxxflags.gprof0x = \
 -Wall -std=c++0x -DNDEBUG -O3 -g -pg 
linkflags.gprof0x = \
 -lm -g -pg
####

SOURCE_DIR = src
BUILD_DIR = build/$(BUILD)

BINARY = $(BUILD_DIR)/wasp
GCC = g++
CXX = $(GCC)
CXXFLAGS = $(cxxflags.$(BUILD))
LINK = $(GCC)
LINKFLAGS = $(linkflags.$(BUILD))

SRCS = $(shell find $(SOURCE_DIR) -name '*.cpp')

OBJS = $(patsubst $(SOURCE_DIR)%.cpp,$(BUILD_DIR)%.o, $(SRCS))
DEPS = $(patsubst $(SOURCE_DIR)%.cpp,$(BUILD_DIR)%.d, $(SRCS))

all: $(BINARY)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.d: $(SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MM -MT '$(@:.d=.o)' $< -MF $@
	
$(BINARY): $(OBJS) $(DEPS)
	$(LINK) $(LINKFLAGS) $(LIBS) $(OBJS) -o $(BINARY)

static: $(OBJS) $(DEPS)
	$(LINK) $(LINKFLAGS) $(LIBS) $(OBJS) -static -o $(BINARY)

run: $(BINARY)
	./$(BINARY)

########## Tests

TESTS_DIR = tests

TESTS_TESTER = $(TESTS_DIR)/pyregtest.py

TESTS_COMMAND_AllAnswerSets = "gringo | $(BINARY)"
TESTS_COMMAND_SatModel = "$(BINARY)"

TESTS_CHECKER_AllAnswerSets = $(TESTS_DIR)/allAnswerSets.checker.py
TESTS_CHECKER_SatModels = $(TESTS_DIR)/satModels.checker.py

TESTS_REPORT_text = $(TESTS_DIR)/text.report.py

TESTS_DIR_wasp1_AllAnswerSets = $(TESTS_DIR)/wasp1/AllAnswerSets
TESTS_SRC_wasp1_AllAnswerSets = $(shell find $(TESTS_DIR_wasp1_AllAnswerSets) -name '*.test.py')
TESTS_OUT_wasp1_AllAnswerSets = $(patsubst %.test.py,%.test.py.text, $(TESTS_SRC_wasp1_AllAnswerSets))

TESTS_DIR_sat_Models = $(TESTS_DIR)/sat/Models
TESTS_SRC_sat_Models = $(shell find $(TESTS_DIR_sat_Models) -name '*.test.py')
TESTS_OUT_sat_Models = $(patsubst %.test.py,%.test.py.text, $(TESTS_SRC_sat_Models))

TESTS_DIR_sat_Intensive = $(TESTS_DIR)/sat/Intensive
TESTS_SRC_sat_Intensive = $(shell find $(TESTS_DIR_sat_Intensive) -name '*.test.py')
TESTS_OUT_sat_Intensive = $(patsubst %.test.py,%.test.py.text, $(TESTS_SRC_sat_Intensive))

tests: tests_wasp1 tests_sat

tests_wasp1: tests_wasp1_AllAnswerSets

tests_wasp1_AllAnswerSets: $(TESTS_OUT_wasp1_AllAnswerSets)

$(TESTS_OUT_wasp1_AllAnswerSets):
	@$(TESTS_TESTER) $(TESTS_COMMAND_AllAnswerSets) $(patsubst %.test.py.text,%.test.py , $@) $(TESTS_CHECKER_AllAnswerSets) $(TESTS_REPORT_text)

tests_sat: tests_sat_models tests_sat_intensive

tests_sat_models: $(TESTS_OUT_sat_Models)

$(TESTS_OUT_sat_Models):
	@$(TESTS_TESTER) $(TESTS_COMMAND_SatModel) $(patsubst %.test.py.text,%.test.py , $@) $(TESTS_CHECKER_SatModels) $(TESTS_REPORT_text)

tests_sat_intensive: $(TESTS_OUT_sat_Intensive)

$(TESTS_OUT_sat_Intensive):
	@$(TESTS_TESTER) $(TESTS_COMMAND_SatModel) $(patsubst %.test.py.text,%.test.py , $@) $(TESTS_CHECKER_SatModels) $(TESTS_REPORT_text)

########## Clean

clean:
	rm -f $(OBJS)

distclean: clean
	rm -fr $(BUILD_DIR)

-include $(DEPS)