CXX = g++
CXXFLAGS = -O3 -fomit-frame-pointer -Wall -Wextra
AR = ar
ARFLAGS = rvs

TARGET = librs.a
SRCS = crc.cpp rs_factory.cpp rs_tables.cpp rs_decoder_general.cpp \
       rs_flipped_encoder_general.cpp rs_standard_encoder_general.cpp \
       rs_decoder_base.cpp rs_decoder_t1.cpp rs_flipped_encoder_t1.cpp \
       rs_flipped_encoder_t2.cpp rs_flipped_encoder_t4.cpp \
       rs_flipped_encoder_t8.cpp rs_flipped_encoder_t14.cpp \
       rs_flipped_encoder_t16.cpp rs_flipped_encoder_t32.cpp \
       rs_flipped_encoder_t64.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

# Dependencies
rs_factory.o: rs_factory.h rs_tables.h rs_codec.h
rs_tables.o: rs_common.h rs_tables.h
rs_decoder_general.o: rs_decoder_base.h rs_decoder_general.h rs_common.h rs_tables.h
rs_flipped_encoder_general.o: rs_encoder_base.h rs_flipped_encoder_general.h rs_common.h rs_tables.h
rs_standard_encoder_general.o: rs_encoder_base.h rs_standard_encoder_general.h rs_common.h rs_tables.h
rs_decoder_base.o: rs_decoder_base.h rs_common.h rs_tables.h
rs_decoder_t1.o: rs_decoder_t1.h rs_common.h rs_tables.h
rs_flipped_encoder_t1.o: rs_flipped_encoder_t1.h rs_common.h rs_tables.h
rs_flipped_encoder_t2.o: rs_flipped_encoder_t2.h rs_common.h rs_tables.h
rs_flipped_encoder_t4.o: rs_flipped_encoder_t4.h rs_common.h rs_tables.h
rs_flipped_encoder_t8.o: rs_flipped_encoder_t8.h rs_common.h rs_tables.h
rs_flipped_encoder_t14.o: rs_flipped_encoder_t14.h rs_common.h rs_tables.h
rs_flipped_encoder_t16.o: rs_flipped_encoder_t16.h rs_common.h rs_tables.h
rs_flipped_encoder_t32.o: rs_flipped_encoder_t32.h rs_common.h rs_tables.h
rs_flipped_encoder_t64.o: rs_flipped_encoder_t64.h rs_common.h rs_tables.h
crc.o: crc.h projdefs.h
