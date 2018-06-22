#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "mach/mtk_thermal_monitor.h"
#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"
#include <cust_pmic.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>
#include <mach/upmu_sw.h>
//#include <mach/mt_pmic_wrap.h>


extern struct proc_dir_entry * mtk_thermal_get_proc_drv_therm_dir_entry(void);

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};

static unsigned int cl_dev_sysrst_state = 0;
static struct thermal_zone_device *thz_dev;

static struct thermal_cooling_device *cl_dev_sysrst;
static int mtktspmic_debug_log = 0;
static int kernelmode = 0;

static int g_THERMAL_TRIP[10] = {0,0,0,0,0,0,0,0,0,0};
static int num_trip=0;
static char g_bind0[20]={0};
static char g_bind1[20]={0};
static char g_bind2[20]={0};
static char g_bind3[20]={0};
static char g_bind4[20]={0};
static char g_bind5[20]={0};
static char g_bind6[20]={0};
static char g_bind7[20]={0};
static char g_bind8[20]={0};
static char g_bind9[20]={0};

/**
 * If curr_temp >= polling_trip_temp1, use interval
 * else if cur_temp >= polling_trip_temp2 && curr_temp < polling_trip_temp1, use interval*polling_factor1
 * else, use interval*polling_factor2
 */
static int polling_trip_temp1 = 40000;
static int polling_trip_temp2 = 20000;
static int polling_factor1 = 5000;
static int polling_factor2 = 10000;

#define mtktspmic_TEMP_CRIT 150000 /* 150.000 degree Celsius */

#define mtktspmic_err(fmt, args...)   \
do {									\
    pr_err("Power/PMIC_Thermal" fmt, ##args); \
} while(0)
#define mtktspmic_info(fmt, args...)   \
do {									\
    pr_notice("Power/PMIC_Thermal" fmt, ##args); \
} while(0)
#define mtktspmic_dbg(fmt, args...)   \
do {									\
    if (mtktspmic_debug_log) {				\
        pr_notice("Power/PMIC_Thermal" fmt, ##args); \
    }								   \
} while(0)

/* Cali */
static kal_int32 g_o_vts = 0;
static kal_int32 g_degc_cali = 0;
static kal_int32 g_adc_cali_en = 0;
static kal_int32 g_o_slope = 0;
static kal_int32 g_o_slope_sign = 0;
static kal_int32 g_id = 0;
static kal_int32 g_slope1= 1;
static kal_int32 g_slope2= 1;
static kal_int32 g_intercept;

#define y_pmic_repeat_times	1


void mtktspmic_read_efuse(void)
{
	U32 reg_val = 0;

	U32 efuse_val0, efuse_val1;

	mtktspmic_info("[mtktspmic_read_efuse] start\n");

	//1. enable RD trig bit
	pmic_config_interface(0x0608, 0x1, 0x1, 0);
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);

	//2. read thermal efuse
	//0x063A    160  175
	//0x063C    176  191
	//Thermal data from 161 to 184
	pmic_read_interface(0x063A, &efuse_val0, 0xFFFF, 0);
	pmic_read_interface(0x063C, &efuse_val1, 0xFFFF, 0);
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);

	//3. polling RD busy bit
/*	do {
		pmic_read_interface(0x60C, &reg_val, 0x1, 0);
		mtktspmic_dbg("[mtktspmic_read_efuse] polling Reg[0x60C][0]=0x%x\n", reg_val);
	} while(reg_val == 1);
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
*/
	mtktspmic_info("[mtktspmic_read_efuse] Reg(0x63a) = 0x%x, Reg(0x63c) = 0x%x\n", efuse_val0, efuse_val1);

	//4. parse thermal parameters
	g_adc_cali_en = (efuse_val0 >> 1) & 0x0001;
	g_degc_cali = (efuse_val0 >> 2) & 0x003f;
	g_o_vts = ((efuse_val1 & 0x001F) << 8) + ((efuse_val0 >> 8) & 0x00FF);
	g_o_slope_sign = (efuse_val1 >> 5) & 0x0001;
	g_o_slope = (((efuse_val1 >> 11) & 0x0007) << 3) + ((efuse_val1 >> 6) & 0x007);
	g_id = (efuse_val1 >> 14) & 0x0001;

	mtktspmic_info("[mtktspmic_read_efuse] done\n");
}

static void pmic_cali_prepare(void)
{
    mtktspmic_read_efuse();

	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
    if(g_id==0)
        g_o_slope=0;

    if(g_adc_cali_en == 0) { //no calibration
        g_o_vts = 3698;
        g_degc_cali = 50;
        g_o_slope = 0;
        g_o_slope_sign = 0;
    }

    mtktspmic_info("g_adc_cali_en = 0x%x\n", g_adc_cali_en);
    mtktspmic_info("g_degc_cali = 0x%x\n", g_degc_cali);
    mtktspmic_info("g_o_vts = 0x%x\n", g_o_vts);
    mtktspmic_info("g_o_slope_sign = 0x%x\n", g_o_slope_sign);
    mtktspmic_info("g_o_slope = 0x%x\n", g_o_slope);
    mtktspmic_info("g_id = 0x%x\n", g_id);
}

static void pmic_cali_prepare2(void)
{
    kal_int32 vbe_t;

    g_slope1 = (100 * 1000);	//1000 is for 0.001 degree

    if(g_o_slope_sign==0)
        g_slope2 = -(171+g_o_slope);
    else
        g_slope2 = -(171-g_o_slope);

    vbe_t= (-1) * (((g_o_vts + 9102)*1800)/32768) * 1000;

    if(g_o_slope_sign==0)
        g_intercept = (vbe_t * 100) / (-(171+g_o_slope)); 	//0.001 degree
    else
        g_intercept = (vbe_t * 100) / (-(171-g_o_slope));  //0.001 degree

    g_intercept = g_intercept + (g_degc_cali*(1000/2)); // 1000 is for 0.1 degree

    mtktspmic_info("[Thermal calibration] SLOPE1=%d SLOPE2=%d INTERCEPT=%d, Vbe = %d\n",
		g_slope1, g_slope2, g_intercept,vbe_t);
}

static kal_int32 pmic_raw_to_temp(kal_uint32 ret)
{
    kal_int32 y_curr = ret;

    kal_int32 t_current;
    t_current = g_intercept + ((g_slope1 * y_curr) / (g_slope2));
    mtktspmic_dbg("[pmic_raw_to_temp] t_current=%d\n",t_current);

    return t_current;
}


static DEFINE_MUTEX(TSPMIC_lock);
static int pre_temp1=0, PMIC_counter=0;
static int mtktspmic_get_hw_temp(void)
{
    int temp=0, temp1=0;

    mutex_lock(&TSPMIC_lock);

    // TODO: check this
    temp = PMIC_IMM_GetOneChannelValue(3 , y_pmic_repeat_times , 2);
    temp1 = pmic_raw_to_temp(temp);

    mtktspmic_dbg("[mtktspmic_get_hw_temp] Raw=%d, T=%d\n",temp, temp1);

    if((temp1 > 100000) || (temp1 < -30000))
        mtktspmic_info("[mtktspmic_get_hw_temp] raw=%d, PMIC T=%d", temp, temp1);

    if((temp1 > 150000) || (temp1 < -50000)) {
        mtktspmic_err("[mtktspmic_get_hw_temp] temp(%d) too high, drop this data!\n", temp1);
        temp1 = pre_temp1;
    }
    else if((PMIC_counter != 0) && (((pre_temp1 - temp1) > 30000) || ((temp1 - pre_temp1) > 30000)) ) {
        mtktspmic_err("[mtktspmic_get_hw_temp] temp diff too large, drop this data\n");
        temp1 = pre_temp1;
    }
    else {
        //update previous temp
        pre_temp1 = temp1;
        mtktspmic_dbg("[mtktspmic_get_hw_temp] pre_temp1=%d\n", pre_temp1);

        if(PMIC_counter==0)
            PMIC_counter++;
    }

    mutex_unlock(&TSPMIC_lock);

    return temp1;
}

static int mtktspmic_get_temp(struct thermal_zone_device *thermal,
				              unsigned long *t)
{
    *t = mtktspmic_get_hw_temp();

    if ((int) *t >= polling_trip_temp1)
        thermal->polling_delay = interval*1000;
    else if ((int) *t < polling_trip_temp2)
        thermal->polling_delay = interval * polling_factor2;
    else
        thermal->polling_delay = interval * polling_factor1;

	return 0;
}

static int mtktspmic_bind(struct thermal_zone_device *thermal,
			struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktspmic_dbg("[mtktspmic_bind] %s\n", cdev->type);
	}
	else
	{
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		mtktspmic_err("[mtktspmic_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
		mtktspmic_dbg("[mtktspmic_bind] binding OK, %d\n", table_val);
	}

	return 0;
}

static int mtktspmic_unbind(struct thermal_zone_device *thermal,
			  struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktspmic_dbg("[mtktspmic_unbind] %s\n", cdev->type);
	}
	else
		return 0;

	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktspmic_err("[mtktspmic_unbind] error unbinding cooling dev\n");
		return -EINVAL;
	} else {
		mtktspmic_dbg("[mtktspmic_unbind] unbinding OK\n");
	}

	return 0;
}

static int mtktspmic_get_mode(struct thermal_zone_device *thermal,
				enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
				 : THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtktspmic_set_mode(struct thermal_zone_device *thermal,
				enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtktspmic_get_trip_type(struct thermal_zone_device *thermal, int trip,
				 enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktspmic_get_trip_temp(struct thermal_zone_device *thermal, int trip,
				 unsigned long *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int mtktspmic_get_crit_temp(struct thermal_zone_device *thermal,
				 unsigned long *temperature)
{
	*temperature = mtktspmic_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktspmic_dev_ops = {
	.bind = mtktspmic_bind,
	.unbind = mtktspmic_unbind,
	.get_temp = mtktspmic_get_temp,
	.get_mode = mtktspmic_get_mode,
	.set_mode = mtktspmic_set_mode,
	.get_trip_type = mtktspmic_get_trip_type,
	.get_trip_temp = mtktspmic_get_trip_temp,
	.get_crit_temp = mtktspmic_get_crit_temp,
};

static int tspmic_sysrst_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	*state = 1;
	return 0;
}
static int tspmic_sysrst_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	*state = cl_dev_sysrst_state;
	return 0;
}
static int tspmic_sysrst_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	cl_dev_sysrst_state = state;
	if(cl_dev_sysrst_state == 1)
	{
		mtktspmic_err("Power/PMIC_Thermal: reset, reset, reset!!!");
		mtktspmic_err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		mtktspmic_err("*****************************************");
		mtktspmic_err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

//		BUG();
		*(unsigned int*) 0x0 = 0xdead; // To trigger data abort to reset the system for thermal protection.
		//arch_reset(0,NULL);
	}
	return 0;
}

static struct thermal_cooling_device_ops mtktspmic_cooling_sysrst_ops = {
	.get_max_state = tspmic_sysrst_get_max_state,
	.get_cur_state = tspmic_sysrst_get_cur_state,
	.set_cur_state = tspmic_sysrst_set_cur_state,
};


static int mtktspmic_read(struct seq_file *m, void *v)
{
//    U16 pmic_data=0;
    seq_printf(m, "[ mtktspmic_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
	trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n\
	g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\n\
	g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n\
	cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n\
	cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
								trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
								trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],
								g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
								g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9],
								g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9,
								interval*1000);

//mtktspmic_read_efuse();
//	pmic_data = ts_pmic_read(0x0E9E);
//    seq_printf(m,"PMIC_Thermal: ts_pmic_read(0x0E9E) = 0x%x\n", pmic_data);

    seq_printf(m,"PMIC_Thermal: g_o_vts        = 0x%x\n", g_o_vts);
    seq_printf(m,"PMIC_Thermal: g_degc_cali    = 0x%x\n", g_degc_cali);
    seq_printf(m,"PMIC_Thermal: g_adc_cali_en  = 0x%x\n", g_adc_cali_en);
    seq_printf(m,"PMIC_Thermal: g_o_slope      = 0x%x\n", g_o_slope);
    seq_printf(m,"PMIC_Thermal: g_o_slope_sign = 0x%x\n", g_o_slope_sign);
    seq_printf(m,"PMIC_Thermal: g_id           = 0x%x\n", g_id);

    return 0;
}

int mtktspmic_register_thermal(void);
void mtktspmic_unregister_thermal(void);

static ssize_t mtktspmic_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	int len=0,time_msec=0;
	int trip[10]={0};
	int t_type[10]={0};
	int i;
	char bind0[20],bind1[20],bind2[20],bind3[20],bind4[20];
	char bind5[20],bind6[20],bind7[20],bind8[20],bind9[20];
	char desc[512];


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d",
							&num_trip, &trip[0],&t_type[0],bind0, &trip[1],&t_type[1],bind1,
												 &trip[2],&t_type[2],bind2, &trip[3],&t_type[3],bind3,
												 &trip[4],&t_type[4],bind4, &trip[5],&t_type[5],bind5,
											   &trip[6],&t_type[6],bind6, &trip[7],&t_type[7],bind7,
												 &trip[8],&t_type[8],bind8, &trip[9],&t_type[9],bind9,
												 &time_msec) == 32)
	{
		mtktspmic_dbg("[mtktspmic_write] mtktspmic_unregister_thermal\n");
		mtktspmic_unregister_thermal();

		for(i=0; i<num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];

		g_bind0[0]=g_bind1[0]=g_bind2[0]=g_bind3[0]=g_bind4[0]=g_bind5[0]=g_bind6[0]=g_bind7[0]=g_bind8[0]=g_bind9[0]='\0';

		for(i=0; i<20; i++)
		{
			g_bind0[i]=bind0[i];
			g_bind1[i]=bind1[i];
			g_bind2[i]=bind2[i];
			g_bind3[i]=bind3[i];
			g_bind4[i]=bind4[i];
			g_bind5[i]=bind5[i];
			g_bind6[i]=bind6[i];
			g_bind7[i]=bind7[i];
			g_bind8[i]=bind8[i];
			g_bind9[i]=bind9[i];
		}

		mtktspmic_dbg("[mtktspmic_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
													g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
													g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
		mtktspmic_dbg("[mtktspmic_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
													g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

		for(i=0; i<num_trip; i++)
		{
			trip_temp[i]=trip[i];
		}

		interval=time_msec / 1000;

		mtktspmic_dbg("[mtktspmic_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d\n",
						trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
						trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000);

		mtktspmic_dbg("[mtktspmic_write] mtktspmic_register_thermal\n");
		mtktspmic_register_thermal();

		return count;
	}
	else
	{
		mtktspmic_err("[mtktspmic_write] bad argument\n");
	}

	return -EINVAL;
}

void mtkts_pmic_cancel_thermal_timer(void)
{
	//cancel timer
	//printk("mtkts_pmic_cancel_thermal_timer \n");

	// stop thermal framework polling when entering deep idle
	if (thz_dev)
	    cancel_delayed_work(&(thz_dev->poll_queue));
}


void mtkts_pmic_start_thermal_timer(void)
{
	//printk("mtkts_pmic_start_thermal_timer \n");
    // resume thermal framework polling when leaving deep idle
    if (thz_dev != NULL && interval != 0)
	    mod_delayed_work(system_freezable_wq, &(thz_dev->poll_queue), round_jiffies(msecs_to_jiffies(1000))); // 60ms
}


int mtktspmic_register_cooler(void)
{
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktspmic-sysrst", NULL,
					   &mtktspmic_cooling_sysrst_ops);
   	return 0;
}

int mtktspmic_register_thermal(void)
{
	mtktspmic_dbg("[mtktspmic_register_thermal] \n");

	/* trips : trip 0~2 */
	thz_dev = mtk_thermal_zone_device_register("mtktspmic", num_trip, NULL,
					  &mtktspmic_dev_ops, 0, 0, 0, interval*1000);

	return 0;
}

void mtktspmic_unregister_cooler(void)
{
	if (cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}

void mtktspmic_unregister_thermal(void)
{
	mtktspmic_dbg("[mtktspmic_unregister_thermal] \n");

	if (thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int mtktspmic_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtktspmic_read, NULL);
}

static const struct file_operations mtktspmic_fops = {
	.owner = THIS_MODULE,
	.open = mtktspmic_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtktspmic_write,
	.release = single_release,
};


static int mtktspmic_read_log(struct seq_file *m, void *v)
{

	seq_printf(m, "mtktspmic_read_log = %d\n", mtktspmic_debug_log);


	return 0;
}

static ssize_t mtktspmic_write_log(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int log_switch;
	int len = 0;


	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d", &log_switch) == 1)
	{
		mtktspmic_debug_log = log_switch;

		return count;
	}
	else
	{
		mtktspmic_err("mtktspmic_write_log bad argument\n");
	}
	return -EINVAL;

}


static int mtktspmic_open_log(struct inode *inode, struct file *file)
{
    return single_open(file, mtktspmic_read_log, NULL);
}
static const struct file_operations mtktspmic_log_fops = {
    .owner = THIS_MODULE,
    .open = mtktspmic_open_log,
    .read = seq_read,
    .llseek = seq_lseek,
    .write = mtktspmic_write_log,
    .release = single_release,
};

static int __init mtktspmic_init(void)
{
	int err = 0;

	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktspmic_dir = NULL;

	mtktspmic_info("[mtktspmic_init] \n");

/*
	bit4	RG_VBUF_EN	1: turn on Vbuf.
						0: turn off Vbuf.
	bit2	RG_VBUF_BYP	1: Bypass Vbuf.
						0: turn on Vbuf.

	RG_VBUF_EN = 1 / RG_VBUF_BYP = 0

	pmic_data = ts_pmic_read(0x0E9E);
    if((pmic_data>>4&0x1)!=1 || (pmic_data>>2&0x1)!=0)
        mtktspmic_err("[mtktspmic_init]: Warrning !!! Need to checking this !!!!!\n");
*/
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
	pmic_cali_prepare();
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
	pmic_cali_prepare2();
	mtktspmic_info("PMIC_Debug %d\n",__LINE__);

	err = mtktspmic_register_cooler();
	if(err)
		return err;
	err = mtktspmic_register_thermal();
	if (err)
		goto err_unreg;

	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
	mtktspmic_dir = mtk_thermal_get_proc_drv_therm_dir_entry();
	if (!mtktspmic_dir)
	{
		mtktspmic_err("[%s]: mkdir /proc/driver/thermal failed\n", __func__);
	}
	else
	{
        entry = proc_create("tzpmic", S_IRUGO | S_IWUSR | S_IWGRP, mtktspmic_dir, &mtktspmic_fops);
        if (entry) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
            proc_set_user(entry, 0, 1000);
#else
            entry->gid = 1000;
#endif
	    }

		entry = proc_create("tzpmic_log", S_IRUGO | S_IWUSR, mtktspmic_dir, &mtktspmic_log_fops);
	}

	mtktspmic_info("PMIC_Debug %d\n",__LINE__);
	return 0;

err_unreg:
		mtktspmic_unregister_cooler();
		return err;
}

static void __exit mtktspmic_exit(void)
{
	mtktspmic_info("[mtktspmic_exit] \n");
	mtktspmic_unregister_thermal();
	mtktspmic_unregister_cooler();
}

module_init(mtktspmic_init);
module_exit(mtktspmic_exit);
