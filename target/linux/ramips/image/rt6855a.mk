# SPDX-License-Identifier: GPL-2.0-only

define Build/append-rootfs-data-marker
	printf '\336\255\300\336' >> $@
endef

define Device/zyxel_keenetic-giga-ii
  SOC := rt6855a
  IMAGE_SIZE := 14976k
  BLOCKSIZE := 4k
  DEVICE_VENDOR := ZyXEL
  DEVICE_MODEL := Keenetic Giga II
  IMAGES += factory.bin
  IMAGE/factory.bin := $$(sysupgrade_bin) | pad-to 64k | \
	append-rootfs-data-marker | pad-to 64k | check-size | \
	zyimage -d 0x6215 -v "ZyXEL Keenetic Giga II"
  DEVICE_PACKAGES := kmod-rt2800-pci kmod-switch-rtl8367b \
	kmod-gpio-rt6856 \
	kmod-usb2 kmod-usb-ohci kmod-usb-storage \
	rt2800-pci-firmware
endef
TARGET_DEVICES += zyxel_keenetic-giga-ii
