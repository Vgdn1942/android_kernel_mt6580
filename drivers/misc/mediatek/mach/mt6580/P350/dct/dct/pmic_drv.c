/*
 * Generated by MTK SP DrvGen Version 03.13.6 for MT6580. Copyright MediaTek Inc. (C) 2013.
 * Sat Aug 18 00:00:38 2018
 * Do Not Modify the File.
 */



#include <linux/types.h>
#include <mach/mt_typedefs.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pm_ldo.h>
void pmu_drv_tool_customization_init(void)
{
    pmic_set_register_value(PMIC_RG_VGP3_EN,1);
    pmic_set_register_value(PMIC_RG_VCAMD_EN,1);
}


