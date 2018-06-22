#include <linux/types.h>
#include <cust_alsps.h>
#include <mach/mt_pm_ldo.h>

//#include <mach/mt6577_pm_ldo.h>
static struct alsps_hw cust_alsps_hw =
{
    .i2c_num    = 2,
    .polling_mode_ps =0,
    .polling_mode_als =1,       //Ivan Interrupt mode not support
    .power_id   = MT65XX_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x23, 0x0, 0x0, 0x00},
   // .als_level  = { 1,  32,  164, 252,  408, 1035,  65535,  65535,  65535, 65535, 65535, 65535, 65535, 65535, 65535},
  //  .als_value  = { 40,  50,  152,  300,  702,  1450,  10243,  10243, 10243, 10243, 10243, 10243, 10243, 10243, 10243},


 //   .als_level  = { 0,  640,  2320, 4000, 5680, 7360,  9040,  10720,  12400, 14080, 15760, 17440, 19120, 20486, 65535},
   // .als_value  = {  0,  320,  1160, 2000, 2840, 3680,  4520,  5360,  6200, 7040, 7880, 8720, 9560, 10243, 10243},
    .als_level  = { 8,  18,  41, 83, 789, 1805,  2881,  4191,  5672, 7227, 8824, 10569, 65535, 65535, 65535}, 
    .als_value  = {  0,  38,  95, 190, 1250, 1500,  1700,  1920,  2900, 5745, 8500, 10000, 10243, 10243, 10243},
    .ps_threshold = 2,  //3,
    .ps_threshold_high = 0xB3, // 300,
    .ps_threshold_low = 0x38, // 150,
    .als_threshold_high = 0xFFFF,
    .als_threshold_low = 0,

};
struct alsps_hw *get_cust_alsps_hw(void)
{
    return &cust_alsps_hw;
}

