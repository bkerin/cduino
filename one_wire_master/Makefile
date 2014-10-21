include run_screen.mk

include generic.mk

# The one_wire_master.h interface requires that we define ONE_WIRE_MASTER_PIN
# at compile time: in this case we choose to use DIO_PIN_DIGITAL_2
CPPFLAGS += -DOWM_PIN=DIO_PIN_DIGITAL_2

# Exactly one of the following OWM_TEST_CONDITION_* macros must be defined.

#CPPFLAGS += -DOWM_TEST_CONDITION_SINGLE_SLAVE

# Uncomment these to test a multi-slave bus configuration.
CPPFLAGS += -DOWM_TEST_CONDITION_MULTIPLE_SLAVES
# FIXME: well once we're using term_io the code referred to by the
# rest of this commen block shouldn't have to be commented out. For the
# multiple-slave tests to work, you must determine your actual slave IDs
# beforehand and fill them in here.  There is some commented-out code in the
# OWM_TEST_CONDITION_SINGLE_SLAVE code in one_wire_master_test.c that can help
# with this.  Note the IDs must also be assigned correctly to first and second
# positions with respect to the discovery order of the one-wire bus search
# algorithm -- it might be easiest to try both orders :) The individual
# bytes of these IDs should be given in the order they occur in the ID,
# the test program takes care of swapping them to account for endianness.
CPPFLAGS += -DOWM_FIRST_SLAVE_ID=0x28542a8e060000b4
CPPFLAGS += -DOWM_SECOND_SLAVE_ID=0x289ae53005000015

# FIXME: USE OWM_ prefix for test conditions?  dio doesn't do it...

# Ensure that exactly one test condition is defined.
ifneq ($(words $(filter -DOWM_TEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one OWM_TEST_CONDITION_* must be defined)
endif
