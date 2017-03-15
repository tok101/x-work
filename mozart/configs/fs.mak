define ROOTFS_cpio_CMDS
cd $(1) && find . | cpio --quiet -oH  newc > $(2)
endef

define ROOTFS_tarball_CMDS
cd $(1) && tar cf $(2) .
endef

define mul
$(shell expr $(1) \* `printf "%d" $(2)`)
endef


define ROOTFS_UPDATER_updatepkg_ramdisk_CMDS
cd $(1) && find . -not -wholename '.git' -not -wholename '.svn' -print | cpio -oH  newc | gzip -9 > $(2)
endef

ROOTFS_ext4_DEPENDENCIES := host-genext2fs host-e2fsprogs
define ROOTFS_UPDATER_ext4_CMDS
GEN=4 REV=1 $(TOPDIR)/configs/genext2fs.sh -d $(1) $(2)
endef
define ROOTFS_APP_ext4_CMDS
GEN=4 REV=1 $(TOPDIR)/configs/genext2fs.sh -d $(1) $(2)
endef

define UPDATE_UPDATER_ext4_CMDS
cp $(TARGET_DIR)/updater.ext4 $(UPDATEPKG_DIR)/temp/
endef
define UPDATE_APP_ext4_CMDS
cp $(TARGET_DIR)/appfs.ext4 $(UPDATEPKG_DIR)/temp/
endef


ROOTFS_cramfs_DEPENDENCIES := host-mkfs.cramfs
define ROOTFS_UPDATER_cramfs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.cramfs $(1) $(2)
endef
define ROOTFS_APP_cramfs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.cramfs $(1) $(2)
endef

define UPDATE_UPDATER_cramfs_CMDS
cp $(TARGET_DIR)/updater.cramfs $(UPDATEPKG_DIR)/temp/
endef
define UPDATE_APP_cramfs_CMDS
cp $(TARGET_DIR)/appfs.cramfs $(UPDATEPKG_DIR)/temp/
endef


ROOTFS_ubifs_DEPENDENCIES := host-mtd-utils mtd-utils flash_erase
define ROOTFS_UPDATER_ubifs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.ubifs -m $(FLASH_PAGE_SIZE) -e $(FLASH_LOGICAL_BLOCK_SIZE) \
	-c $(FLASH_UPDATER_BLOCK_COUNT) -r $(1) -o $(2); \
cp $(DEVICE_OTA_DIR)/updater.ini $(TOPDIR)/configs/updater-volume.ini; \
$(OUTPUT_DIR)/host/usr/bin/inirw \
	-f $(TOPDIR)/configs/updater-volume.ini \
	-w -s updater -k vol_size -v $(call mul,$(FLASH_UPDATER_BLOCK_COUNT),$(FLASH_ERASE_BLOCK_SIZE)); \
$(OUTPUT_DIR)/host/usr/sbin/ubinize \
	-o $(TARGET_DIR)/updater.img \
	-p $(FLASH_ERASE_BLOCK_SIZE) \
	-m $(FLASH_PAGE_SIZE) \
	-s $(FLASH_PAGE_SIZE) \
	-O $(FLASH_PAGE_SIZE) \
	$(TOPDIR)/configs/updater-volume.ini;\
	rm -f $(2)
endef

define ROOTFS_APP_ubifs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.ubifs -m $(FLASH_PAGE_SIZE) -e $(FLASH_LOGICAL_BLOCK_SIZE) \
	-c $(FLASH_APPFS_BLOCK_COUNT) -r $(1) -o $(2); \
cp $(DEVICE_OTA_DIR)/appfs.ini $(TOPDIR)/configs/appfs-volume.ini; \
$(OUTPUT_DIR)/host/usr/bin/inirw \
	-f $(TOPDIR)/configs/appfs-volume.ini \
	-w -s app -k vol_size -v $(call mul,$(FLASH_APPFS_BLOCK_COUNT),$(FLASH_ERASE_BLOCK_SIZE)); \
$(OUTPUT_DIR)/host/usr/sbin/ubinize \
	-o $(TARGET_DIR)/appfs.img \
	-p $(FLASH_ERASE_BLOCK_SIZE) \
	-m $(FLASH_PAGE_SIZE) \
	-s $(FLASH_PAGE_SIZE) \
	-O $(FLASH_PAGE_SIZE) \
	$(TOPDIR)/configs/appfs-volume.ini; \
	rm -f $(2)
endef

define UPDATE_UPDATER_ubifs_CMDS
cp $(TARGET_DIR)/updater.img $(UPDATEPKG_DIR)/temp/
endef
define UPDATE_APP_ubifs_CMDS
cp $(TARGET_DIR)/appfs.img $(UPDATEPKG_DIR)/temp/
endef

USRDATA_jffs2_DEPENDENCIES := host-mtd-utils flash_erase
define USRDATA_jffs2_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.jffs2 -m none -r $(1) -s$(FLASH_PAGE_SIZE) \
	-e$(FLASH_ERASE_BLOCK_SIZE) -p$(FLASH_USRDATA_PADSIZE) -o $(2)
endef
define USRDATA_ubifs_CMDS
$(OUTPUT_DIR)/host/usr/sbin/mkfs.ubifs -m $(FLASH_PAGE_SIZE) -e $(FLASH_LOGICAL_BLOCK_SIZE) \
	-c $(FLASH_USRDATA_BLOCK_COUNT) -r $(1) -o $(2); \
cp $(DEVICE_OTA_DIR)/usrdata.ini $(TOPDIR)/configs/usrdata-volume.ini; \
$(OUTPUT_DIR)/host/usr/bin/inirw \
	-f $(TOPDIR)/configs/usrdata-volume.ini \
	-w -s usrdata -k vol_size -v $(call mul,$(FLASH_USRDATA_BLOCK_COUNT),$(FLASH_ERASE_BLOCK_SIZE)); \
$(OUTPUT_DIR)/host/usr/sbin/ubinize \
	-o $(TARGET_DIR)/usrdata.img \
	-p $(FLASH_ERASE_BLOCK_SIZE) \
	-m $(FLASH_PAGE_SIZE) \
	-s $(FLASH_PAGE_SIZE) \
	-O $(FLASH_PAGE_SIZE) \
	$(TOPDIR)/configs/usrdata-volume.ini; \
	rm -f $(2)

endef
define USRDATA_ext4_CMDS
PATH=$(OUTPUT_DIR)/host/usr/bin:$(OUTPUT_DIR)/host/usr/sbin:$(PATH) \
	GEN=4 REV=1 $(TOPDIR)/configs/genext2fs.sh -b 4000 -d $(1) $(2)
endef

ifeq ("$(SUPPORT_FS)", "ramdisk")
$(eval $(call install_fs_rules,cpio,1))
endif
ifeq ("$(SUPPORT_FS)", "ext4")
TARGETS += $(ROOTFS_ext4_DEPENDENCIES)
$(eval $(call install_fs_rules,ext4,0))
endif
ifeq ("$(SUPPORT_FS)", "cramfs")
TARGETS += $(ROOTFS_cramfs_DEPENDENCIES)
$(eval $(call install_fs_rules,cramfs,0))
endif
ifeq ("$(SUPPORT_FS)", "ubifs")
TARGETS += $(ROOTFS_ubifs_DEPENDENCIES)
$(eval $(call install_fs_rules,ubifs,0))
endif

ifeq ("$(SUPPORT_USRDATA)", "jffs2")
TARGETS += $(USRDATA_jffs2_DEPENDENCIES)
$(eval $(call install_usrdata_rules,jffs2,0))
endif
ifeq ("$(SUPPORT_USRDATA)", "ubifs")
TARGETS += $(ROOTFS_ubifs_DEPENDENCIES)
$(eval $(call install_usrdata_rules,ubifs,0))
endif
ifeq ("$(SUPPORT_USRDATA)", "ext4")
TARGETS += $(ROOTFS_ext4_DEPENDENCIES)
$(eval $(call install_usrdata_rules,ext4,0))
endif
#$(eval $(call install_fs_rules,tarball,0))
