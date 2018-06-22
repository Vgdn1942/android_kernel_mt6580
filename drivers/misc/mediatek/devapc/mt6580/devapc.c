#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/xlog.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/types.h>
#ifdef CONFIG_MTK_HIBERNATION
#include <mach/mtk_hibernate_dpm.h>
#endif
#include "mach/mt_clkmgr.h"
#include "mach/mt_reg_base.h"
#include "mach/mt_device_apc.h"
#include "mach/mt_typedefs.h"
#include "mach/sync_write.h"
#include "mach/irqs.h"
#include "devapc.h"

static struct cdev *g_devapc_ctrl = NULL;
static unsigned int devapc_irq = 0;
static void __iomem *devapc_ao_base;
static void __iomem *devapc_pd_base;

static DEVICE_INFO devapc_devices[] = {
    
    {"INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR",           TRUE},  /* 0 */
    {"INFRA_AO_INFRASYS_CONFIG_REGS",                TRUE},
    {"INFRA_AO_KPAD_CONTROL_REG",                    TRUE},
    {"INFRA_AO_PERISYS_CONFIG_REGS",                 TRUE},
    {"INFRA_AO _RSVD_0",                             TRUE},
    {"INFRA_AO_GPIO_CONTROLLER",                     FALSE},
    {"INFRA_AO_TOP_LEVEL_SLP_MANAGER",               TRUE},
    {"INFRA_AO_TOP_LEVEL_RESET_GENERATOR",           TRUE},
    {"INFRA_AO_GPT",                                 TRUE},
    {"INFRASYS_EFUSEC",                              FALSE},
    
    {"INFRA_AO_SEJ",                                 TRUE},  /*10*/
    {"INFRA_AO_APMCU_EINT_CONTROLLER",               TRUE}, 
    {"INFRA_AO_ AP_CCIF",                            FALSE},
    {"INFRA_AO_ MD_CCIF",                            FALSE},
    {"INFRA_AO_AES_TOP",                             TRUE},
    {"INFRA_AO_PMIC_WRAP_CONTROL_REG",               FALSE},
    {"INFRA_AO_DEVICE_APC_AO",                       TRUE},
    {"MIPI_CONFIG_REG",                              TRUE},
    {"INFRA_AO_MBIST_CONTROL_REG",                   TRUE},
    {"INFRA_AO _RSVD_1",                             TRUE},
    
    {"INFRA_AO_IO_CONFG_T",                          TRUE},  /*20*/
    {"INFRA_AO_IO_CONFG_B",                          TRUE},
    {"INFRA_AO_IO_CONFG_L",                          TRUE},
    {"INFRA_AO_IO_CONFG_R",                          TRUE},
    {"INFRA_AO_APMIXEDSYS",                          TRUE},
    {"DEGBUGSYS",                                    TRUE},
    {"INFRASYS_MCUSYS_CONFIG_REG",                   TRUE},
    {"INFRASYS_CONTROL_REG",                         TRUE},
    {"INFRASYS_SYSTEM_CIRQ",                         TRUE},
    {"INFRASYS_MMSYS_IOMMU",                         TRUE},
    
    {"INFRASYS _DEVICE_APC",                         TRUE},  /*30*/
    {"INFRASYS_EMI",                                 FALSE},
    {"INFRASYS_DRAMC",                               FALSE},
    {"INFRASYS_DRAMC_CONFIG",                        FALSE},
    {"INFRASYS_DDRPHY_CONFIG",                       FALSE},
    {"INFRASYS_SRAMROM",                             TRUE},
    {"INFRASYS_GCE",                                 TRUE},
    {"INFRASYS_BUS_DBG_TRACKER",                     TRUE},
    {"INFRASYS_TRNG",                                TRUE},
    {"MCUSYS_BIU_CONFIG",                            TRUE},
    
    {"DMA",                                          TRUE},  /*40*/
    {"NFI",                                          TRUE},
    {"NFI_ECC",                                      TRUE},
    {"AUXADC",                                       FALSE},
    {"FHCTL",                                        TRUE},
    {"UART0",                                        TRUE},
    {"UART1",                                        TRUE},
    {"UART2",                                        TRUE},
    {"PWM",                                          TRUE},
    {"I2C0",                                         TRUE},
    
    {"I2C1",                                         TRUE},  /*50*/
    {"I2C2",                                         TRUE},
    {"SPI0",                                         TRUE},
    {"PTP",                                          TRUE},
    {"BTIF",                                         TRUE},
    {"DISP_PWM",                                     TRUE},
    {"USB2.0",                                       FALSE},
    {"USB2.0 SIF",                                   FALSE},
    {"MSDC0",                                        TRUE},
    {"MSDC1",                                        TRUE},
    
    {"AUDIO",                                        FALSE},  /*60*/
    {"CONN_PERIPHERALS",                             FALSE},
    {"MD1_PERIPHERALS",                              TRUE},
    {"G3D_CONFIG",                                   TRUE},
    {"MALI",                                         TRUE},
    {"MMSYS_CONFIG",                                 TRUE},
    {"MDP_RDMA",                                     TRUE},
    {"MDP_RSZ0",                                     TRUE},
    {"MDP_RSZ1",                                     TRUE},
    {"MDP_WDMA",                                     TRUE},
    
    {"MDP_WROT",                                     TRUE},  /*70*/
    {"MDP_TDSHP",                                    TRUE},
    {"DISP_OVL0",                                    TRUE},
    {"DISP_OVL1",                                    TRUE}, 
    {"DISP_RDMA0",                                   TRUE},
    {"DISP_RDMA1",                                   TRUE},
    {"DISP_WDMA",                                    TRUE},
    {"DISP_COLOR",                                   TRUE},
    {"DISP_CCORR",                                   TRUE},
    {"DISP_AAL",                                     TRUE},
    
    {"DISP_GAMMA",                                   TRUE},  /*80*/
    {"DISP_DITHER",                                  TRUE},
    {"DSI_UFOE",                                     TRUE},
    {"DSI0",                                         TRUE},
    {"DPI0",                                         TRUE},
    {"DISP_PWM0",                                    TRUE},
    {"MM_MUTEX",                                     TRUE},
    {"SMI_LARB0",                                    TRUE},
    {"SMI_COMMON",                                   TRUE},
    {"MIPI_TX_CONFIG",                               TRUE},
    
    {"IMGSYS_CONFIG",                                TRUE},  /*90*/
    {"SMI_LARB2",                                    TRUE},
    {"CAM_0",                                        TRUE},
    {"CAM_1",                                        TRUE},
    {"SENINF_TOP",                                   TRUE},
    {"VENC",                                         TRUE},
    {"VDEC",                                         TRUE},

};

/*****************************************************************************
*FUNCTION DEFINITION
*****************************************************************************/
static int clear_vio_status(unsigned int module);
static int devapc_ioremap(void);
/**************************************************************************
*EXTERN FUNCTION
**************************************************************************/
int mt_devapc_check_emi_violation(void)
{
    if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_3)) & ABORT_EMI) == 0) {
        return -1;
    } else {
        pr_err("EMI violation! It should be cleared by EMI MPU driver later!\n");
        return 0;
    }
}

int mt_devapc_emi_initial(void)
{
    pr_warn("EMI_DAPC Init start \n");

    devapc_ioremap();
    
    mt_reg_sync_writel(readl(IOMEM(DEVAPC0_APC_CON)) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_APC_CON);
    mt_reg_sync_writel(readl(IOMEM(DEVAPC0_PD_APC_CON)) & (0xFFFFFFFF ^ (1 << 2)), DEVAPC0_PD_APC_CON);
    mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_3);
    mt_reg_sync_writel(readl(IOMEM(DEVAPC0_D0_VIO_MASK_3)) & (0xFFFFFFFF ^ (ABORT_EMI)), DEVAPC0_D0_VIO_MASK_3);
    pr_warn("EMI_DAPC Init done \n");
    return 0;
}

int mt_devapc_clear_emi_violation(void)
{
    if ((readl(IOMEM(DEVAPC0_D0_VIO_STA_3)) & ABORT_EMI) != 0) {
        mt_reg_sync_writel(ABORT_EMI, DEVAPC0_D0_VIO_STA_3);
    }
    return 0;
}


 /*
 * mt_devapc_set_permission: set module permission on device apc.
 * @module: the moudle to specify permission
 * @domain_num: domain index number
 * @permission_control: specified permission
 * no return value.
 */
int mt_devapc_set_permission(unsigned int module, E_MASK_DOM domain_num, APC_ATTR permission)
{
    volatile unsigned int* base;
    unsigned int clr_bit = 0x3 << ((module % 16) * 2);
    unsigned int set_bit = permission << ((module % 16) * 2);

    if(module >= DEVAPC_DEVICE_NUMBER) {
        pr_warn("[DEVAPC] ERROR, device number %d exceeds the max number!\n", module);
        return -1;
    }

    if (DEVAPC_DOMAIN_AP == domain_num) {
        base = DEVAPC0_D0_APC_0 + (module / 16) * 4;
    } else if (DEVAPC_DOMAIN_MD == domain_num) {
        base = DEVAPC0_D1_APC_0 + (module / 16) * 4;
    } else if (DEVAPC_DOMAIN_CONN == domain_num) {
        base = DEVAPC0_D2_APC_0 + (module / 16) * 4;
    } else if (DEVAPC_DOMAIN_MM == domain_num) {
        base = DEVAPC0_D3_APC_0 + (module / 16) * 4;
    } else {
        pr_warn("[DEVAPC] ERROR, domain number %d exceeds the max number!\n", domain_num);
        return -2;
    }

    mt_reg_sync_writel(readl(base) & ~clr_bit, base);
    mt_reg_sync_writel(readl(base) | set_bit, base);
    return 0;
}
/**************************************************************************
*STATIC FUNCTION
**************************************************************************/

static int devapc_ioremap(void)
{
    struct device_node *node = NULL;
    /*IO remap*/
    node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC_AO");
    if (node) {
       devapc_ao_base = of_iomap(node, 0);
       pr_warn("[DEVAPC] AO_ADDRESS=%p \n",devapc_ao_base);
    } else {
       pr_warn("[DEVAPC] can't find DAPC_AO compatible node \n");
       return -1;
    }

    node = of_find_compatible_node(NULL, NULL, "mediatek,DEVAPC");
    if (node) {
       devapc_pd_base = of_iomap(node, 0);
       devapc_irq = irq_of_parse_and_map(node, 0);
       pr_warn("[DEVAPC] PD_ADDRESS=%p, IRD: %d \n",devapc_pd_base,devapc_irq );
    } else {
       pr_warn("[DEVAPC] can't find DAPC_PD compatible node \n");
       return -1;
    }

    return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
static int devapc_pm_restore_noirq(struct device *device)
{
    if (devapc_irq != 0) {
        mt_irq_set_sens(devapc_irq, MT_LEVEL_SENSITIVE);
        mt_irq_set_polarity(devapc_irq, MT_POLARITY_LOW);
    }

    return 0;
}
#endif

/*
 * set_module_apc: set module permission on device apc.
 * @module: the moudle to specify permission
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * @permission_control: specified permission
 * no return value.
 */
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT)
static void start_devapc(void)
{
    pr_warn("[DEVAPC] Walk TEE path\n");

    mt_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_PD_APC_CON);
}
#else
static void set_module_apc(unsigned int module, E_MASK_DOM domain_num , APC_ATTR permission_control)
{

    volatile unsigned int* base= 0;
    unsigned int clr_bit = 0x3 << ((module % 16) * 2);
    unsigned int set_bit = permission_control << ((module % 16) * 2);

    if(module >= DEVAPC_DEVICE_NUMBER) {
        pr_warn("set_module_apc : device number %d exceeds the max number!\n", module);
        return;
    }

    if (DEVAPC_DOMAIN_AP == domain_num) {
        base = (unsigned int*)((uintptr_t)DEVAPC0_D0_APC_0 + (module / 16) * 4);
    } else if (DEVAPC_DOMAIN_MD == domain_num) {
        base = (unsigned int*)((uintptr_t)DEVAPC0_D1_APC_0 + (module / 16) * 4);
    } else if (DEVAPC_DOMAIN_CONN == domain_num) {
        base = (unsigned int*)((uintptr_t)DEVAPC0_D2_APC_0 + (module / 16) * 4);
    } else if (DEVAPC_DOMAIN_MM == domain_num) {
        base = (unsigned int*)((uintptr_t)DEVAPC0_D3_APC_0 + (module / 16) * 4);
    } else {
        pr_warn("set_module_apc : domain number %d exceeds the max number!\n", domain_num);
        return;
    }

    mt_reg_sync_writel( readl(base) & ~clr_bit, base);
    mt_reg_sync_writel( readl(base) | set_bit, base);
}

/*
 * unmask_module_irq: unmask device apc irq for specified module.
 * @module: the moudle to unmask
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * no return value.
 */
static int unmask_module_irq(unsigned int module)
{

    unsigned int apc_index = 0;
    unsigned int apc_bit_index = 0;

    apc_index = module / (MOD_NO_IN_1_DEVAPC*2);
    apc_bit_index = module % (MOD_NO_IN_1_DEVAPC*2);

    switch (apc_index) {
    case 0:
        *DEVAPC0_D0_VIO_MASK_0 &= ~(0x1 << apc_bit_index);
        break;
    case 1:
        *DEVAPC0_D0_VIO_MASK_1 &= ~(0x1 << apc_bit_index);
        break;
    case 2:
        *DEVAPC0_D0_VIO_MASK_2 &= ~(0x1 << apc_bit_index);
        break;
    case 3:
        *DEVAPC0_D0_VIO_MASK_3 &= ~(0x1 << apc_bit_index);
        break;
    default:
        pr_warn("UnMask_Module_IRQ_ERR :check if domain master setting is correct or not !\n");
        break;
    }
    return 0;
}

static void init_devpac(void)
{
   /* clear violation*/
   mt_reg_sync_writel(0x80000000, DEVAPC0_VIO_DBG0);
   mt_reg_sync_writel(readl(DEVAPC0_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC0_APC_CON);
   mt_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_PD_APC_CON);
}


/*
 * start_devapc: start device apc for MD
 */
static int start_devapc(void)
{
    int i = 0;

    init_devpac();
    for (i = 0; i < (sizeof(devapc_devices) / sizeof(devapc_devices[0])); i++) {
        if (TRUE == devapc_devices[i].forbidden) {
            clear_vio_status(i);
            unmask_module_irq(i);
            set_module_apc(i, E_DOMAIN_1, E_L3);
        }
    }
    return 0;
}


#endif


/*
 * clear_vio_status: clear violation status for each module.
 * @module: the moudle to clear violation status
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * no return value.
 */
static int clear_vio_status(unsigned int module)
{

    unsigned int apc_index = 0;
    unsigned int apc_bit_index = 0;

    apc_index = module / (MOD_NO_IN_1_DEVAPC*2);
    apc_bit_index = module % (MOD_NO_IN_1_DEVAPC*2);

    switch (apc_index) {
    case 0:
       *DEVAPC0_D0_VIO_STA_0 = (0x1 << apc_bit_index);
       break;
    case 1:
       *DEVAPC0_D0_VIO_STA_1 = (0x1 << apc_bit_index);
       break;
    case 2:
       *DEVAPC0_D0_VIO_STA_2 = (0x1 << apc_bit_index);
        break;
    case 3:
       *DEVAPC0_D0_VIO_STA_3 = (0x1 << apc_bit_index);
        break;
    default:
       break;
    }

    return 0;
}


static irqreturn_t devapc_violation_irq(int irq, void *dev_id)
{
    unsigned int dbg0 = 0, dbg1 = 0;
    unsigned int master_id;
    unsigned int domain_id;
    unsigned int r_w_violation;
    int i;

    dbg0 = readl(DEVAPC0_VIO_DBG0);
    dbg1 = readl(DEVAPC0_VIO_DBG1);
    master_id = dbg0 & VIO_DBG_MSTID;
    domain_id = dbg0 & VIO_DBG_DMNID;
    r_w_violation = dbg0 & VIO_DBG_RW;

    pr_err("[DEVAPC] Device Access Permission Violation (Device APC, DAPC)\n");
    if (1 == r_w_violation) {
        pr_err("[DEVAPC] Process:%s PID:%i Vio Addr:0x%x , Master ID:0x%x , Dom ID:0x%x, W\n",
        current->comm,current->pid, dbg1, master_id, domain_id);
    } else {
        pr_err("[DEVAPC] Process:%s PID:%i Vio Addr:0x%x , Master ID:0x%x , Dom ID:0x%x, r\n",
        current->comm,current->pid, dbg1, master_id, domain_id);
    }

    pr_err("[DEVAPC] VIO_STA 0:0x%x, 1:0x%x, 2:0x%x, 3:0x%x\n",
    readl(DEVAPC0_D0_VIO_STA_0 ), readl(DEVAPC0_D0_VIO_STA_1 ), readl(DEVAPC0_D0_VIO_STA_2 ),
    readl(DEVAPC0_D0_VIO_STA_3 ));

    for (i = 0; i < (sizeof(devapc_devices) / sizeof(devapc_devices[0])); i++) {
        clear_vio_status(i);
    }

    mt_reg_sync_writel(VIO_DBG_CLR , DEVAPC0_VIO_DBG0);
    dbg0 = readl(DEVAPC0_VIO_DBG0);
    dbg1 = readl(DEVAPC0_VIO_DBG1);

    if ((dbg0 != 0) || (dbg1 != 0)) {
        pr_err("[DEVAPC] Multi-violation!\n");
        pr_err("[DEVAPC] DBG0 = %x, DBG1 = %x\n", dbg0, dbg1);
    }

    return IRQ_HANDLED;
}

static int devapc_probe(struct platform_device *dev)
{
    int ret;

    pr_warn("[DEVAPC] module probe. \n");
    /*IO remap*/
    devapc_ioremap();
    /*
    * Interrupts of violation (including SPC in SMI, or EMI MPU) are triggered by the device APC.
    * need to share the interrupt with the SPC driver.
    */
    ret = request_irq(devapc_irq, (irq_handler_t)devapc_violation_irq,
        IRQF_TRIGGER_LOW | IRQF_SHARED, "devapc", &g_devapc_ctrl);
    if (ret) {
        pr_err("[DEVAPC] Failed to request irq! (%d)\n", ret);
        return ret;
    }

#ifdef CONFIG_MTK_HIBERNATION
    register_swsusp_restore_noirq_func(ID_M_DEVAPC, devapc_pm_restore_noirq, NULL);
#endif
    start_devapc();
    return 0;
}


static int devapc_remove(struct platform_device *dev)
{
    return 0;
}

static int devapc_suspend(struct platform_device *dev, pm_message_t state)
{
    return 0;
}

static int devapc_resume(struct platform_device *dev)
{
    pr_info("[DEVAPC] module resume. \n");
    start_devapc();

    return 0;
}

struct platform_device devapc_device = {
    .name = "devapc",
    .id = -1,
};

static struct platform_driver devapc_driver = {
    .probe = devapc_probe,
    .remove = devapc_remove,
    .suspend = devapc_suspend,
    .resume = devapc_resume,
    .driver = {
        .name = "devapc",
        .owner = THIS_MODULE,
    },
};

/*
 * devapc_init: module init function.
 */
static int __init devapc_init(void)
{
    int ret;

    /*You do not need to open the clock for Rainier*/
    //enable_clock(MT_CG_INFRA_DEVICE_APC,"DEVAPC");
    
    pr_warn("[DEVAPC] module init. \n");

    ret = platform_device_register(&devapc_device);
    if (ret) {
        pr_err("[DEVAPC] Unable to do device register(%d)\n", ret);
        return ret;
    }

    ret = platform_driver_register(&devapc_driver);
    if (ret) {
        pr_err("[DEVAPC] Unable to register driver (%d)\n", ret);
        platform_device_unregister(&devapc_device);
        return ret;
    }

    g_devapc_ctrl = cdev_alloc();
    if (!g_devapc_ctrl) {
        pr_err("[DEVAPC] Failed to add devapc device! (%d)\n", ret);
        platform_driver_unregister(&devapc_driver);
        platform_device_unregister(&devapc_device);
        return ret;
    }
    g_devapc_ctrl->owner = THIS_MODULE;

    return 0;
}

/*
 * devapc_exit: module exit function.
 */
static void __exit devapc_exit(void)
{
    pr_info("[DEVAPC] DEVAPC module exit\n");
#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_DEVAPC);
#endif
}

late_initcall(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
