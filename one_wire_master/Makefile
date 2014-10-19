include generic.mk

# The one_wire_master.h interface requires that we define ONE_WIRE_MASTER_PIN
# at compile time: in this case we choose to use DIO_PIN_DIGITAL_2
# FIXME: should this be called OWM_PIN?
CPPFLAGS += -DONE_WIRE_MASTER_PIN=DIO_PIN_DIGITAL_2

# Uncomment this to test a multi-slave bus configuration.
#CPPFLAGS += -DOWM_TEST_CONDITION_MULTIPLE_SLAVES
