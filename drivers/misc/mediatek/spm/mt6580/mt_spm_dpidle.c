#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <linux/lockdep.h>

#include <mach/irqs.h>
#include <mach/mt_cirq.h>
#include <mach/mt_spm_idle.h>
#include <mach/mt_cpuidle.h>
#include <mach/wd_api.h>
#include <mach/mt_gpt.h>
#include <mach/mt_cpufreq.h>
#include <mach/mt_ccci_common.h>
#include <mach/md32_helper.h>

#include "mt_spm_internal.h"


/**************************************
 * only for internal debug
 **************************************/
//FIXME: for FPGA early porting
#define  CONFIG_MTK_LDVT

#ifdef CONFIG_MTK_LDVT
#define SPM_PWAKE_EN            0
#define SPM_BYPASS_SYSPWREQ     1
#else
#define SPM_PWAKE_EN            1
#define SPM_BYPASS_SYSPWREQ     0
#endif

#if 0
#define WAKE_SRC_FOR_DPIDLE  0
#else
#define WAKE_SRC_FOR_DPIDLE                                                              \
    (WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT| \
     WAKE_SRC_CCIF0_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD| WAKE_SRC_USB_PDN| \
     WAKE_SRC_AFE | WAKE_SRC_SYSPWREQ | WAKE_SRC_MD1_WDT | WAKE_SRC_SEJ)
#endif     

#define WAKE_SRC_FOR_MD32  0

#define I2C_CHANNEL 2

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

#ifdef CONFIG_MTK_RAM_CONSOLE
#define SPM_AEE_RR_REC 1
#else
#define SPM_AEE_RR_REC 0
#endif
#define SPM_USE_TWAM_DEBUG	0

#if SPM_AEE_RR_REC
enum spm_deepidle_step
{
	SPM_DEEPIDLE_ENTER=0,
	SPM_DEEPIDLE_ENTER_UART_SLEEP,
	SPM_DEEPIDLE_ENTER_WFI,
	SPM_DEEPIDLE_LEAVE_WFI,
	SPM_DEEPIDLE_ENTER_UART_AWAKE,
	SPM_DEEPIDLE_LEAVE
};
#endif

// please put firmware to vendor/mediatek/proprietary/hardware/spm/mtxxxx/
#define USE_DYNA_LOAD_DPIDLE
#ifndef USE_DYNA_LOAD_DPIDLE
/**********************************************************
 * PCM code for deep idle
 **********************************************************/ 
static const u32 dpidle_binary[] = {
	0x80318400, 0x80328400, 0xc2802e80, 0x1290041f, 0x1b00001f, 0x7fffd7ff,
	0xf0000000, 0x17c07c1f, 0x1b00001f, 0x3fffc7ff, 0x1b80001f, 0x20000004,
	0xd80002cc, 0x17c07c1f, 0xa0128400, 0xa0118400, 0xc2802e80, 0x1290841f,
	0xc2802e80, 0x129f041f, 0x1b00001f, 0x3fffcfff, 0xf0000000, 0x17c07c1f,
	0x81449801, 0xd8000405, 0x17c07c1f, 0x1a00001f, 0x10006604, 0xe2200000,
	0xc0c02da0, 0x17c07c1f, 0x81491801, 0xd8000565, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0x813f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x81411801, 0xd8000685, 0x17c07c1f, 0x18c0001f, 0x10006240,
	0xe0e00016, 0xe0e0001e, 0xe0e0000e, 0xe0e0000f, 0x803e0400, 0x1b80001f,
	0x20000222, 0x80380400, 0x1b80001f, 0x20000280, 0x803b0400, 0x1b80001f,
	0x2000001a, 0x803d0400, 0x1b80001f, 0x20000208, 0x80340400, 0x80310400,
	0x1b80001f, 0x2000000a, 0x18c0001f, 0x10006240, 0xe0e0000d, 0xd8000ca5,
	0x17c07c1f, 0x1b80001f, 0x20000020, 0x18c0001f, 0x102080f0, 0x1910001f,
	0x102080f0, 0xa9000004, 0x10000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xefffffff, 0xe0c00004, 0x18c0001f, 0x102070f4, 0x1910001f,
	0x102070f4, 0xa9000004, 0x02000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xfdffffff, 0xe0c00004, 0x1910001f, 0x102070f4, 0x81fa0407,
	0x81f08407, 0xe8208000, 0x10006354, 0xfffffc23, 0xa1d80407, 0xa1df0407,
	0xc2802e80, 0x1291041f, 0x81491801, 0xd8000f25, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x1b00001f, 0xbfffc7ff, 0xf0000000, 0x17c07c1f, 0x1b80001f,
	0x20000fdf, 0x1a50001f, 0x10006608, 0x80c9a401, 0x810ba401, 0x10920c1f,
	0xa0979002, 0x8080080d, 0xd82012a2, 0x17c07c1f, 0x81f08407, 0xa1d80407,
	0xa1df0407, 0x1b00001f, 0x3fffc7ff, 0x1b80001f, 0x20000004, 0xd8001b4c,
	0x17c07c1f, 0x1b00001f, 0xbfffc7ff, 0xd0001b40, 0x17c07c1f, 0x81f80407,
	0x81ff0407, 0x1880001f, 0x10006320, 0xc0c02640, 0xe080000f, 0xd8001103,
	0x17c07c1f, 0xe080001f, 0xa1da0407, 0x18c0001f, 0x110040d8, 0x1910001f,
	0x110040d8, 0xa11f8404, 0xe0c00004, 0x1910001f, 0x110040d8, 0x81491801,
	0xd8001645, 0x17c07c1f, 0x18c0001f, 0x102085cc, 0x1910001f, 0x102085cc,
	0x813f8404, 0xe0c00004, 0x1910001f, 0x102085cc, 0xa0110400, 0xa0140400,
	0xa0180400, 0xa01b0400, 0xa01d0400, 0x17c07c1f, 0x17c07c1f, 0xa01e0400,
	0x17c07c1f, 0x17c07c1f, 0x81491801, 0xd80018e5, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x81411801, 0xd80019c5, 0x17c07c1f, 0x18c0001f, 0x10006240,
	0xc0c025a0, 0x17c07c1f, 0x81449801, 0xd8001ac5, 0x17c07c1f, 0x1a00001f,
	0x10006604, 0xe2200001, 0xc0c02da0, 0x17c07c1f, 0xc2802e80, 0x1291841f,
	0x1b00001f, 0x7fffd7ff, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x10006b6c,
	0xe0f03eef, 0x80318400, 0x80328400, 0xa1de0407, 0x80390400, 0x1b80001f,
	0x20000300, 0x803c0400, 0x1b80001f, 0x20000300, 0x80350400, 0x1b80001f,
	0x20000104, 0x81f40407, 0x18c0001f, 0x111403a8, 0xe0c00001, 0x18c0001f,
	0x10006810, 0x1910001f, 0x10006810, 0x81308404, 0xe0c00004, 0x1b00001f,
	0x7fffcfff, 0xf0000000, 0x17c07c1f, 0x81fe0407, 0x18c0001f, 0x10006810,
	0x1910001f, 0x10006810, 0xa1108404, 0xe0c00004, 0x18c0001f, 0x10006b6c,
	0xe0f05ead, 0x1b00001f, 0xffffcfff, 0x80dd040d, 0xd82020a3, 0x17c07c1f,
	0xc0c02ba0, 0x17c07c1f, 0xa0150400, 0xa01c0400, 0xa0190400, 0xf0000000,
	0x17c07c1f, 0xe0f07f16, 0x1380201f, 0xe0f07f1e, 0x1380201f, 0xe0f07f0e,
	0x1b80001f, 0x20000100, 0xe0f07f0c, 0xe0f07f0d, 0xe0f07e0d, 0xe0f07c0d,
	0xe0f0780d, 0xe0f0700d, 0xf0000000, 0x17c07c1f, 0xe0f07f0d, 0xe0f07f0f,
	0xe0f07f1e, 0xe0f07f12, 0xf0000000, 0x17c07c1f, 0xe0e00016, 0x1380201f,
	0xe0e0001e, 0x1380201f, 0xe0e0000e, 0xe0e0000c, 0xe0e0000d, 0xf0000000,
	0x17c07c1f, 0xe0e0000f, 0xe0e0001e, 0xe0e00012, 0xf0000000, 0x17c07c1f,
	0x1112841f, 0xa1d08407, 0xd8202704, 0x80eab401, 0xd8002683, 0x01200404,
	0x1a00001f, 0x10006814, 0xe2000003, 0xf0000000, 0x17c07c1f, 0xa1d00407,
	0x1b80001f, 0x20000100, 0x80ea3401, 0x1a00001f, 0x10006814, 0xe2000003,
	0xf0000000, 0x17c07c1f, 0xd800298a, 0x17c07c1f, 0xe2e00036, 0xe2e0003e,
	0x1380201f, 0xe2e0003c, 0xd8202aca, 0x17c07c1f, 0x1b80001f, 0x20000018,
	0xe2e0007c, 0x1b80001f, 0x20000003, 0xe2e0005c, 0xe2e0004c, 0xe2e0004d,
	0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000,
	0x17c07c1f, 0xa1d40407, 0x1391841f, 0xf0000000, 0x17c07c1f, 0xd8002cca,
	0x17c07c1f, 0xe2e0004f, 0xe2e0006f, 0xe2e0002f, 0xd8202d6a, 0x17c07c1f,
	0xe2e0002e, 0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f, 0x18d0001f,
	0x10006604, 0x10cf8c1f, 0xd8202da3, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0x18c0001f, 0x10006b18, 0x1910001f, 0x10006b18, 0xa1002804, 0xe0c00004,
	0xf0000000, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0x1990001f,
	0x10006b08, 0xe8208000, 0x10006b18, 0x00000000, 0x1b00001f, 0x3fffc7ff,
	0x1b80001f, 0xd00f0000, 0x8880000c, 0x3fffc7ff, 0xd8005522, 0x17c07c1f,
	0xe8208000, 0x10006354, 0xfffffc23, 0xc0c02b00, 0x81401801, 0xd80046c5,
	0x17c07c1f, 0x81f60407, 0x18c0001f, 0x10006200, 0xc0c02c20, 0x12807c1f,
	0xe8208000, 0x1000625c, 0x00000001, 0x1b80001f, 0x20000080, 0xc0c02c20,
	0x1280041f, 0x18c0001f, 0x10006208, 0xc0c02c20, 0x12807c1f, 0xe8208000,
	0x10006248, 0x00000000, 0x1b80001f, 0x20000080, 0xc0c02c20, 0x1280041f,
	0x18c0001f, 0x10006290, 0xc0c02c20, 0x12807c1f, 0xc0c02c20, 0x1280041f,
	0xc2802e80, 0x1292041f, 0x18c0001f, 0x10006294, 0xe0f07fff, 0xe0e00fff,
	0xe0e000ff, 0xc0c02ba0, 0x17c07c1f, 0xa1d38407, 0xa1d98407, 0x18c0001f,
	0x11004078, 0x1910001f, 0x11004078, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x11004078, 0x18c0001f, 0x11004098, 0x1910001f, 0x11004098, 0xa11f8404,
	0xe0c00004, 0x1910001f, 0x11004098, 0xa0108400, 0xa0120400, 0xa0148400,
	0xa0150400, 0xa0158400, 0xa01b8400, 0xa01c0400, 0xa01c8400, 0xa0188400,
	0xa0190400, 0xa0198400, 0xe8208000, 0x10006310, 0x0b1600f8, 0x1b00001f,
	0xbfffc7ff, 0x1b80001f, 0x90100000, 0x80c18001, 0xc8c00003, 0x17c07c1f,
	0x80c10001, 0xc8c00303, 0x17c07c1f, 0x1b00001f, 0x3fffc7ff, 0x18c0001f,
	0x10006294, 0xe0e001fe, 0xe0e003fc, 0xe0e007f8, 0xe0e00ff0, 0x1b80001f,
	0x20000020, 0xe0f07ff0, 0xe0f07f00, 0x80388400, 0x80390400, 0x80398400,
	0x1b80001f, 0x20000300, 0x803b8400, 0x803c0400, 0x803c8400, 0x1b80001f,
	0x20000300, 0x80348400, 0x80350400, 0x80358400, 0x1b80001f, 0x20000104,
	0x80308400, 0x80320400, 0x81f38407, 0x81f98407, 0x81f40407, 0x81401801,
	0xd8005525, 0x17c07c1f, 0x18c0001f, 0x10006290, 0x1212841f, 0xc0c028c0,
	0x12807c1f, 0xc0c028c0, 0x1280041f, 0x18c0001f, 0x10006208, 0x1212841f,
	0xc0c028c0, 0x12807c1f, 0xe8208000, 0x10006248, 0x00000001, 0x1b80001f,
	0x20000080, 0xc0c028c0, 0x1280041f, 0x18c0001f, 0x10006200, 0x1212841f,
	0xc0c028c0, 0x12807c1f, 0xe8208000, 0x1000625c, 0x00000000, 0x1b80001f,
	0x20000080, 0xc0c028c0, 0x1280041f, 0x81f10407, 0x81f48407, 0xa1d60407,
	0x1ac0001f, 0x55aa55aa, 0x10007c1f, 0xf0000000
};
static struct pcm_desc dpidle_pcm = {
	.version	= "pcm_deepidle_v14.0_new",
	.base		= dpidle_binary,
	.size		= 688,
	.sess		= 2,
	.replace	= 0,
	.vec0		= EVENT_VEC(11, 1, 0, 0),	/* FUNC_26M_WAKEUP */
	.vec1		= EVENT_VEC(12, 1, 0, 8),	/* FUNC_26M_SLEEP */
	.vec2		= EVENT_VEC(30, 1, 0, 24),	/* FUNC_APSRC_WAKEUP */
	.vec3		= EVENT_VEC(31, 1, 0, 125),	/* FUNC_APSRC_SLEEP */
	.vec4		= EVENT_VEC(20, 1, 0, 220),	/* FUNC_AUDIO_REQ_WAKEUP */
	.vec5		= EVENT_VEC(1, 1, 0, 249),	/* FUNC_AUDIO_REQ_SLEEP */
};
#endif
 
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src		= WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32		= WAKE_SRC_FOR_MD32,
	.r0_ctrl_en		= 1,
	.r7_ctrl_en		= 1,
	.infra_dcm_lock		= 1,
	.wfi_op			= WFI_OP_AND,
	.ca15_wfi0_en		= 1,
	.ca15_wfi1_en		= 1,
	.ca15_wfi2_en		= 1,
	.ca15_wfi3_en		= 1,
	.ca7_wfi0_en		= 1,
	.ca7_wfi1_en		= 1,
	.ca7_wfi2_en		= 1,
	.ca7_wfi3_en		= 1,
	.disp_req_mask		= 1,
	.mfg_req_mask		= 1,
	.lte_mask		= 1,
	.syspwreq_mask		= 1,
};

struct spm_lp_scen __spm_dpidle = {
#ifndef USE_DYNA_LOAD_DPIDLE
	.pcmdesc	= &dpidle_pcm,
#endif
	.pwrctrl	= &dpidle_ctrl,
};

extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern void mt_irq_unmask_for_sleep(unsigned int irq);
extern int request_uart_to_sleep(void);
extern int request_uart_to_wakeup(void);
extern unsigned int mt_get_clk_mem_sel(void);
extern int is_ext_buck_exist(void);


#if SPM_AEE_RR_REC
extern void aee_rr_rec_deepidle_val(u32 val);
extern u32 aee_rr_curr_deepidle_val(void);
#endif

//FIXME: Denali early porting
#if 0
void __attribute__((weak))
mt_cirq_clone_gic(void)
{
	return;//FIXME: K2 early porting
}
void __attribute__((weak))
mt_cirq_enable(void)
{
	return;//FIXME: K2 early porting
}
void __attribute__((weak))
mt_cirq_flush(void)
{
	return;//FIXME: K2 early porting
}

void __attribute__((weak))
mt_cirq_disable(void)
{
	return;//FIXME: K2 early porting
}
#endif

static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
    if (is_cpu_pdn(pwrctrl->pcm_flags)) {
        mt_cpu_dormant(CPU_DEEPIDLE_MODE);
    } else { 
        wfi_with_sync();	
    }
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
    unsigned long flags;

    if (spm_is_wakesrc_invalid(wakesrc))
        return -EINVAL;

    spin_lock_irqsave(&__spm_lock, flags);
    if (enable) {
        if (replace)
            __spm_dpidle.pwrctrl->wake_src = wakesrc;
        else
            __spm_dpidle.pwrctrl->wake_src |= wakesrc;
    } else {
        if (replace)
            __spm_dpidle.pwrctrl->wake_src = 0;
        else
            __spm_dpidle.pwrctrl->wake_src &= ~wakesrc;
    }
    spin_unlock_irqrestore(&__spm_lock, flags);

    return 0;
}

extern void spm_i2c_control(u32 channel, bool onoff);
U32 g_bus_ctrl = 0;
U32 g_sys_ck_sel = 0;
static void spm_dpidle_pre_process(void)
{
    //spm_i2c_control(I2C_CHANNEL, 1);

    /* set PMIC WRAP table for deepidle power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);  
}

static void spm_dpidle_post_process(void)
{
/*   //TODO
    spm_i2c_control(I2C_CHANNEL, 0);
*/
    /* set PMIC WRAP table for normal power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);     
}
enum mempll_type{
    MEMP26MHZ		= 0,
    MEMPLL3PLL		= 1,
    MEMPLL1PLL		= 2,
};
#ifdef CONFIG_OF
static int dt_scan_memory(unsigned long node, const char *uname, int depth, void *data)
{
	char *type = of_get_flat_dt_prop(node, "device_type", NULL);
	__be32 *reg, *endp;
	unsigned long l;

	/* We are scanning "memory" nodes only */
	if (type == NULL) {
		/*
		 * The longtrail doesn't have a device_type on the
		 * /memory node, so look for the node called /memory@0.
		 */
		if (depth != 1 || strcmp(uname, "memory@0") != 0)
			return 0;
	} else if (strcmp(type, "memory") != 0)
		return 0;

		reg = of_get_flat_dt_prop(node, "reg", &l);
	if (reg == NULL)
		return 0;

	endp = reg + (l / sizeof(__be32));
	*(unsigned long*)data = node;
	return node;
}
#endif

extern u32 slp_spm_deepidle_flags;
bool spm_set_dpidle_pcm_init_flag(void)
{
    return true;
}

#ifdef SPM_DEEPIDLE_PROFILE_TIME
extern unsigned int dpidle_profile[4];
#endif
unsigned long pcm_timer_seed = 0;
wake_reason_t spm_go_to_dpidle(u32 spm_flags, u32 spm_data)
{
    struct wake_status wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    wake_reason_t wr = WR_NONE;
#ifndef USE_DYNA_LOAD_DPIDLE
    struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc;
#else
    struct pcm_desc *pcmdesc;
#endif
    struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

#ifdef USE_DYNA_LOAD_DPIDLE
    if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
	    pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
    else
	    BUG();
#endif

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(1<<SPM_DEEPIDLE_ENTER);
#endif 

#if 0
    pwrctrl->timer_val_cust = pcm_timer_seed;
    pcm_timer_seed++;
    if(pcm_timer_seed>=300)
        pcm_timer_seed=0;
#endif

    set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

    spm_dpidle_before_wfi();

    lockdep_off();
    spin_lock_irqsave(&__spm_lock, flags);
    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val()|(1<<SPM_DEEPIDLE_ENTER_UART_SLEEP));
#endif     

    if (request_uart_to_sleep()) {
        wr = WR_UART_BUSY;
        goto RESTORE_IRQ;
    }

    __spm_reset_and_init_pcm(pcmdesc);

    __spm_kick_im_to_fetch(pcmdesc);
	
    __spm_init_pcm_register();

    __spm_init_event_vector(pcmdesc);

    __spm_set_power_control(pwrctrl);

    __spm_set_wakeup_event(pwrctrl);

    spm_dpidle_pre_process();    

    __spm_kick_pcm_to_run(pwrctrl);

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val()|(1<<SPM_DEEPIDLE_ENTER_WFI));
#endif

#ifdef SPM_DEEPIDLE_PROFILE_TIME
    gpt_get_cnt(SPM_PROFILE_APXGPT,&dpidle_profile[1]);
#endif
    spm_trigger_wfi_for_dpidle(pwrctrl);
#ifdef SPM_DEEPIDLE_PROFILE_TIME
    gpt_get_cnt(SPM_PROFILE_APXGPT,&dpidle_profile[2]);
#endif

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val()|(1<<SPM_DEEPIDLE_LEAVE_WFI));
#endif 

    spm_dpidle_post_process();

    __spm_get_wakeup_status(&wakesta);

    __spm_clean_after_wakeup();

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val()|(1<<SPM_DEEPIDLE_ENTER_UART_AWAKE));
#endif

#if 1
    request_uart_to_wakeup();
#endif
    wr = __spm_output_wake_reason(&wakesta, pcmdesc, false);

RESTORE_IRQ:
    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);
    spin_unlock_irqrestore(&__spm_lock, flags);
    lockdep_on();
    spm_dpidle_after_wfi();

#if SPM_AEE_RR_REC
    aee_rr_rec_deepidle_val(0);
#endif 
    return wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 * pwake_time:
 *    >= 0  = specific wakeup period
 */
wake_reason_t spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data)
{
    u32 sec = 0;
    u32 dpidle_timer_val = 0;
    u32 dpidle_wake_src = 0;
    int wd_ret;
    struct wake_status wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    struct wd_api *wd_api;
    static wake_reason_t last_wr = WR_NONE;
#ifndef USE_DYNA_LOAD_DPIDLE
    struct pcm_desc *pcmdesc = __spm_dpidle.pcmdesc;
#else
    struct pcm_desc *pcmdesc;
#endif
    struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

#ifdef USE_DYNA_LOAD_DPIDLE
    if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
	    pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
    else
	    BUG();
#endif

	/* backup original dpidle setting */
	dpidle_timer_val = pwrctrl->timer_val;
	dpidle_wake_src = pwrctrl->wake_src;

    set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

    spm_dpidle_before_wfi();

#if SPM_PWAKE_EN
    sec = spm_get_wake_period(-1 /* FIXME */, last_wr);
#endif
    pwrctrl->timer_val = sec * 32768;

    pwrctrl->wake_src = spm_get_sleep_wakesrc();

    wd_ret = get_wd_api(&wd_api);
    if (!wd_ret)
        wd_api->wd_suspend_notify();

    spin_lock_irqsave(&__spm_lock, flags);
    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();
    /* set PMIC WRAP table for deepidle power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);

    spm_crit2("sleep_deepidle, sec = %u, wakesrc = 0x%x [%u]\n",
              sec, pwrctrl->wake_src, is_cpu_pdn(pwrctrl->pcm_flags));

    __spm_reset_and_init_pcm(pcmdesc);

    __spm_kick_im_to_fetch(pcmdesc);

    if (request_uart_to_sleep()) {
        last_wr = WR_UART_BUSY;
        goto RESTORE_IRQ;
    }

    __spm_init_pcm_register();

    __spm_init_event_vector(pcmdesc);

    __spm_set_power_control(pwrctrl);

    __spm_set_wakeup_event(pwrctrl);

    __spm_kick_pcm_to_run(pwrctrl);

    spm_dpidle_pre_process();

    spm_trigger_wfi_for_dpidle(pwrctrl);

    spm_dpidle_post_process();    

    __spm_get_wakeup_status(&wakesta);

    __spm_clean_after_wakeup();

    request_uart_to_wakeup();

    last_wr = __spm_output_wake_reason(&wakesta, pcmdesc, true);

RESTORE_IRQ:
    /* set PMIC WRAP table for normal power control */
    mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);
    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);

    spin_unlock_irqrestore(&__spm_lock, flags);

    if (!wd_ret)
        wd_api->wd_resume_notify();

    spm_dpidle_after_wfi();

    /* restore original dpidle setting */
    pwrctrl->timer_val = dpidle_timer_val;
    pwrctrl->wake_src = dpidle_wake_src;

    return last_wr;  
}

#if SPM_USE_TWAM_DEBUG
#define SPM_TWAM_MONITOR_TICK 333333


static void twam_handler(struct twam_sig *twamsig)
{
	spm_crit("sig_high = %u%%  %u%%  %u%%  %u%%, r13 = 0x%x\n",
		 get_percent(twamsig->sig0,SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig1,SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig2,SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig3,SPM_TWAM_MONITOR_TICK),
		 spm_read(SPM_PCM_REG13_DATA));
}
#endif

void spm_deepidle_init(void)
{
#if SPM_USE_TWAM_DEBUG
	unsigned long flags;
	struct twam_sig twamsig = {
		.sig0 = 26,	/* md1_srcclkena */
		.sig1 = 22,	/* md_apsrc_req_mux */
		.sig2 = 25,	/* md2_srcclkena */
		.sig3 = 21,	/* md2_apsrc_req_mux */		
		//.sig2 = 23,	/* conn_srcclkena */
		//.sig3 = 20,	/* conn_apsrc_req */
	};
#if 0
	spin_lock_irqsave(&__spm_lock, flags);
	spm_write(SPM_AP_STANBY_CON, spm_read(SPM_AP_STANBY_CON) | ASC_MD_DDR_EN_SEL);
	spin_unlock_irqrestore(&__spm_lock, flags);
#endif
	spm_twam_register_handler(twam_handler);
	spm_twam_enable_monitor(&twamsig, false,SPM_TWAM_MONITOR_TICK);
#endif
}


MODULE_DESCRIPTION("SPM-DPIdle Driver v0.1");
