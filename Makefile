
obj-m += 3dspusbbus.o 3dspusbwlan.o 3dspusbbt.o

3dspusbbus-y := \
	bus/businit.o \
	bus/busioctl.o          

3dspusbwlan-y := \
	wlan/tdsp_memory.o \
	wlan/tdsp_event.o \
	wlan/tdsp_mutex.o \
	wlan/tdsp_timer.o \
	wlan/tdsp_tasklet.o \
	wlan/tdsp_usb.o \
	wlan/tdsp_bus.o \
	wlan/tdsp_file.o \
	wlan/lwlan_ioctl.o \
	wlan/lwlan_netdev.o \
	wlan/lwlan_entry.o \
	wlan/tdsp_debug.o \
	wlan/usbwlan_syscall.o \
	wlan/usbwlan_timer.o \
	wlan/usbwlan_mng.o \
	wlan/usbwlan_main.o \
	wlan/usbwlan_Task.o \
	wlan/usbwlan_rx.o \
	wlan/usbwlan_tx.o \
	wlan/usbwlan_proto.o \
	wlan/usbwlan_rate.o \
	wlan/usbwlan_interrupt.o \
	wlan/usbwlan_vendor.o \
	wlan/usbwlan_UsbDev.o \
	wlan/usbwlan_ioctl.o

3dspusbbt-y :=  \
	bluetooth/bt_main.o \
	bluetooth/tdsp_bus.o \
	bluetooth/bt_keygen.o \
	bluetooth/g711.o \
	bluetooth/cvsd.o \
	bluetooth/bt_cancel.o \
	bluetooth/bt_flush.o \
	bluetooth/bt_frag.o \
	bluetooth/bt_hal.o \
	bluetooth/bt_hci.o \
	bluetooth/sched.o \
	bluetooth/bt_lmp.o \
	bluetooth/bt_pnp.o \
	bluetooth/bt_task.o \
	bluetooth/bt_power.o \
	bluetooth/bt_sendrecv.o \
	bluetooth/btdownload.o \
	bluetooth/bt_usb_vendorcom.o \
	bluetooth/btusb.o \
	bluetooth/afhclassify.o

#DEFS += -DBT_TESTDRIVER
#DEFS += -DBT_PETER_TEST_WRITE
DEFS += -DBT_FRAG_USED
DEFS += -DBT_PCM_COMPRESS
DEFS += -DBT_AUTO_SEL_PACKET
DEFS += -DBT_SEND_MULTISCO_ONCE
#DEFS += -DBT_DISCARD_ACL_IN_SCO_CONNECTED
DEFS += -DBT_WRITE_COMMAND_INDICATOR_ONLY_LINK_EMPTY
#DEFS += -DBT_INTERFACE_BUS_DRIVER
#DEFS += -DBT_ACCESS_SCRATCH_REG_SAFE
DEFS += -DBT_AFH_ADJUST_MAP_SUPPORT
#DEFS += -DBT_ACCESS_SCRATCH_BY_DMA0
#DEFS += -DBT_RECORD_VOX_TO_FILE
#DEFS += -DBT_USE_INTERNAL_TEST_BDADDR
#DEFS += -DBT_FHS_PACKET_TAKE_WLAN_SPACE
DEFS += -DBT_SCHEDULER_SUPPORT
#DEFS += -DBT_USE_ONE_SLOT_FOR_ACL
#DEFS += -DBT_AUX1_INSTEAD_DH1
#DEFS += -DBT_LOOPBACK_SUPPORT
DEFS += -DBT_RSSI_SUPPORT
DEFS += -DBT_POWER_CONTROL_SUPPORT
#DEFS += -DBT_TESTMODE_SUPPORT
DEFS += -DBT_ROLESWITCH_SUPPORT
DEFS += -DBT_AFH_SUPPORT
DEFS += -DBT_USING_ADD_SCO_COMMAND
#DEFS += -DBT_DISCARD_SPILTH_SCO_IN_BYTES
DEFS += -DBT_USE_NEW_ARCHITECTURE
#DEFS += -DBT_SERIALIZE_DRIVER
#DEFS += -DBT_AUTO_SNIFF
DEFS += -DBT_SNIFF_SUPPORT
DEFS += -DBT_ENHANCED_RATE
DEFS += -DBT_ROLESWITCH_UNALLOWED_WHEN_MULTI_SLAVE
DEFS += -DBT_SUPPROT_RX_FRAG
DEFS += -DBT_MATCH_CLASS_OF_DEVICE
DEFS += -DDSP_REG_WRITE_API
DEFS += -DBT_ENCRYPTION_SUPPORT
#DEFS += -DSERIALIZE_ALL_THREADS
DEFS += -DBT_EXT_SCO_LINK_SUPPORT
#DEFS += -DBT_LOAD_EEPROM_SUPPORT
#DEFS += -DBT_3DSP_HI_SPEED
DEFS += -DLMP_DEBUG_DUMP
DEFS += -DBT_3DSP_FLUSH_SUPPORT
#DEFS += -DWB_PROXY_FLAG
DEFS += -DHOT_KEY_ENABLE
#DEFS += -DBT_USE_ANTENNA_0
DEFS += -DBT_DEVICE_CLASS_TYPE_2
DEFS += -DBT_DOWNLOAD_BIN_WITH_FILE_MODE
DEFS += -DBT_FORCE_MASTER_SNIFF_ONE_SLOT
DEFS += -DBT_EXIT_SNIFF_WHEN_EXIST_SLAVE
DEFS += -DBT_SNIFF_ONLY_ONE_CONNECTION

#DEFS += -DBT_INQUIRY_RESULT_WITH_RSSI_SUPPORT
#DEFS += -DBT_2_1_SPEC_SUPPORT
#DEFS += -DBT_ALLOCATE_FRAG_FOR_EACH_CONNECTION
#DEFS += -DBT_CLOSE_SCO_HV2_TYPE
#DEFS += -DBT_CLOSE_SCO_HV3_TYPE
#DEFS += -DBT_INTERNAL_QOS_SETUP
#DEFS += -DBT_POWER_CONTROL_2829
#DEFS += -DBT_SET_LINK_KEY_USING_APP

EXTRA_CFLAGS = \
	$(DEFS) \
	-Wno-unused-variable \
	-Wno-parentheses \
	-Wno-format \
	-Wnonnull \
	-Wstrict-prototypes

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

ccflags-y := -D__CHECK_ENDIAN__

all: module

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -f */*.o */.*.o.d */.*.o.cmd
	rm -f *.o .*.cmd *.ko *.mod.c
	rm -f Module.symvers modules.order
	rm -rf .tmp_versions $(hostprogs-y)
