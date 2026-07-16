# SPDX-License-Identifier: GPL-2.0-only

SUBTARGET:=rt6855a
BOARDNAME:=RT6855A based boards
FEATURES+=ramdisk pci usb
CPU_TYPE:=34kc

# MIPS16 userspace crashes reproducibly on RT6855A/RT6856.
DISABLE_MIPS16:=1

DEFAULT_PACKAGES += wpad-mbedtls swconfig

define Target/Description
	Build firmware images for Ralink RT6855A based boards.
endef
