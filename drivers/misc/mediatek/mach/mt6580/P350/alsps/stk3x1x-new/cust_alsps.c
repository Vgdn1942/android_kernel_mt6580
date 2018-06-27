#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 2,
    .i2c_addr   = {0x48, 0x00, 0x00, 0x00},	/*STK3x1x*/
    .polling_mode_ps = 0, //0: interrupt mode
    .polling_mode_als = 1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .als_level  = {10, 20, 36, 59, 80, 120, 180, 260, 450, 845, 1136, 1545, 2364, 4655, 6982},
    .als_value  = {0, 10, 40, 65, 90, 145, 225, 300, 550, 930, 1250, 1700, 2600, 5120, 7680, 10240},
    .ps_threshold_high = 34,
    .ps_threshold_low = 28,
    .is_batch_supported_ps = 0,
    .is_batch_supported_als = 0,
};

struct alsps_hw* get_cust_alsps_hw(void)
{
    return (&cust_alsps_hw);
}

