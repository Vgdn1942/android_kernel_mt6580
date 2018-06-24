#ifdef IAP_PORTION
const u8 huaruichuan_fw[]=
{
//#include "tinno_MTK_L5220VC406.i"
//#include "TINO_5030_4402.i"
//#include "TINO_5030_C403.i"
//#include "TINO_5030_C406.i"
//#include "S5030_Hua_ID_5030_Ver_C408.i"
//#include "S5030_Hua_ID_5030_Ver_C409.i"
#include "S5030_Hua_ID_5030_Ver_C40a.i"
};

struct vendor_map
{
	int vendor_id;
	char vendor_name[30];
	uint8_t* fw_array;
};
const struct vendor_map g_vendor_map[]=
{
	{0x2ae1,"HUARUIC",huaruichuan_fw}
};

#endif/*IAP_PORTION*/
