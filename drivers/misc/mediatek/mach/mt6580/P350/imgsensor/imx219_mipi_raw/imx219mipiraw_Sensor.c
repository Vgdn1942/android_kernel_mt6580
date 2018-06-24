
/*****************************************************************************

 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>


#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "imx219mipiraw_Sensor.h"
#include "imx219mipiraw_Camera_Sensor_para.h"
#include "imx219mipiraw_CameraCustomized.h"

kal_bool  IMX219MIPI_MPEG4_encode_mode = KAL_FALSE;
kal_bool IMX219MIPI_Auto_Flicker_mode = KAL_FALSE;

#if 1 //defined(SLT_DRV_W5520_CONFIG)
static void VideoFullSizeSetting(void);
int video_flag = -1;
#endif

kal_uint8 IMX219MIPI_sensor_write_I2C_address = IMX219MIPI_WRITE_ID;
kal_uint8 IMX219MIPI_sensor_read_I2C_address = IMX219MIPI_READ_ID;



#define kal_uint16 	unsigned short


UINT16  IMX219VIDEO_MODE_TARGET_FPS = 30;

static struct IMX219MIPI_sensor_STRUCT IMX219MIPI_sensor={
	IMX219MIPI_WRITE_ID,//i2c_write_id
	IMX219MIPI_READ_ID,//i2c_read_id
	KAL_TRUE,//first_init
	KAL_FALSE,//fix_video_fps
	KAL_TRUE,//pv_mode
	KAL_FALSE,//video_mode
	KAL_FALSE,//capture_mode
	KAL_FALSE,//night_mode
	KAL_FALSE,//mirror_flip
	IMX219MIPI_PREVIEW_CLK,//pv_pclk
	IMX219MIPI_VIDEO_CLK,//video_pclk
	IMX219MIPI_CAPTURE_CLK,//capture_pclk
	0,//pv_shutter
	0,//video_shutter
	0,//cp_shutter
	IMX219MIPI_BASE_GAIN,//pv_gain
	IMX219MIPI_BASE_GAIN,//video_gain
	IMX219MIPI_BASE_GAIN,//cp_gain
	IMX219MIPI_PV_LINE_LENGTH_PIXELS,//pv_line_length
	IMX219MIPI_PV_FRAME_LENGTH_LINES,//pv_frame_length
	IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS,//video_line_length
	IMX219MIPI_VIDEO_FRAME_LENGTH_LINES,//video_frame_length
	IMX219MIPI_FULL_LINE_LENGTH_PIXELS,//cp_line_length
	IMX219MIPI_FULL_FRAME_LENGTH_LINES,//cp_frame_length
	0,//pv_dummy_pixels
	0,//pv_dummy_lines
	0,//video_dummy_pixels
	0,//video_dummy_lines
	0,//cp_dummy_pixels
	0,//cp_dummy_lines
	IMX219MIPI_VIDEO_MAX_FRAMERATE//video_current_frame_rate
};
static MSDK_SCENARIO_ID_ENUM CurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;	
kal_uint16	IMX219MIPI_sensor_gain_base=0x0;
/* MAX/MIN Explosure Lines Used By AE Algorithm */
kal_uint16 IMX219MIPI_MAX_EXPOSURE_LINES = IMX219MIPI_PV_FRAME_LENGTH_LINES-5;//650;
kal_uint8  IMX219MIPI_MIN_EXPOSURE_LINES = 2;
kal_uint8  test_pattern_flag=0;
#define IMX219_TEST_PATTERN_CHECKSUM (0x9e08861c)

kal_uint32 IMX219MIPI_isp_master_clock;
static DEFINE_SPINLOCK(IMX219_drv_lock);

#ifdef DEBUG_IMX219
#define SENSORDB(fmt, arg...) printk( "[IMX219MIPIRaw] "  fmt, ##arg)
#else
#define SENSORDB(fmt, arg...)
#endif
#define RETAILMSG(x,...)
#define TEXT
UINT8 IMX219MIPIPixelClockDivider=0;
kal_uint16 IMX219MIPI_sensor_id=0;
MSDK_SENSOR_CONFIG_STRUCT IMX219MIPISensorConfigData;
kal_uint32 IMX219MIPI_FAC_SENSOR_REG;
kal_uint16 IMX219MIPI_sensor_flip_value; 
#define IMX219MIPI_MaxGainIndex (97)
kal_uint16 IMX219MIPI_sensorGainMapping[IMX219MIPI_MaxGainIndex][2] ={
{ 64 ,0  },   
{ 68 ,12 },   
{ 71 ,23 },   
{ 74 ,33 },   
{ 77 ,42 },   
{ 81 ,52 },   
{ 84 ,59 },   
{ 87 ,66 },   
{ 90 ,73 },   
{ 93 ,79 },   
{ 96 ,85 },   
{ 100,91 },   
{ 103,96 },   
{ 106,101},   
{ 109,105},   
{ 113,110},   
{ 116,114},   
{ 120,118},   
{ 122,121},   
{ 125,125},   
{ 128,128},   
{ 132,131},   
{ 135,134},   
{ 138,137},
{ 141,139},
{ 144,142},   
{ 148,145},   
{ 151,147},   
{ 153,149}, 
{ 157,151},
{ 160,153},      
{ 164,156},   
{ 168,158},   
{ 169,159},   
{ 173,161},   
{ 176,163},   
{ 180,165}, 
{ 182,166},   
{ 187,168},
{ 189,169},
{ 193,171},
{ 196,172},
{ 200,174},
{ 203,175}, 
{ 205,176},
{ 208,177}, 
{ 213,179}, 
{ 216,180},  
{ 219,181},   
{ 222,182},
{ 225,183},  
{ 228,184},   
{ 232,185},
{ 235,186},
{ 238,187},
{ 241,188},
{ 245,189},
{ 249,190},
{ 253,191},
{ 256,192}, 
{ 260,193},
{ 265,194},
{ 269,195},
{ 274,196},   
{ 278,197},
{ 283,198},
{ 288,199},
{ 293,200},
{ 298,201},   
{ 304,202},   
{ 310,203},
{ 315,204},
{ 322,205},   
{ 328,206},   
{ 335,207},   
{ 342,208},   
{ 349,209},   
{ 357,210},   
{ 365,211},   
{ 373,212}, 
{ 381,213},
{ 400,215},      
{ 420,217},   
{ 432,218},   
{ 443,219},      
{ 468,221},   
{ 482,222},   
{ 497,223},   
{ 512,224},
{ 529,225}, 	 
{ 546,226},   
{ 566,227},   
{ 585,228}, 	 
{ 607,229},   
{ 631,230},   
{ 656,231},   
{ 683,232}
};
/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT IMX219MIPISensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT IMX219MIPISensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define IMX219MIPI_write_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 1, IMX219MIPI_WRITE_ID)
//#define IMX219_wordwrite_cmos_sensor(addr, para) iWriteReg((u16) addr , (u32) para , 2, imx219MIPI_WRITE_ID)

kal_uint16 IMX219MIPI_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,IMX219MIPI_WRITE_ID);
    return get_byte;
}

/****************add OTP O-film*********************/

#define VALID_OTP          0x01
#define O_FILM_ID           0x07

unsigned short golden_r_Gr;
unsigned short golden_Gb_Gr;
unsigned short golden_b_Gr;

unsigned short current_r_Gr;
unsigned short current_Gb_Gr;
unsigned short current_b_Gr;

void IMX219_readpage(kal_uint16 page, BYTE *buf)
{
   kal_uint16 cnt = 0;
   USHORT i = 0x3204;
   if(page<0 || page>11)
   	return;
   if(buf==NULL)
   	return;

   IMX219MIPI_write_cmos_sensor(0x0100,0x00);

   //12MHZ target 25us
   IMX219MIPI_write_cmos_sensor(0x3302,0x01);
   IMX219MIPI_write_cmos_sensor(0x3303,0x2c);

   IMX219MIPI_write_cmos_sensor(0x012A,0x0c);
   IMX219MIPI_write_cmos_sensor(0x012B,0x00);
   /*
   //24MHZ,target 25us
   IMX219MIPI_write_cmos_sensor(0x3302,0x02);
   IMX219MIPI_write_cmos_sensor(0x3303,0x58);

   IMX219MIPI_write_cmos_sensor(0x012A,0x18);
   IMX219MIPI_write_cmos_sensor(0x012B,0x00);
   */
   //set ECC here OFF
  // if(page == 1||page == 0)
   IMX219MIPI_write_cmos_sensor(0x3300,0x08);
   
   //IMX219MIPI_write_cmos_sensor(0x3300,0x00);
   //set Read mode
   IMX219MIPI_write_cmos_sensor(0x3200,0x01);
   //check read status OK?
   cnt = 0;
   while((IMX219MIPI_read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
   
   //set page
   IMX219MIPI_write_cmos_sensor(0x3202,page);

   //read OTP buffer to sensor
   for(i=0x3204;i<=0x3243;i++)
   	{
   	buf[i-0x3204] = (BYTE)IMX219MIPI_read_cmos_sensor(i);
   	}

   //check read status OK?
   cnt = 0;
    while((IMX219MIPI_read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
	IMX219MIPI_write_cmos_sensor(0x0100,0x01);
	#if 1 //defined(SLT_DRV_W5520_CONFIG)
	if(video_flag == 2)
	{
		VideoFullSizeSetting();
	}
	#endif
   
}

void IMX219_readpage0(kal_uint16 page, BYTE *buf)
{
   kal_uint16 cnt = 0;
   USHORT i = 0x3204;
   if(page<0 || page>11)
   	return;
   if(buf==NULL)
   	return;

   IMX219MIPI_write_cmos_sensor(0x0100,0x00);

   //12MHZ target 25us
   IMX219MIPI_write_cmos_sensor(0x3302,0x01);
   IMX219MIPI_write_cmos_sensor(0x3303,0x2c);

   IMX219MIPI_write_cmos_sensor(0x012A,0x0c);
   IMX219MIPI_write_cmos_sensor(0x012B,0x00);
   /*
   //24MHZ,target 25us
   IMX219MIPI_write_cmos_sensor(0x3302,0x02);
   IMX219MIPI_write_cmos_sensor(0x3303,0x58);

   IMX219MIPI_write_cmos_sensor(0x012A,0x18);
   IMX219MIPI_write_cmos_sensor(0x012B,0x00);
   */
   //set ECC here OFF
  
   //IMX219MIPI_write_cmos_sensor(0x3300,0x08);
    //set ECC here ON
   IMX219MIPI_write_cmos_sensor(0x3300,0x00);
   //set Read mode
   IMX219MIPI_write_cmos_sensor(0x3200,0x01);
   //check read status OK?
   cnt = 0;
   while((IMX219MIPI_read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
   
   //set page
   IMX219MIPI_write_cmos_sensor(0x3202,page);

   //read OTP buffer to sensor
   for(i=0x3204;i<=0x3243;i++)
   	{
   	buf[i-0x3204] = (BYTE)IMX219MIPI_read_cmos_sensor(i);
   	}

   //check read status OK?
   cnt = 0;
    while((IMX219MIPI_read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
   
}


unsigned char get_otp_module_id()
{	
	unsigned char module_id = 0;
	unsigned char calibration_version = 0;
	unsigned char production_year = 0;
	unsigned char production_month = 0;
	unsigned char production_day = 0;
	unsigned char lens_id = 0;
	unsigned char vcm_id = 0;
	unsigned char driveric_id = 0;
	unsigned char temp = 0;
       BYTE pBasicInfoData[64] = {0};
	
	IMX219_readpage(0,pBasicInfoData);

	//BasicInfo Flag check
       kal_uint16 nBasicInfoFlag = pBasicInfoData[0];
	kal_uint16 nBasicInfoGroup;
	kal_uint16 sum;	
	if((nBasicInfoFlag&0x30) ==0x10)
		{
		nBasicInfoGroup = 2;
		module_id = pBasicInfoData[1+12*(nBasicInfoGroup-1)];
		return module_id;		
		}
	else if((nBasicInfoFlag & 0xc0) == 0x40)
		{
		nBasicInfoGroup = 1;
		module_id = pBasicInfoData[1+12*(nBasicInfoGroup-1)];
		return module_id;
		}
	else
		{
		SENSORDB("Basic Info is Invalid or empty");
		}
	  
        SENSORDB("O-film module_id = %x\n",module_id);

	calibration_version = pBasicInfoData[2+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film calibration_version = %x\n",calibration_version);

	production_year = pBasicInfoData[3+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film production_year = %x\n",production_year);

	production_month = pBasicInfoData[4+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film production_month = %x\n",production_month);

	production_day = pBasicInfoData[5+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film production_day = %x\n",production_day);

	lens_id = pBasicInfoData[6+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film lens_id = %x\n",lens_id);

	vcm_id = pBasicInfoData[7+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film vcm_id = %x\n",vcm_id);

	driveric_id = pBasicInfoData[8+12*(nBasicInfoGroup-1)];
	SENSORDB("O-film driveric_id = %x\n",driveric_id);

       //BasicInfo CheckSum
	sum = 0;
	int i;
	for(i=1+12*(nBasicInfoGroup-1);i<12+12*(nBasicInfoGroup-1);i++)
	{
	     sum+= pBasicInfoData[i];
	}
       kal_uint16 nBasicInfoCheckSum = sum % 0xff +1;

	if(pBasicInfoData[12+12*(nBasicInfoGroup-1)] != nBasicInfoCheckSum)
	{
            SENSORDB("Basic Info CheckSum is error");
	}
	
//	module_id = 0;
//	printk("OTP_Module ID error:0x%02.\n".module_id);
//	return module_id;
      return module_id;
	
}

unsigned char get_otp_AFData()
{
       BYTE pBasicAFInfoData[64] = {0};
	
	IMX219_readpage(0,pBasicAFInfoData); 
	//Basic AF Info Flag check
       kal_uint16 nBasicAFInfoFlag = pBasicAFInfoData[25];
	kal_uint16 nBasicAFInfoGroup;
	kal_uint16 sum;

	if((nBasicAFInfoFlag&0x30) ==0x10)
		{
		nBasicAFInfoGroup = 2;
		}
	else if((nBasicAFInfoFlag & 0xc0) == 0x40)
		{
		nBasicAFInfoGroup = 1;
		}
	else
		{
		SENSORDB("Basic AF Info is Invalid or empty");
		}

       //Basic AF Info CheckSum
	sum = 0;
	kal_uint16 i;
	for(i=26+7*(nBasicAFInfoGroup-1);i<26+7*(nBasicAFInfoGroup-1);i++)
	{
	     sum+= pBasicAFInfoData[i];
	}
       kal_uint16 nBasicAFInfoCheckSum = sum % 0xff +1;

	if(pBasicAFInfoData[32+7*(nBasicAFInfoGroup-1)] != nBasicAFInfoCheckSum)
	{
            SENSORDB("Basic AF Info CheckSum is error");
	}
	
	return 1;
	
}

void IMX219_ApplyLSCAutoLoad(kal_uint16 table)
{
   //enable it
  
      IMX219MIPI_write_cmos_sensor(0x0190,0x01);
      IMX219MIPI_write_cmos_sensor(0x0192,table);
      IMX219MIPI_write_cmos_sensor(0x0191,0x00);
      IMX219MIPI_write_cmos_sensor(0x0193,0x00);
      IMX219MIPI_write_cmos_sensor(0x01a4,0x03);
}

unsigned char get_otp_lsc()
{
      //LSC flag check
      BYTE pBasicInfoData[64] = {0};
      IMX219_readpage(0,pBasicInfoData);  
      kal_uint16 nLSCFlag = pBasicInfoData[63];
      kal_uint16 lscTable;
      kal_uint16 sum;
      kal_uint16 i;
	  
     	  
      BYTE pLSCInfoData[256] = {0};
      kal_uint16 nLSCGroup;
      if((nLSCFlag & 0xc0) == 0x40)
      	{
      	     nLSCGroup = 1;
	     lscTable = 0;
			 
	     for (i = 2; i < 5; i++)  
		{
			IMX219_readpage(i, pLSCInfoData+ (i-2)*64);
		}
		
      	}
	 else if((nLSCFlag & 0x30) == 0x10)
	 {
	     nLSCGroup = 2;
	     lscTable = 2;
		 
	      for (i = 7; i < 11; i++)  
		{
			IMX219_readpage(i, pLSCInfoData+ (i-7)*64);
		}
	 }
	 else
	 {
             SENSORDB("LSC flag is error");
	 }

	// LSC CheckSum
	sum = 0;
	//kal_uint16 i;
	for(i=0+30*(nLSCGroup-1);i<175+30*(nLSCGroup-1);i++)
	{
              sum += pLSCInfoData[i];
	      //printk("pLSCInfoData[i] = %d\n",pLSCInfoData[i]);
	}
	kal_uint16 nLSCCheckSum = sum % 0xff + 1;
	//printk()
	SENSORDB("pBasicInfoData[61+(nLSCGroup-1)] = %d\n",pBasicInfoData[61+(nLSCGroup-1)]);
	SENSORDB("nLSCCheckSum = %d\n",nLSCCheckSum);
	if(pBasicInfoData[61+(nLSCGroup-1)] != nLSCCheckSum)
	{
            SENSORDB("LSC CheckSum is error");
	}

	IMX219_ApplyLSCAutoLoad(lscTable);

	return 1;
      
}

int get_otp_wb(u8 *buf,u32 length)
{
      //WB flag check
      BYTE pWBInfoData[64] = {0};
      unsigned char temph = 0;
      unsigned char templ = 0;
      kal_uint16 sum;
	  int i=0;
	  
      IMX219_readpage(1,pWBInfoData);
	  
      kal_uint16 nWBFlag = pWBInfoData[0];
      kal_uint16 nWBGroup;
	if((nWBFlag & 0x30) == 0x10)
      	{
      	     nWBGroup = 2; 
	     temph = pWBInfoData[22];
	     templ = pWBInfoData[23];
	     current_r_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() current_r_Gr=%x\n",current_r_Gr);

	     temph = pWBInfoData[24];
	     templ = pWBInfoData[25];
	     current_b_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() current_b_Gr=%x\n",current_b_Gr);

	     temph = pWBInfoData[26];
	     templ = pWBInfoData[27];
	     current_Gb_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() current_Gb_Gr=%x\n",current_Gb_Gr);

	     temph = pWBInfoData[28];
	     templ = pWBInfoData[29];
	     golden_r_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() golden_r_Gr=%x\n",golden_r_Gr);

	     temph = pWBInfoData[30];
	     templ = pWBInfoData[31];
	     golden_b_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() golden_b_Gr=%x\n",golden_r_Gr);

	     temph = pWBInfoData[32];
	     templ = pWBInfoData[33];
	     golden_Gb_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group2 get_otp_wb() golden_Gb_Gr=%x\n",golden_Gb_Gr);
		 for (i=0;i<length;i++)
		 {
		 	buf[i]=pWBInfoData[22+i];
		 }

	}
        else if((nWBFlag & 0xC0) == 0x40)
	{
      	     nWBGroup = 1;

	     temph = pWBInfoData[1];
	     templ = pWBInfoData[2];
	     current_r_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() current_r_Gr=%x\n",current_r_Gr);

	     temph = pWBInfoData[3];
	     templ = pWBInfoData[4];
	     current_b_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() current_b_Gr=%x\n",current_b_Gr);

	     temph = pWBInfoData[5];
	     templ = pWBInfoData[6];
	     current_Gb_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() current_Gb_Gr=%x\n",current_Gb_Gr);

	     temph = pWBInfoData[7];
	     templ = pWBInfoData[8];
	     golden_r_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() golden_r_Gr=%x\n",golden_r_Gr);

	     temph = pWBInfoData[9];
	     templ = pWBInfoData[10];
	     golden_b_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() golden_b_Gr=%x\n",golden_b_Gr);

	     temph = pWBInfoData[11];
	     templ = pWBInfoData[12];
	     golden_Gb_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
	     SENSORDB("---o-film group1 get_otp_wb() golden_Gb_Gr=%x\n",golden_Gb_Gr);
		 for (i=0;i<length;i++)
		 {
		 	buf[i]=pWBInfoData[1+i];
		 }
        }
        else
	{
	     SENSORDB("WB flag is error");
		 return 1;
	}
	
	//WB Data CheckSum
	sum = 0;
	//kal_uint16 i;
	for(i=1+21*(nWBGroup-1);i<21+21*(nWBGroup-1);i++)
	{
	     sum+=pWBInfoData[i];
	}
	kal_uint16 nWBCheckSum = (sum % 0xff + 1);
	if(pWBInfoData[21+21*(nWBGroup-1)] != nWBCheckSum)
	{
            SENSORDB("WB Data CheckSum is error");
			return 1;
	}
	return 0;
}

bool otp_update()
{
    unsigned char FLG  = 0x00;
    unsigned char MID = 0x00;

    MID = get_otp_module_id();
    if(MID != O_FILM_ID)
    	{
    	 SENSORDB("NO O_film Module!!!\n");
	 return KAL_FALSE;
    	}

    if(!get_otp_AFData())
    	{
    	   SENSORDB("Basic AF data is  Err\n");
	   return KAL_FALSE;
    	}


    if(!get_otp_lsc())
    	{
    	   SENSORDB("Update LSC Err\n");
	   return KAL_FALSE;
    	}
/*
    if(!get_otp_wb())
    	{
    	   SENSORDB("wb data Err\n");
	   return KAL_FALSE;
    	}
*/
	return KAL_TRUE;
}

/****************OTP end O-film************************/

void IMX219MIPI_write_shutter(kal_uint16 shutter)
{
	kal_uint32 frame_length = 0,line_length=0,shutter1=0;
    kal_uint32 extra_lines = 0;
	kal_uint32 max_exp_shutter = 0;
	kal_uint32 min_framelength = IMX219MIPI_PV_LINE_LENGTH_PIXELS;
	unsigned long flags;	
	//SENSORDB("[IMX219MIPI]enter IMX219MIPI_write_shutter function\n"); 
	//SENSORDB("[IMX219MIPI]IMX219MIPI_write_shutter shutter:%d \n", shutter);
	
	    if (IMX219MIPI_sensor.pv_mode == KAL_TRUE) 
		 {
		   max_exp_shutter = IMX219MIPI_PV_FRAME_LENGTH_LINES + IMX219MIPI_sensor.pv_dummy_lines-4;
	     }
	     else if (IMX219MIPI_sensor.video_mode== KAL_TRUE) 
	     {
	       max_exp_shutter = IMX219MIPI_VIDEO_FRAME_LENGTH_LINES + IMX219MIPI_sensor.video_dummy_lines-4;
		 }	 
	     else if (IMX219MIPI_sensor.capture_mode== KAL_TRUE) 
	     {
	       max_exp_shutter = IMX219MIPI_FULL_FRAME_LENGTH_LINES + IMX219MIPI_sensor.cp_dummy_lines-4;
		 }	 
		 else
		 	{
		 	
			SENSORDB("sensor mode error\n");
		 	}
		 
		 if(shutter > max_exp_shutter)
		   extra_lines = shutter - max_exp_shutter;
		 else 
		   extra_lines = 0;
		 if (IMX219MIPI_sensor.pv_mode == KAL_TRUE) 
		 {
	       frame_length =IMX219MIPI_PV_FRAME_LENGTH_LINES+ IMX219MIPI_sensor.pv_dummy_lines + extra_lines;
		   line_length = IMX219MIPI_PV_LINE_LENGTH_PIXELS+ IMX219MIPI_sensor.pv_dummy_pixels;
		   spin_lock_irqsave(&IMX219_drv_lock,flags);
		   IMX219MIPI_sensor.pv_line_length = line_length;
		   IMX219MIPI_sensor.pv_frame_length = frame_length;
		   spin_unlock_irqrestore(&IMX219_drv_lock,flags);
		 }
		 else if (IMX219MIPI_sensor.video_mode== KAL_TRUE) 
	     {
		    frame_length = IMX219MIPI_VIDEO_FRAME_LENGTH_LINES+ IMX219MIPI_sensor.video_dummy_lines + extra_lines;
			line_length =IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS + IMX219MIPI_sensor.video_dummy_pixels;
			spin_lock_irqsave(&IMX219_drv_lock,flags);
			IMX219MIPI_sensor.video_line_length = line_length;
		    IMX219MIPI_sensor.video_frame_length = frame_length;
			spin_unlock_irqrestore(&IMX219_drv_lock,flags);
		 } 
		 else if(IMX219MIPI_sensor.capture_mode== KAL_TRUE)
		 	{
		    frame_length = IMX219MIPI_FULL_FRAME_LENGTH_LINES+ IMX219MIPI_sensor.cp_dummy_lines + extra_lines;
			line_length =IMX219MIPI_FULL_LINE_LENGTH_PIXELS + IMX219MIPI_sensor.cp_dummy_pixels;
			spin_lock_irqsave(&IMX219_drv_lock,flags);
			IMX219MIPI_sensor.cp_line_length = line_length;
		    IMX219MIPI_sensor.cp_frame_length = frame_length;
			spin_unlock_irqrestore(&IMX219_drv_lock,flags);
		 }
		 else
		 	{
		 	
			SENSORDB("sensor mode error\n");
		 	}
		//IMX219MIPI_write_cmos_sensor(0x0100,0x00);// STREAM STop
		
		IMX219MIPI_write_cmos_sensor(0x0160, (frame_length >>8) & 0xFF);
	    IMX219MIPI_write_cmos_sensor(0x0161, frame_length & 0xFF);	  
	    IMX219MIPI_write_cmos_sensor(0x015a, (shutter >> 8) & 0xFF);
	    IMX219MIPI_write_cmos_sensor(0x015b, shutter  & 0xFF);
		SENSORDB("[IMX219MIPI]man-flicker frame_length=%d, shutter=%d\n", frame_length, shutter);
    SENSORDB("[IMX219MIPI]exit IMX219MIPI_write_shutter function\n");
}   /* write_IMX219MIPI_shutter */

static kal_uint16 IMX219MIPIReg2Gain(const kal_uint8 iReg)
{
	SENSORDB("[IMX219MIPI]enter IMX219MIPIReg2Gain function\n");
    kal_uint8 iI;
    // Range: 1x to 8x
    for (iI = 0; iI < IMX219MIPI_MaxGainIndex; iI++) 
	{
        if(iReg < IMX219MIPI_sensorGainMapping[iI][1])
		{
            break;
        }
		if(iReg == IMX219MIPI_sensorGainMapping[iI][1])			
		{			
			return IMX219MIPI_sensorGainMapping[iI][0];
		}    
    }
	SENSORDB("[IMX219MIPI]exit IMX219MIPIReg2Gain function\n");
    return IMX219MIPI_sensorGainMapping[iI-1][0];
}
static kal_uint8 IMX219MIPIGain2Reg(const kal_uint16 iGain)
{
	kal_uint8 iI;
    SENSORDB("[IMX219MIPI]enter IMX219MIPIGain2Reg function\n");
    for (iI = 0; iI < (IMX219MIPI_MaxGainIndex-1); iI++) 
	{

        if(iGain <IMX219MIPI_sensorGainMapping[iI][0])
		{    
            break;
        }
		if(iGain < IMX219MIPI_sensorGainMapping[iI][0])
		{                
			return IMX219MIPI_sensorGainMapping[iI][1];       
		}
			

    }
    if(iGain != IMX219MIPI_sensorGainMapping[iI][0])
    {
         SENSORDB("[IMX219MIPIGain2Reg] Gain mapping don't correctly:%d %d \n", iGain, IMX219MIPI_sensorGainMapping[iI][0]);
    }
	SENSORDB("[IMX219MIPI]exit IMX219MIPIGain2Reg function\n");
    return IMX219MIPI_sensorGainMapping[iI-1][1];
	//return NONE;
}

/*************************************************************************
* FUNCTION
*    IMX219MIPI_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT16 iPreGain = 0;
void IMX219MIPI_SetGain(UINT16 iGain)
{   

    kal_uint8 iReg;
	
	#if 1
	//xb.pang for zsd ae peak;
	//iGain = 1024;
	//
	if (iPreGain != iGain)
	{
		SENSORDB("[IMX219MIPI]enter IMX219MIPI_SetGain function\n");
		iPreGain = iGain;
    	iReg = IMX219MIPIGain2Reg(iGain);
    	IMX219MIPI_write_cmos_sensor(0x0157, (kal_uint8)iReg);
		
		SENSORDB("[IMX219MIPI]exit IMX219MIPI_SetGain function\n");
	}
    #endif //xb.pang need check
	
}   /*  IMX219MIPI_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_IMX219MIPI_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_IMX219MIPI_gain(void)
{  
	SENSORDB("[IMX219MIPI]enter read_IMX219MIPI_gain function\n");
	//xb.pang need check
    return (kal_uint16)(IMX219MIPI_read_cmos_sensor(0x0157)) ;
}  /* read_IMX219MIPI_gain */

void write_IMX219MIPI_gain(kal_uint16 gain)
{
    IMX219MIPI_SetGain(gain);
}
void IMX219MIPI_camera_para_to_sensor(void)
{

}


/*************************************************************************
* FUNCTION
*    IMX219MIPI_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX219MIPI_sensor_to_camera_para(void)
{

}

/*************************************************************************
* FUNCTION
*    IMX219MIPI_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  IMX219MIPI_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void IMX219MIPI_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{

}

void IMX219MIPI_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
	
}
kal_bool IMX219MIPI_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
  
    return KAL_TRUE;
}
static void IMX219MIPI_SetDummy(const kal_uint16 iPixels, const kal_uint16 iLines)
{

	//xb.pang need check
	kal_uint32 frame_length = 0, line_length = 0;
    if(IMX219MIPI_sensor.pv_mode == KAL_TRUE)
   	{
   	 spin_lock(&IMX219_drv_lock);    
   	 IMX219MIPI_sensor.pv_dummy_pixels = iPixels;
	 IMX219MIPI_sensor.pv_dummy_lines = iLines;
   	 IMX219MIPI_sensor.pv_line_length = IMX219MIPI_PV_LINE_LENGTH_PIXELS + iPixels;
	 IMX219MIPI_sensor.pv_frame_length = IMX219MIPI_PV_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&IMX219_drv_lock);
	 line_length = IMX219MIPI_sensor.pv_line_length;
	 frame_length = IMX219MIPI_sensor.pv_frame_length;	 	
   	}
   else if(IMX219MIPI_sensor.video_mode== KAL_TRUE)
   	{
   	 spin_lock(&IMX219_drv_lock);    
   	 IMX219MIPI_sensor.video_dummy_pixels = iPixels;
	 IMX219MIPI_sensor.video_dummy_lines = iLines;
   	 IMX219MIPI_sensor.video_line_length = IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS + iPixels;
	 IMX219MIPI_sensor.video_frame_length = IMX219MIPI_VIDEO_FRAME_LENGTH_LINES + iLines;
	 spin_unlock(&IMX219_drv_lock);
	 line_length = IMX219MIPI_sensor.video_line_length;
	 frame_length = IMX219MIPI_sensor.video_frame_length;
   	}
	else if(IMX219MIPI_sensor.capture_mode== KAL_TRUE) 
		{
	  spin_lock(&IMX219_drv_lock);	
   	  IMX219MIPI_sensor.cp_dummy_pixels = iPixels;
	  IMX219MIPI_sensor.cp_dummy_lines = iLines;
	  IMX219MIPI_sensor.cp_line_length = IMX219MIPI_FULL_LINE_LENGTH_PIXELS + iPixels;
	  IMX219MIPI_sensor.cp_frame_length = IMX219MIPI_FULL_FRAME_LENGTH_LINES + iLines;
	   spin_unlock(&IMX219_drv_lock);
	  line_length = IMX219MIPI_sensor.cp_line_length;
	  frame_length = IMX219MIPI_sensor.cp_frame_length;
    }
	else
	{
	 SENSORDB("[IMX219MIPI][IMX219MIPI_SetDummy] error \n");
	}
      SENSORDB("[IMX219MIPI][IMX219MIPI_SetDummy] line_length:%d, frame_length:%d \n", line_length, frame_length);
      IMX219MIPI_write_cmos_sensor(0x0160, (frame_length >>8) & 0xFF);
      IMX219MIPI_write_cmos_sensor(0x0161, frame_length & 0xFF);	
      IMX219MIPI_write_cmos_sensor(0x0162, (line_length >>8) & 0xFF);
      IMX219MIPI_write_cmos_sensor(0x0163, line_length & 0xFF);
      
}   /*  IMX219MIPI_SetDummy */
static void IMX219MIPI_Sensor_Init(void)
{
	SENSORDB("[IMX219MIPI]enter IMX219MIPI_Sensor_Init function\n");
	
	// The register only need to enable 1 time.    
	spin_lock(&IMX219_drv_lock);  
	IMX219MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
	spin_unlock(&IMX219_drv_lock);
	SENSORDB("[IMX219MIPI]exit IMX219MIPI_Sensor_Init function\n");
}   /*  IMX219MIPI_Sensor_Init  */
static void VideoFullSizeSetting(void)//16:9   6M
{
	SENSORDB("[IMX219MIPI]enter VideoFullSizeSetting function\n");
	IMX219MIPI_write_cmos_sensor(0x0100,   0x00);
	
	IMX219MIPI_write_cmos_sensor(0x30EB, 0x05); 
	IMX219MIPI_write_cmos_sensor(0x30EB, 0x0C); 
	IMX219MIPI_write_cmos_sensor(0x300A, 0xFF); 
	IMX219MIPI_write_cmos_sensor(0x300B, 0xFF); 
	IMX219MIPI_write_cmos_sensor(0x30EB, 0x05); 
	IMX219MIPI_write_cmos_sensor(0x30EB, 0x09); 
	IMX219MIPI_write_cmos_sensor(0x0114, 0x03); 
	IMX219MIPI_write_cmos_sensor(0x0128, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x012A, 0x18); 
	IMX219MIPI_write_cmos_sensor(0x012B, 0x00); 
												
												
												
	IMX219MIPI_write_cmos_sensor(0x0160, ((IMX219MIPI_VIDEO_FRAME_LENGTH_LINES >> 8) & 0xFF)); 
	IMX219MIPI_write_cmos_sensor(0x0161, (IMX219MIPI_VIDEO_FRAME_LENGTH_LINES & 0xFF)); 
	IMX219MIPI_write_cmos_sensor(0x0162, ((IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS >> 8) & 0xFF)); 
	IMX219MIPI_write_cmos_sensor(0x0163, (IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS & 0xFF)); 

	
	IMX219MIPI_write_cmos_sensor(0x0164, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x0165, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x0166, 0x0C); 
	IMX219MIPI_write_cmos_sensor(0x0167, 0xCF); 
	IMX219MIPI_write_cmos_sensor(0x0168, 0x01); 
	IMX219MIPI_write_cmos_sensor(0x0169, 0x36); 
	IMX219MIPI_write_cmos_sensor(0x016A, 0x08); 
	IMX219MIPI_write_cmos_sensor(0x016B, 0x6B); 
	IMX219MIPI_write_cmos_sensor(0x016C, 0x0C); 
	IMX219MIPI_write_cmos_sensor(0x016D, 0xD0); 
	IMX219MIPI_write_cmos_sensor(0x016E, 0x07); 
	IMX219MIPI_write_cmos_sensor(0x016F, 0x36); 
	IMX219MIPI_write_cmos_sensor(0x0170, 0x01); 
	IMX219MIPI_write_cmos_sensor(0x0171, 0x01); 
	IMX219MIPI_write_cmos_sensor(0x0174, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x0175, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x018C, 0x0A); 
	IMX219MIPI_write_cmos_sensor(0x018D, 0x0A); 
	IMX219MIPI_write_cmos_sensor(0x0301, 0x05); 
	IMX219MIPI_write_cmos_sensor(0x0303, 0x01); 
	IMX219MIPI_write_cmos_sensor(0x0304, 0x03); 
	IMX219MIPI_write_cmos_sensor(0x0305, 0x03); 
	IMX219MIPI_write_cmos_sensor(0x0306, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x0307, 0x51); 
	IMX219MIPI_write_cmos_sensor(0x0309, 0x0A); 
	IMX219MIPI_write_cmos_sensor(0x030B, 0x01); 
	IMX219MIPI_write_cmos_sensor(0x030C, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x030D, 0x54); 
	IMX219MIPI_write_cmos_sensor(0x455E, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x471E, 0x4B); 
	IMX219MIPI_write_cmos_sensor(0x4767, 0x0F); 
	IMX219MIPI_write_cmos_sensor(0x4750, 0x14); 
	IMX219MIPI_write_cmos_sensor(0x4540, 0x00); 
	IMX219MIPI_write_cmos_sensor(0x47B4, 0x14); 
	IMX219MIPI_write_cmos_sensor(0x4713, 0x30); 
	IMX219MIPI_write_cmos_sensor(0x478B, 0x10); 
	IMX219MIPI_write_cmos_sensor(0x478F, 0x10); 
	IMX219MIPI_write_cmos_sensor(0x4793, 0x10); 
	IMX219MIPI_write_cmos_sensor(0x4797, 0x0E); 
	IMX219MIPI_write_cmos_sensor(0x479B, 0x0E); 
	IMX219MIPI_write_cmos_sensor(0x0100, 0x01); 


	SENSORDB("[IMX219MIPI]exit VideoFullSizeSetting function\n");
}
static void PreviewSetting(void)
{
	SENSORDB("[IMX219MIPI]enter PreviewSetting function\n");
	IMX219MIPI_write_cmos_sensor(0x0100,   0x00);

	
	IMX219MIPI_write_cmos_sensor(0x30EB,  0x05);   
	IMX219MIPI_write_cmos_sensor(0x30EB,  0x0C);   
	IMX219MIPI_write_cmos_sensor(0x300A,  0xFF);   
	IMX219MIPI_write_cmos_sensor(0x300B,  0xFF);   
	IMX219MIPI_write_cmos_sensor(0x30EB,  0x05);   
	IMX219MIPI_write_cmos_sensor(0x30EB,  0x09);		  
	IMX219MIPI_write_cmos_sensor(0x0114,  0x03);   
	IMX219MIPI_write_cmos_sensor(0x0128,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x012A,  0x18);   
	IMX219MIPI_write_cmos_sensor(0x012B,  0x00);   
	  
	  
	  
	IMX219MIPI_write_cmos_sensor(0x0160,  ((IMX219MIPI_PV_FRAME_LENGTH_LINES >> 8) & 0xFF));   
	IMX219MIPI_write_cmos_sensor(0x0161,  (IMX219MIPI_PV_FRAME_LENGTH_LINES & 0xFF));   
	IMX219MIPI_write_cmos_sensor(0x0162,  ((IMX219MIPI_PV_LINE_LENGTH_PIXELS >> 8) & 0xFF));   
	IMX219MIPI_write_cmos_sensor(0x0163,  (IMX219MIPI_PV_LINE_LENGTH_PIXELS & 0xFF));   

	
	IMX219MIPI_write_cmos_sensor(0x0164,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x0165,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x0166,  0x0C);   
	IMX219MIPI_write_cmos_sensor(0x0167,  0xCF);   
	IMX219MIPI_write_cmos_sensor(0x0168,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x0169,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x016A,  0x09);   
	IMX219MIPI_write_cmos_sensor(0x016B,  0x9F);   
	IMX219MIPI_write_cmos_sensor(0x016C,  0x06);   
	IMX219MIPI_write_cmos_sensor(0x016D,  0x68);   
	IMX219MIPI_write_cmos_sensor(0x016E,  0x04);   
	IMX219MIPI_write_cmos_sensor(0x016F,  0xD0);   
	IMX219MIPI_write_cmos_sensor(0x0170,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x0171,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x0174,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x0175,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x018C,  0x0A);   
	IMX219MIPI_write_cmos_sensor(0x018D,  0x0A);   
	IMX219MIPI_write_cmos_sensor(0x0301,  0x05);   
	IMX219MIPI_write_cmos_sensor(0x0303,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x0304,  0x03);   
	IMX219MIPI_write_cmos_sensor(0x0305,  0x03);   
	IMX219MIPI_write_cmos_sensor(0x0306,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x0307,  0x51);   
	IMX219MIPI_write_cmos_sensor(0x0309,  0x0A);   
	IMX219MIPI_write_cmos_sensor(0x030B,  0x01);   
	IMX219MIPI_write_cmos_sensor(0x030C,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x030D,  0x54);   
	IMX219MIPI_write_cmos_sensor(0x455E,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x471E,  0x4B);   
	IMX219MIPI_write_cmos_sensor(0x4767,  0x0F);   
	IMX219MIPI_write_cmos_sensor(0x4750,  0x14);   
	IMX219MIPI_write_cmos_sensor(0x4540,  0x00);   
	IMX219MIPI_write_cmos_sensor(0x47B4,  0x14);   
	IMX219MIPI_write_cmos_sensor(0x4713,  0x30);   
	IMX219MIPI_write_cmos_sensor(0x478B,  0x10);   
	IMX219MIPI_write_cmos_sensor(0x478F,  0x10);   
	IMX219MIPI_write_cmos_sensor(0x4793,  0x10);   
	IMX219MIPI_write_cmos_sensor(0x4797,  0x0E);   
	IMX219MIPI_write_cmos_sensor(0x479B,  0x0E);   
	IMX219MIPI_write_cmos_sensor(0x0100,  0x01); 

	// The register only need to enable 1 time.    
	spin_lock(&IMX219_drv_lock);  
	//IMX219MIPI_Auto_Flicker_mode = KAL_FALSE;	  // reset the flicker status	 
	spin_unlock(&IMX219_drv_lock);
	SENSORDB("[IMX219MIPI]exit PreviewSetting function\n");
}

void IMX219MIPI_set_8M(void)
{	//77 capture setting
	SENSORDB("[IMX219MIPI]enter IMX219MIPI_set_8M function\n");
	IMX219MIPI_write_cmos_sensor(0x0100,   0x00); 
	
	IMX219MIPI_write_cmos_sensor(0x30EB,   0x05); 
	IMX219MIPI_write_cmos_sensor(0x30EB,   0x0C); 
	IMX219MIPI_write_cmos_sensor(0x300A,   0xFF); 
	IMX219MIPI_write_cmos_sensor(0x300B,   0xFF); 
	IMX219MIPI_write_cmos_sensor(0x30EB,   0x05); 
	IMX219MIPI_write_cmos_sensor(0x30EB,   0x09); 
	
	IMX219MIPI_write_cmos_sensor(0x0114,   0x03); 
	IMX219MIPI_write_cmos_sensor(0x0128,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x012A,   0x18); 
	IMX219MIPI_write_cmos_sensor(0x012B,   0x00); 
												  
												  
												  
	IMX219MIPI_write_cmos_sensor(0x0160,   ((IMX219MIPI_FULL_FRAME_LENGTH_LINES >> 8) & 0xFF)); 
	IMX219MIPI_write_cmos_sensor(0x0161,   (IMX219MIPI_FULL_FRAME_LENGTH_LINES & 0xFF));
	//IMX219MIPI_write_cmos_sensor(0x0160,   0x0e); 
	//IMX219MIPI_write_cmos_sensor(0x0161,   0xae); 
	
	IMX219MIPI_write_cmos_sensor(0x0162,   ((IMX219MIPI_FULL_LINE_LENGTH_PIXELS >> 8) & 0xFF));  
	IMX219MIPI_write_cmos_sensor(0x0163,   (IMX219MIPI_FULL_LINE_LENGTH_PIXELS & 0xFF));
	
	IMX219MIPI_write_cmos_sensor(0x0164,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x0165,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x0166,   0x0C); 
	IMX219MIPI_write_cmos_sensor(0x0167,   0xCF); 
	IMX219MIPI_write_cmos_sensor(0x0168,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x0169,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x016A,   0x09); 
	IMX219MIPI_write_cmos_sensor(0x016B,   0x9F); 
	IMX219MIPI_write_cmos_sensor(0x016C,   0x0C); 
	IMX219MIPI_write_cmos_sensor(0x016D,   0xD0); 
	
	IMX219MIPI_write_cmos_sensor(0x016E,   0x09); 
	IMX219MIPI_write_cmos_sensor(0x016F,   0xA0); 
	//IMX219MIPI_write_cmos_sensor(0x016E,   0x07); 
	//IMX219MIPI_write_cmos_sensor(0x016F,   0x36); 
	
	IMX219MIPI_write_cmos_sensor(0x0170,   0x01); 
	IMX219MIPI_write_cmos_sensor(0x0171,   0x01); 
	IMX219MIPI_write_cmos_sensor(0x0174,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x0175,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x018C,   0x0A); 
	IMX219MIPI_write_cmos_sensor(0x018D,   0x0A); 
	IMX219MIPI_write_cmos_sensor(0x0301,   0x05); 
	IMX219MIPI_write_cmos_sensor(0x0303,   0x01); 
	IMX219MIPI_write_cmos_sensor(0x0304,   0x03); 
	IMX219MIPI_write_cmos_sensor(0x0305,   0x03); 
	IMX219MIPI_write_cmos_sensor(0x0306,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x0307,   0x51); //0x51
	IMX219MIPI_write_cmos_sensor(0x0309,   0x0A); 
	IMX219MIPI_write_cmos_sensor(0x030B,   0x01); 
	IMX219MIPI_write_cmos_sensor(0x030C,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x030D,   0x54);//0x54 
	IMX219MIPI_write_cmos_sensor(0x455E,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x471E,   0x4B); 
	IMX219MIPI_write_cmos_sensor(0x4767,   0x0F); 
	IMX219MIPI_write_cmos_sensor(0x4750,   0x14); 
	IMX219MIPI_write_cmos_sensor(0x4540,   0x00); 
	IMX219MIPI_write_cmos_sensor(0x47B4,   0x14); 
	IMX219MIPI_write_cmos_sensor(0x4713,   0x30); 
	IMX219MIPI_write_cmos_sensor(0x478B,   0x10); 
	IMX219MIPI_write_cmos_sensor(0x478F,   0x10); 
	IMX219MIPI_write_cmos_sensor(0x4793,   0x10); 
	IMX219MIPI_write_cmos_sensor(0x4797,   0x0E); 
	IMX219MIPI_write_cmos_sensor(0x479B,   0x0E); 
	IMX219MIPI_write_cmos_sensor(0x0100,   0x01); 

	SENSORDB("[IMX219MIPI]exit IMX219MIPI_set_8M function\n"); 
}
/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*   IMX219MIPIOpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 IMX219MIPIOpen(void)
{
    int  retry = 0; 
	kal_uint16 sensorid;
    // check if sensor ID correct
    retry = 3; 
	SENSORDB("[IMX219MIPI]enter IMX219MIPIOpen function\n");
    do {
	   sensorid=(kal_uint16)(((IMX219MIPI_read_cmos_sensor(0x0000)&&0x0f)<<8) | IMX219MIPI_read_cmos_sensor(0x0001));  
	   spin_lock(&IMX219_drv_lock);    
	   IMX219MIPI_sensor_id =sensorid;
	   spin_unlock(&IMX219_drv_lock);
		if (IMX219MIPI_sensor_id == IMX219MIPI_SENSOR_ID)
			break; 
		retry--; 
	    }
	while (retry > 0);
    SENSORDB("Read Sensor ID = 0x%04x\n", IMX219MIPI_sensor_id);
    if (IMX219MIPI_sensor_id != IMX219MIPI_SENSOR_ID)
        return ERROR_SENSOR_CONNECT_FAIL;
	iPreGain = 0;
    IMX219MIPI_Sensor_Init();
    if(!otp_update()) //o-film otp add code 
    {
    	SENSORDB("lzq----ofilm otp add code IMX219 err failed!!\n");
    }
    else
    {
    	SENSORDB("lzq----ofilm otp add code IMX219 succssful!!\n");
    }
	
	sensorid=read_IMX219MIPI_gain();
	spin_lock(&IMX219_drv_lock);	
	IMX219MIPI_sensor.pv_mode=KAL_TRUE;	
    IMX219MIPI_sensor_gain_base = sensorid;
	spin_unlock(&IMX219_drv_lock);
	SENSORDB("[IMX219MIPI]exit IMX219MIPIOpen function\n");
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX219MIPIGetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static u8 imx219_rc = 0;
static u8 imx219_used = 0;
#ifdef SLT_DEVINFO_CMM
#include <linux/dev_info.h>
#endif
UINT32 IMX219MIPIGetSensorID(UINT32 *sensorID) 
{

	// *sensorID = IMX219MIPI_SENSOR_ID;
	// return ERROR_NONE;
	imx219_rc++;
	
	#ifdef SLT_DEVINFO_CMM
    struct devinfo_struct *dev;
	#endif

    int  retry = 3; 
	SENSORDB("[IMX219MIPI]enter IMX219MIPIGetSensorID function\n");
    // check if sensor ID correct
    do {		
	   *sensorID =(kal_uint16)(((IMX219MIPI_read_cmos_sensor(0x0000)&&0x0f)<<8) | IMX219MIPI_read_cmos_sensor(0x0001)); 
        if (*sensorID == IMX219MIPI_SENSOR_ID)
            break;
        retry--; 
    } while (retry > 0);

    if (*sensorID == IMX219MIPI_SENSOR_ID) {
			
		#ifdef SLT_DEVINFO_CMM
		// S5K3L2X2DB("zjz: To ensure this segment will run. LensID = 0x%04x VcmID = 0x%04x\n", LensID, VcmID);
		dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);
		dev->device_type = "CCM";
		dev->device_module = "O-FILM";
		dev->device_vendor	="O-FILM";		 
		dev->device_ic	= "IMX219";
		dev->device_version = "CDGM+unknown";
		dev->device_info = "800W";
		dev->device_used    =   DEVINFO_USED;

		imx219_used = 1; 
		devinfo_check_add_device(dev);	   
		#endif			  
    }
    if (*sensorID != IMX219MIPI_SENSOR_ID) {
 		#ifdef SLT_DEVINFO_CMM
		  if(imx219_rc == 2 && imx219_used == 0 )
		  {
			dev=(struct devinfo_struct*)kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);
			dev->device_type = "CCM";
			dev->device_module = "O-FILM";
			dev->device_vendor	="O-FILM";		 
			dev->device_ic	= "IMX219";
			dev->device_version = "CDGM+unknown";
			dev->device_info = "800W";			
			dev->device_used	=	DEVINFO_UNUSED;
			devinfo_check_add_device(dev);
		  }  
		#endif
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	SENSORDB("[IMX219MIPI]exit IMX219MIPIGetSensorID function\n");
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   IMX219MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of IMX219MIPI to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX219MIPI_SetShutter(kal_uint16 iShutter)
{
	//xb.pang need check
	SENSORDB("[IMX219MIPI]%s():shutter=%d\n",__FUNCTION__,iShutter);
    if (iShutter < 1)
        iShutter = 1; 
	else if(iShutter > 0xffff)
		iShutter = 0xffff;
	unsigned long flags;
	spin_lock_irqsave(&IMX219_drv_lock,flags);
    IMX219MIPI_sensor.pv_shutter = iShutter;	
	spin_unlock_irqrestore(&IMX219_drv_lock,flags);
	//xb.pang for zsd ae
    IMX219MIPI_write_shutter(iShutter);
	//SENSORDB("[IMX219MIPI]exit IMX219MIPIGetSensorID function\n");
}   /*  IMX219MIPI_SetShutter   */



/*************************************************************************
* FUNCTION
*   IMX219MIPI_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT16 IMX219MIPI_read_shutter(void)
{
	//xb.pang need check
    return (UINT16)( (IMX219MIPI_read_cmos_sensor(0x015a)<<8) | IMX219MIPI_read_cmos_sensor(0x015b) );
}

/*************************************************************************
* FUNCTION
*   IMX219MIPI_night_mode
*
* DESCRIPTION
*   This function night mode of IMX219MIPI.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void IMX219MIPI_NightMode(kal_bool bEnable)
{
	SENSORDB("[IMX219MIPI]IMX219MIPI_NightMode\n");
}/*	IMX219MIPI_NightMode */



/*************************************************************************
* FUNCTION
*   IMX219MIPIClose
*
* DESCRIPTION
*   This function is to turn off sensor module power.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX219MIPIClose(void)
{
	//xb.pang need check
    IMX219MIPI_write_cmos_sensor(0x0100,0x00);
    return ERROR_NONE;
}	/* IMX219MIPIClose() */

void IMX219MIPISetFlipMirror(kal_int32 imgMirror)
{
	//xb.pang need check
    kal_uint8  iTemp; 
	SENSORDB("[IMX219MIPI]enter IMX219MIPISetFlipMirror function\n");
    iTemp = IMX219MIPI_read_cmos_sensor(0x0172) & 0x03;	//Clear the mirror and flip bits.
    switch (imgMirror)
    {
        case IMAGE_NORMAL:
            IMX219MIPI_write_cmos_sensor(0x0172, 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            IMX219MIPI_write_cmos_sensor(0x0172, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            IMX219MIPI_write_cmos_sensor(0x0172, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            IMX219MIPI_write_cmos_sensor(0x0172, 0x00);	//Set mirror and flip
            break;
    }
	SENSORDB("[IMX219MIPI]exit IMX219MIPISetFlipMirror function\n");
}

/*
*	FOR TEST PATTERN--test pattern need change sensor out size
*	first_testpattern=false----set sensor out size: preview size
*   first_testpattern=true----set sensor out size: capture size
*/
bool first_testpattern = false;
UINT32 IMX219MIPISetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[IMX219MIPISetTestPatternMode] Test pattern enable:%d\n", bEnable);
    if(IMX219MIPI_sensor.capture_mode == KAL_FALSE)
    {
	    if(bEnable) 
		{   
			//1640 x 1232
			// enable color bar
			test_pattern_flag=TRUE;
			IMX219MIPI_write_cmos_sensor(0x0600, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x0601, 0x02); 	  
			IMX219MIPI_write_cmos_sensor(0x0624, 0x06); //W:3280---h
			IMX219MIPI_write_cmos_sensor(0x0625, 0x68); //		   l
			IMX219MIPI_write_cmos_sensor(0x0626, 0x04); //H:2464   h
			IMX219MIPI_write_cmos_sensor(0x0627, 0xd0); //		   l
			IMX219MIPI_write_cmos_sensor(0x6128, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6129, 0x02); 		  
			IMX219MIPI_write_cmos_sensor(0x613C, 0x06); //W         h
			IMX219MIPI_write_cmos_sensor(0x613D, 0x68); //		    l
			IMX219MIPI_write_cmos_sensor(0x613E, 0x04); //H         h
			IMX219MIPI_write_cmos_sensor(0x613F, 0xd0); // 		    l
			IMX219MIPI_write_cmos_sensor(0x6506, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6507, 0x00);

	    } 
		else 
		{   
			//1640 x 1232
			test_pattern_flag=FALSE;
			IMX219MIPI_write_cmos_sensor(0x0600, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x0601, 0x00); 	  
			IMX219MIPI_write_cmos_sensor(0x0624, 0x06); //W:3280---h
			IMX219MIPI_write_cmos_sensor(0x0625, 0x68); //		   l
			IMX219MIPI_write_cmos_sensor(0x0626, 0x04); //H:2464   h
			IMX219MIPI_write_cmos_sensor(0x0627, 0xd0); //		   l
			IMX219MIPI_write_cmos_sensor(0x6128, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6129, 0x02); 		  
			IMX219MIPI_write_cmos_sensor(0x613C, 0x06); //W         h
			IMX219MIPI_write_cmos_sensor(0x613D, 0x68); //		    l
			IMX219MIPI_write_cmos_sensor(0x613E, 0x04); //H         h
			IMX219MIPI_write_cmos_sensor(0x613F, 0xd0); // 		    l
			IMX219MIPI_write_cmos_sensor(0x6506, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6507, 0x00);

	    }
    }
	else
	{
        if(bEnable) 
		{   
			//3280 x 2464
			// enable color bar
			test_pattern_flag=TRUE;
			IMX219MIPI_write_cmos_sensor(0x0600, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x0601, 0x02); 	  
			IMX219MIPI_write_cmos_sensor(0x0624, 0x0C); //W:3280---h
			IMX219MIPI_write_cmos_sensor(0x0625, 0xD0); //		   l
			IMX219MIPI_write_cmos_sensor(0x0626, 0x09); //H:2464   h
			IMX219MIPI_write_cmos_sensor(0x0627, 0xA0); //		   l
			IMX219MIPI_write_cmos_sensor(0x6128, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6129, 0x02); 		  
			IMX219MIPI_write_cmos_sensor(0x613C, 0x0C); //W         h
			IMX219MIPI_write_cmos_sensor(0x613D, 0xD0); //		    l
			IMX219MIPI_write_cmos_sensor(0x613E, 0x09); //H         h
			IMX219MIPI_write_cmos_sensor(0x613F, 0xA0); // 		    l
			IMX219MIPI_write_cmos_sensor(0x6506, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6507, 0x00);

	    } 
		else 
		{   
			test_pattern_flag=FALSE;
			IMX219MIPI_write_cmos_sensor(0x0600, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x0601, 0x02); 	  
			IMX219MIPI_write_cmos_sensor(0x0624, 0x0C); //W:3280---h
			IMX219MIPI_write_cmos_sensor(0x0625, 0xD0); //		   l
			IMX219MIPI_write_cmos_sensor(0x0626, 0x09); //H:2464   h
			IMX219MIPI_write_cmos_sensor(0x0627, 0xA0); //		   l
			IMX219MIPI_write_cmos_sensor(0x6128, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6129, 0x02); 		  
			IMX219MIPI_write_cmos_sensor(0x613C, 0x0C); //W         h
			IMX219MIPI_write_cmos_sensor(0x613D, 0xD0); //		    l
			IMX219MIPI_write_cmos_sensor(0x613E, 0x09); //H         h
			IMX219MIPI_write_cmos_sensor(0x613F, 0xA0); // 		    l
			IMX219MIPI_write_cmos_sensor(0x6506, 0x00); 
			IMX219MIPI_write_cmos_sensor(0x6507, 0x00);


	    }
	}
		
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   IMX219MIPIPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX219MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;	
	SENSORDB("[IMX219MIPI]enter IMX219MIPIPreview function\n");
	spin_lock(&IMX219_drv_lock);    
	IMX219MIPI_MPEG4_encode_mode = KAL_FALSE;
	IMX219MIPI_sensor.video_mode=KAL_FALSE;
	IMX219MIPI_sensor.pv_mode=KAL_TRUE;
	IMX219MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&IMX219_drv_lock);
	PreviewSetting();
	iStartX += IMX219MIPI_IMAGE_SENSOR_PV_STARTX;
	iStartY += IMX219MIPI_IMAGE_SENSOR_PV_STARTY;
	spin_lock(&IMX219_drv_lock);	
	IMX219MIPI_sensor.cp_dummy_pixels = 0;
	IMX219MIPI_sensor.cp_dummy_lines = 0;
	IMX219MIPI_sensor.pv_dummy_pixels = 0;
	IMX219MIPI_sensor.pv_dummy_lines = 0;
	IMX219MIPI_sensor.video_dummy_pixels = 0;
	IMX219MIPI_sensor.video_dummy_lines = 0;
	IMX219MIPI_sensor.pv_line_length = IMX219MIPI_PV_LINE_LENGTH_PIXELS+IMX219MIPI_sensor.pv_dummy_pixels; 
	IMX219MIPI_sensor.pv_frame_length = IMX219MIPI_PV_FRAME_LENGTH_LINES+IMX219MIPI_sensor.pv_dummy_lines;
	spin_unlock(&IMX219_drv_lock);

	IMX219MIPI_SetDummy(IMX219MIPI_sensor.pv_dummy_pixels,IMX219MIPI_sensor.pv_dummy_lines);
	IMX219MIPI_SetShutter(IMX219MIPI_sensor.pv_shutter);
	spin_lock(&IMX219_drv_lock);	
	memcpy(&IMX219MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&IMX219_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;
	image_window->ExposureWindowWidth= IMX219MIPI_REAL_PV_WIDTH ;
	image_window->ExposureWindowHeight= IMX219MIPI_REAL_PV_HEIGHT ; 
	SENSORDB("[IMX219MIPI]eXIT IMX219MIPIPreview function\n"); 
	return ERROR_NONE;
	}	/* IMX219MIPIPreview() */

/*************************************************************************
* FUNCTION
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 IMX219MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX219MIPI]enter IMX219MIPIVideo function\n"); 
	spin_lock(&IMX219_drv_lock);    
    IMX219MIPI_MPEG4_encode_mode = KAL_TRUE;  
	IMX219MIPI_sensor.video_mode=KAL_TRUE;
	IMX219MIPI_sensor.pv_mode=KAL_FALSE;
	IMX219MIPI_sensor.capture_mode=KAL_FALSE;
	spin_unlock(&IMX219_drv_lock);
	VideoFullSizeSetting();
	iStartX += IMX219MIPI_IMAGE_SENSOR_VIDEO_STARTX;
	iStartY += IMX219MIPI_IMAGE_SENSOR_VIDEO_STARTY;
	spin_lock(&IMX219_drv_lock);	
	IMX219MIPI_sensor.cp_dummy_pixels = 0;
	IMX219MIPI_sensor.cp_dummy_lines = 0;
	IMX219MIPI_sensor.pv_dummy_pixels = 0;
	IMX219MIPI_sensor.pv_dummy_lines = 0;
	IMX219MIPI_sensor.video_dummy_pixels = 0;
	IMX219MIPI_sensor.video_dummy_lines = 0;
	IMX219MIPI_sensor.video_line_length = IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS+IMX219MIPI_sensor.video_dummy_pixels; 
	IMX219MIPI_sensor.video_frame_length = IMX219MIPI_VIDEO_FRAME_LENGTH_LINES+IMX219MIPI_sensor.video_dummy_lines;
	spin_unlock(&IMX219_drv_lock);
	
	IMX219MIPI_SetDummy(IMX219MIPI_sensor.video_dummy_pixels,IMX219MIPI_sensor.video_dummy_lines);
	IMX219MIPI_SetShutter(IMX219MIPI_sensor.video_shutter);
	spin_lock(&IMX219_drv_lock);	
	memcpy(&IMX219MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&IMX219_drv_lock);
	image_window->GrabStartX= iStartX;
	image_window->GrabStartY= iStartY;    
    SENSORDB("[IMX219MIPI]eXIT IMX219MIPIVideo function\n"); 
	return ERROR_NONE;
}	/* IMX219MIPIPreview() */

UINT32 IMX219MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
	kal_uint16 iStartX = 0, iStartY = 0;
	SENSORDB("[IMX219MIPI]enter IMX219MIPICapture function\n");
	if (IMX219MIPI_sensor.pv_mode == KAL_TRUE||IMX219MIPI_sensor.video_mode == KAL_TRUE) 
	{
		spin_lock(&IMX219_drv_lock);	
		IMX219MIPI_sensor.video_mode=KAL_FALSE;
		IMX219MIPI_sensor.pv_mode=KAL_FALSE;
		IMX219MIPI_sensor.capture_mode=KAL_TRUE;
		IMX219MIPI_MPEG4_encode_mode = KAL_FALSE; 
		//IMX219MIPI_Auto_Flicker_mode = KAL_FALSE;       
		IMX219MIPI_sensor.cp_dummy_pixels = 0;
		IMX219MIPI_sensor.cp_dummy_lines = 0;
		spin_unlock(&IMX219_drv_lock);
		IMX219MIPI_set_8M();
		//IMX219MIPISetFlipMirror(sensor_config_data->SensorImageMirror); 
		spin_lock(&IMX219_drv_lock);    
		IMX219MIPI_sensor.cp_line_length=IMX219MIPI_FULL_LINE_LENGTH_PIXELS+IMX219MIPI_sensor.cp_dummy_pixels;
		IMX219MIPI_sensor.cp_frame_length=IMX219MIPI_FULL_FRAME_LENGTH_LINES+IMX219MIPI_sensor.cp_dummy_lines;
		spin_unlock(&IMX219_drv_lock);
		iStartX = IMX219MIPI_IMAGE_SENSOR_CAP_STARTX;
		iStartY = IMX219MIPI_IMAGE_SENSOR_CAP_STARTY;
		image_window->GrabStartX=iStartX;
		image_window->GrabStartY=iStartY;
		image_window->ExposureWindowWidth=IMX219MIPI_REAL_CAP_WIDTH ;
		image_window->ExposureWindowHeight=IMX219MIPI_REAL_CAP_HEIGHT;
		IMX219MIPI_SetDummy(IMX219MIPI_sensor.cp_dummy_pixels, IMX219MIPI_sensor.cp_dummy_lines);   
		spin_lock(&IMX219_drv_lock);	
		memcpy(&IMX219MIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
		spin_unlock(&IMX219_drv_lock);
		if(test_pattern_flag)
		{
			IMX219MIPISetTestPatternMode(TRUE);
			test_pattern_flag=FALSE;
		}
	}
	SENSORDB("[IMX219MIPI]exit IMX219MIPICapture function\n");
	return ERROR_NONE;
}	/* IMX219MIPICapture() */

UINT32 IMX219MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    SENSORDB("[IMX219MIPI]eXIT IMX219MIPIGetResolution function\n");
    pSensorResolution->SensorPreviewWidth	= IMX219MIPI_REAL_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight	= IMX219MIPI_REAL_PV_HEIGHT;
    pSensorResolution->SensorFullWidth		= IMX219MIPI_REAL_CAP_WIDTH;
    pSensorResolution->SensorFullHeight		= IMX219MIPI_REAL_CAP_HEIGHT;
    pSensorResolution->SensorVideoWidth		= IMX219MIPI_REAL_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = IMX219MIPI_REAL_VIDEO_HEIGHT;
    SENSORDB("IMX219MIPIGetResolution :8-14");    
    return ERROR_NONE;
}   /* IMX219MIPIGetResolution() */

UINT32 IMX219MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{ 
	SENSORDB("[IMX219MIPI]enter IMX219MIPIGetInfo function\n");
	switch(ScenarioId){
			case MSDK_SCENARIO_ID_CAMERA_ZSD:
			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG://hhl 2-28
				pSensorInfo->SensorFullResolutionX=IMX219MIPI_REAL_CAP_WIDTH;
				pSensorInfo->SensorFullResolutionY=IMX219MIPI_REAL_CAP_HEIGHT;
				//pSensorInfo->SensorStillCaptureFrameRate=20;
				pSensorInfo->SensorStillCaptureFrameRate=IMX219MIPI_CAPTURE_MAX_FRAMERATE;

			break;//hhl 2-28
			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
				pSensorInfo->SensorPreviewResolutionX=IMX219MIPI_REAL_VIDEO_WIDTH;
				pSensorInfo->SensorPreviewResolutionY=IMX219MIPI_REAL_VIDEO_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=IMX219MIPI_PREVIEW_MAX_FRAMERATE;
			break;
		default:
        pSensorInfo->SensorPreviewResolutionX=IMX219MIPI_REAL_PV_WIDTH;
        pSensorInfo->SensorPreviewResolutionY=IMX219MIPI_REAL_PV_HEIGHT;
				pSensorInfo->SensorCameraPreviewFrameRate=IMX219MIPI_PREVIEW_MAX_FRAMERATE;
			break;
	}
    pSensorInfo->SensorVideoFrameRate=IMX219MIPI_VIDEO_MAX_FRAMERATE;	
    //pSensorInfo->SensorStillCaptureFrameRate=24;
    pSensorInfo->SensorStillCaptureFrameRate=IMX219MIPI_CAPTURE_MAX_FRAMERATE;
    //pSensorInfo->SensorWebCamCaptureFrameRate=24;
    pSensorInfo->SensorWebCamCaptureFrameRate=IMX219MIPI_WEB_CAM_CAPTURE_MAX_FRAMERATE;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=5;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW; /*??? */
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;
    pSensorInfo->MIPIsensorType=MIPI_OPHY_CSI2;
	
    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 2; 
    pSensorInfo->SensorMasterClockSwitch = 0; 
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;      
    pSensorInfo->AEShutDelayFrame = 0;//0		    /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 1;  //0   /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;	
	   
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX219MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX219MIPI_IMAGE_SENSOR_PV_STARTY;           		
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	     	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
	    	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
            pSensorInfo->SensorPacketECCOrder = 1;
            break;	
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			   pSensorInfo->SensorClockFreq=24;
			   pSensorInfo->SensorClockDividCount= 5;
			   pSensorInfo->SensorClockRisingCount= 0;
			   pSensorInfo->SensorClockFallingCount= 2;
			   pSensorInfo->SensorPixelClockCount= 3;
			   pSensorInfo->SensorDataLatchCount= 2;
			   pSensorInfo->SensorGrabStartX = IMX219MIPI_IMAGE_SENSOR_VIDEO_STARTX; 
			   pSensorInfo->SensorGrabStartY = IMX219MIPI_IMAGE_SENSOR_VIDEO_STARTY;				   
			   pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;		   
			   pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
			pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
			pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			   pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
			   pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
			   pSensorInfo->SensorPacketECCOrder = 1;

			break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
			//pSensorInfo->SensorClockFreq=12;
            pSensorInfo->SensorClockDividCount=	5;
            pSensorInfo->SensorClockRisingCount= 0;
            pSensorInfo->SensorClockFallingCount= 2;
            pSensorInfo->SensorPixelClockCount= 3;
            pSensorInfo->SensorDataLatchCount= 2;
            pSensorInfo->SensorGrabStartX = IMX219MIPI_IMAGE_SENSOR_CAP_STARTX;	//2*IMX219MIPI_IMAGE_SENSOR_PV_STARTX; 
            pSensorInfo->SensorGrabStartY = IMX219MIPI_IMAGE_SENSOR_CAP_STARTY;	//2*IMX219MIPI_IMAGE_SENSOR_PV_STARTY;          			
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;			
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 32; 
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0; 
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
			 pSensorInfo->SensorClockFreq=24;
			 pSensorInfo->SensorClockDividCount= 5;
			 pSensorInfo->SensorClockRisingCount= 0;
			 pSensorInfo->SensorClockFallingCount= 2;
			 pSensorInfo->SensorPixelClockCount= 3;
			 pSensorInfo->SensorDataLatchCount= 2;
			 pSensorInfo->SensorGrabStartX = IMX219MIPI_IMAGE_SENSOR_PV_STARTX; 
			 pSensorInfo->SensorGrabStartY = IMX219MIPI_IMAGE_SENSOR_PV_STARTY; 				 
			 pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_4_LANE;		 
			 pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
		     pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 32;//14; 
		  	 pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
			 pSensorInfo->SensorWidthSampling = 0;	// 0 is default 1x
			 pSensorInfo->SensorHightSampling = 0;	 // 0 is default 1x 
			 pSensorInfo->SensorPacketECCOrder = 1;

            break;
    }
	spin_lock(&IMX219_drv_lock);	
    IMX219MIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &IMX219MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	spin_unlock(&IMX219_drv_lock);
    SENSORDB("[IMX219MIPI]exit IMX219MIPIGetInfo function\n");
    return ERROR_NONE;
}   /* IMX219MIPIGetInfo() */


UINT32 IMX219MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{    
		spin_lock(&IMX219_drv_lock);	
		CurrentScenarioId = ScenarioId;
		spin_unlock(&IMX219_drv_lock);
		SENSORDB("[IMX219MIPI]enter IMX219MIPIControl function\n");
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            IMX219MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			IMX219MIPIVideo(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    case MSDK_SCENARIO_ID_CAMERA_ZSD:
            IMX219MIPICapture(pImageWindow, pSensorConfigData);//hhl 2-28
            break;
        default:
            return ERROR_INVALID_SCENARIO_ID;
    }
	SENSORDB("[IMX219MIPI]exit IMX219MIPIControl function\n");
	#if 1 //defined(SLT_DRV_W5520_CONFIG)
	video_flag = ScenarioId;
	#endif
    return ERROR_NONE;
} /* IMX219MIPIControl() */


//for auto-flicker fps
//int flag_framerate = 0;
UINT32 IMX219MIPISetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("[IMX219MIPISetVideoMode] frame rate = %d\n", u2FrameRate);
	kal_uint16 IMX219MIPI_Video_Max_Expourse_Time = 0;
	SENSORDB("[IMX219MIPI]%s():fix_frame_rate=%d\n",__FUNCTION__,u2FrameRate);
	spin_lock(&IMX219_drv_lock);
	IMX219MIPI_sensor.fix_video_fps = KAL_TRUE;
	spin_unlock(&IMX219_drv_lock);
	IMX219VIDEO_MODE_TARGET_FPS = u2FrameRate;
	u2FrameRate=u2FrameRate*10;//10*FPS
	SENSORDB("[IMX219MIPI][Enter Fix_fps func] IMX219MIPI_Fix_Video_Frame_Rate = %d\n", u2FrameRate/10);
	IMX219MIPI_Video_Max_Expourse_Time = (kal_uint16)((IMX219MIPI_sensor.video_pclk*10/u2FrameRate)/IMX219MIPI_sensor.video_line_length);
	
	if (IMX219MIPI_Video_Max_Expourse_Time > IMX219MIPI_VIDEO_FRAME_LENGTH_LINES/*IMX219MIPI_sensor.pv_frame_length*/) 
	{
		spin_lock(&IMX219_drv_lock);    
		IMX219MIPI_sensor.video_frame_length = IMX219MIPI_Video_Max_Expourse_Time;
		IMX219MIPI_sensor.video_dummy_lines = IMX219MIPI_sensor.video_frame_length-IMX219MIPI_VIDEO_FRAME_LENGTH_LINES;
		spin_unlock(&IMX219_drv_lock);
		SENSORDB("[IMX219MIPI]%s():frame_length=%d,dummy_lines=%d\n",__FUNCTION__,IMX219MIPI_sensor.video_frame_length,IMX219MIPI_sensor.video_dummy_lines);
		IMX219MIPI_SetDummy(IMX219MIPI_sensor.video_dummy_pixels,IMX219MIPI_sensor.video_dummy_lines);
	}
	spin_lock(&IMX219_drv_lock);    
	IMX219MIPI_MPEG4_encode_mode = KAL_TRUE; 
	spin_unlock(&IMX219_drv_lock);
	SENSORDB("[IMX219MIPI]exit IMX219MIPISetVideoMode function\n");
	return ERROR_NONE;
}

UINT32 IMX219MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{

    return ERROR_NONE;
}
UINT32 IMX219MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) {
	kal_uint32 pclk;
	kal_int16 dummyLine;
	kal_uint16 lineLength,frameHeight;	
	SENSORDB("IMX219MIPISetMaxFramerateByScenario: scenarioId = %d, frame rate = %d\n",scenarioId,frameRate);
	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
			pclk = IMX219MIPI_PREVIEW_CLK;
			lineLength = IMX219MIPI_PV_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX219MIPI_PV_FRAME_LENGTH_LINES;

			
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&IMX219_drv_lock);	
			IMX219MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&IMX219_drv_lock);
			IMX219MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			pclk = IMX219MIPI_VIDEO_CLK;
			lineLength = IMX219MIPI_VIDEO_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX219MIPI_VIDEO_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			spin_lock(&IMX219_drv_lock);	
			IMX219MIPI_sensor.pv_mode=TRUE;
			spin_unlock(&IMX219_drv_lock);
			IMX219MIPI_SetDummy(0, dummyLine);			
			break;			
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:			
			pclk = IMX219MIPI_CAPTURE_CLK;
			lineLength = IMX219MIPI_FULL_LINE_LENGTH_PIXELS;
			frameHeight = (10 * pclk)/frameRate/lineLength;
			dummyLine = frameHeight - IMX219MIPI_FULL_FRAME_LENGTH_LINES;
			if(dummyLine<0)
				dummyLine = 0;
			
			spin_lock(&IMX219_drv_lock);	
			IMX219MIPI_sensor.pv_mode=FALSE;
			spin_unlock(&IMX219_drv_lock);
			IMX219MIPI_SetDummy(0, dummyLine);			
			break;		
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
			break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			break;		
		default:
			break;
	}
	SENSORDB("[IMX219MIPI]exit IMX219MIPISetMaxFramerateByScenario function\n");
	return ERROR_NONE;
}
UINT32 IMX219MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

	switch (scenarioId) {
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
			 *pframeRate = IMX219MIPI_PREVIEW_MAX_FRAMERATE * 10;
			 break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_ZSD:
			 *pframeRate = IMX219MIPI_CAPTURE_MAX_FRAMERATE * 10;
			break;		//hhl 2-28
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
			 *pframeRate = IMX219MIPI_3D_PREVIEW_MAX_FRAMERATE * 10;
			break;		
		default:
			break;
	}

	return ERROR_NONE;
}
UINT32 IMX219MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT	*pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=IMX219MIPI_REAL_CAP_WIDTH;
            *pFeatureReturnPara16=IMX219MIPI_REAL_CAP_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        		    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
 		            *pFeatureReturnPara16++=IMX219MIPI_sensor.cp_line_length;  
 		            *pFeatureReturnPara16=IMX219MIPI_sensor.cp_frame_length;
		            SENSORDB("Sensor period:%d %d\n",IMX219MIPI_sensor.cp_line_length, IMX219MIPI_sensor.cp_frame_length); 
		            *pFeatureParaLen=4;        				
        				break;
        			case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
					*pFeatureReturnPara16++=IMX219MIPI_sensor.video_line_length;  
					*pFeatureReturnPara16=IMX219MIPI_sensor.video_frame_length;
					 SENSORDB("Sensor period:%d %d\n", IMX219MIPI_sensor.video_line_length, IMX219MIPI_sensor.video_frame_length); 
					 *pFeatureParaLen=4;
						break;
        			default:	
					*pFeatureReturnPara16++=IMX219MIPI_sensor.pv_line_length;  
					*pFeatureReturnPara16=IMX219MIPI_sensor.pv_frame_length;
		            SENSORDB("Sensor period:%d %d\n", IMX219MIPI_sensor.pv_line_length, IMX219MIPI_sensor.pv_frame_length); 
		            *pFeatureParaLen=4;
	            break;
          	}
          	break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        		switch(CurrentScenarioId)
        		{
        			case MSDK_SCENARIO_ID_CAMERA_ZSD:
        			case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		            *pFeatureReturnPara32 = IMX219MIPI_sensor.cp_pclk; 
		            *pFeatureParaLen=4;		         	
					
		            SENSORDB("Sensor CPCLK:%dn",IMX219MIPI_sensor.cp_pclk); 
		         		break; //hhl 2-28
					case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
						*pFeatureReturnPara32 = IMX219MIPI_sensor.video_pclk;
						*pFeatureParaLen=4;
						SENSORDB("Sensor videoCLK:%d\n",IMX219MIPI_sensor.video_pclk); 
						break;
		         		default:
		            *pFeatureReturnPara32 = IMX219MIPI_sensor.pv_pclk;
		            *pFeatureParaLen=4;
					SENSORDB("Sensor pvclk:%d\n",IMX219MIPI_sensor.pv_pclk); 
		            break;
		         }
		         break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            IMX219MIPI_SetShutter(*pFeatureData16); 
            break;
		case SENSOR_FEATURE_SET_SENSOR_SYNC: 
			break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            IMX219MIPI_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
           IMX219MIPI_SetGain((UINT16) *pFeatureData16); 
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			spin_lock(&IMX219_drv_lock);    
            IMX219MIPI_isp_master_clock=*pFeatureData32;
			spin_unlock(&IMX219_drv_lock);
            break;
        case SENSOR_FEATURE_SET_REGISTER:
			//IMX219MIPI_REAL_CAP_WIDTH =				pSensorRegData->RegAddr;//3280
            //IMX219MIPI_REAL_CAP_HEIGHT	=			pSensorRegData->RegData;//2464
            SENSORDB("xb.pang---w:%d, h:%d\n",IMX219MIPI_REAL_CAP_WIDTH, IMX219MIPI_REAL_CAP_HEIGHT);
			IMX219MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = IMX219MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)  
            {
            	spin_lock(&IMX219_drv_lock);    
                IMX219MIPISensorCCT[i].Addr=*pFeatureData32++;
                IMX219MIPISensorCCT[i].Para=*pFeatureData32++; 
				spin_unlock(&IMX219_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX219MIPISensorCCT[i].Addr;
                *pFeatureData32++=IMX219MIPISensorCCT[i].Para; 
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {	spin_lock(&IMX219_drv_lock);    
                IMX219MIPISensorReg[i].Addr=*pFeatureData32++;
                IMX219MIPISensorReg[i].Para=*pFeatureData32++;
				spin_unlock(&IMX219_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=IMX219MIPISensorReg[i].Addr;
                *pFeatureData32++=IMX219MIPISensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=IMX219MIPI_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, IMX219MIPISensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, IMX219MIPISensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &IMX219MIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            IMX219MIPI_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            IMX219MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=IMX219MIPI_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            IMX219MIPI_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            IMX219MIPI_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            IMX219MIPI_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
            pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_B;
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            IMX219MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            IMX219MIPIGetSensorID(pFeatureReturnPara32); 
            break;             
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            IMX219MIPISetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));            
	        break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            IMX219MIPISetTestPatternMode((BOOL)*pFeatureData16);        	
            break;
		case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE:
			*pFeatureReturnPara32 = IMX219_TEST_PATTERN_CHECKSUM;
			*pFeatureParaLen=4;
			break;
		case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
			IMX219MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
			IMX219MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
			break;
        default:
            break;
    }
    return ERROR_NONE;
}	/* IMX219MIPIFeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncIMX219MIPI=
{
    IMX219MIPIOpen,
    IMX219MIPIGetInfo,
    IMX219MIPIGetResolution,
    IMX219MIPIFeatureControl,
    IMX219MIPIControl,
    IMX219MIPIClose
};

UINT32 IMX219_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncIMX219MIPI;
    return ERROR_NONE;
}   /* SensorInit() */

