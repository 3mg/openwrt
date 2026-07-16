# SPDX-License-Identifier: GPL-2.0-only

SUBTARGET:=rt6855a
BOARDNAME:=RT6855A based boards
FEATURES+=ramdisk pci usb
CPU_TYPE:=34kc

# MIPS16 userspace crashes reproducibly on RT6855A/RT6856.
DISABLE_MIPS16:=y

DEFAULT_PACKAGES += wpad-basic-mbedtls swconfig

define Target/Description
	Build firmware images for Ralink RT6855A based boards.
endef
