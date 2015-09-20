ccflags-y += -I$(src)			# needed for trace events

<<<<<<< HEAD
ifeq ($(CONFIG_MTK_INTERNAL), y)
KBUILD_CFLAGS += -DMTK_INTERNAL_BUILD  # add for LMK
endif

=======
>>>>>>> v3.10.88
obj-$(CONFIG_ANDROID_BINDER_IPC)	+= binder.o
obj-$(CONFIG_ASHMEM)			+= ashmem.o
obj-$(CONFIG_ANDROID_LOGGER)		+= logger.o
obj-$(CONFIG_ANDROID_TIMED_OUTPUT)	+= timed_output.o
obj-$(CONFIG_ANDROID_TIMED_GPIO)	+= timed_gpio.o
<<<<<<< HEAD
obj-$(CONFIG_ANDROID_LOW_MEMORY_KILLER)	+= lowmemorykiller.o almk_drv.o
obj-$(CONFIG_ANDROID_SWITCH)		+= switch/

#obj-$(CONFIG_ANDROID_INTF_ALARM_DEV)	+= alarm.o
obj-$(CONFIG_ANDROID_INTF_ALARM_DEV)	+= alarm-dev.o

obj-$(CONFIG_PERSISTENT_TRACER)		+= trace_persistent.o

CFLAGS_REMOVE_trace_persistent.o = -pg
=======
obj-$(CONFIG_ANDROID_LOW_MEMORY_KILLER)	+= lowmemorykiller.o
obj-$(CONFIG_ANDROID_INTF_ALARM_DEV)	+= alarm-dev.o
obj-$(CONFIG_SYNC)			+= sync.o
obj-$(CONFIG_SW_SYNC)			+= sw_sync.o
>>>>>>> v3.10.88
