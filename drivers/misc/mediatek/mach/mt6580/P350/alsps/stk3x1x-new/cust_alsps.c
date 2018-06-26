#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 2,
    .polling_mode_ps = 0, //0: interrupt mode
    .polling_mode_als = 1,
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x90, 0x00, 0x00, 0x00},	/*STK3x1x*/
	//STK add for CCI start 20130113
	.als_level  = {0,  66,  124, 250, 380, 508, 630, 820, 1003, 1255, 1884, 2510, 3664, 5520, 10240},   		 /* als_code */
	.als_value  = {0,  50,  100, 200, 300, 400, 500, 650, 800, 1000, 1500, 2000, 3000, 4000, 8000, 10240},    	 /* lux */
	//STK add for CCI end 20130113
	//CCI set config start
	//.state_val = 0x0,		/* disable all */
	//.psctrl_val = 0x33, 	/* ps_persistance=4, ps_gain=64X, PS_IT=0.391ms */
	//.alsctrl_val = 0x39,	/* als_persistance=1, als_gain=64X, ALS_IT=100ms */
	//.ledctrl_val = 0xFF,	/* 50mA IRDR, 64/64 LED duty */
	//.wait_val = 0x9,		/* 50 ms */
    //.ps_high_thd_val = 1500,
    //.ps_high_thd_val = 1500,
    //.ps_low_thd_val = 1300,
	//CCI set config end
    .ps_threshold_high = 53,
    .ps_threshold_low = 46,
};

struct alsps_hw* get_cust_alsps_hw(void)
{
    return (&cust_alsps_hw);
}

