include $(srctree)/drivers/misc/mediatek/Makefile.custom

# $(error xxxxlogzhangxian)
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)/touchpanel/ektf2k_s5222/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/ektf2k_s5222/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/


ifeq ($(CONFIG_MTK_PLATFORM),"mt6572")
    ccflags-y += -DMT6572
endif

obj-y	+=  ektf2k_driver.o


