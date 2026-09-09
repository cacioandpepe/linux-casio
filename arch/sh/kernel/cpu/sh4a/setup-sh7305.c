// SPDX-License-Identifier: GPL-2.0
/*
 * Renesas SH7305 CPU setup
 *
 * Initial Linux support for the SH7305 used in Casio calculators.
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/sh_timer.h>

#include <asm/irq.h>
#include <asm/platform_early.h>
#include <linux/sh_intc.h>


/*
 * SH7305 TMU0
 *
 *   0xa4490004  TSTR
 *   0xa4490008  TMU channel 0
 *   0xa4490014  TMU channel 1
 *   0xa4490020  TMU channel 2
 *
 * The generic SH TMU driver maps from 0xa4490000 so TSTR appears
 * at the standard +0x04 offset.
 */
static struct sh_timer_config sh7305_tmu0_platform_data = {
	.channels_mask = 0x7,
};

static struct resource sh7305_tmu0_resources[] = {
	DEFINE_RES_MEM(0xa4490000, 0x2c),

	DEFINE_RES_IRQ(evt2irq(0x400)),
	DEFINE_RES_IRQ(evt2irq(0x420)),
	DEFINE_RES_IRQ(evt2irq(0x440)),
};

static struct platform_device sh7305_tmu0_device = {
	.name = "sh-tmu",
	.id = 0,
	.dev = {
		.platform_data = &sh7305_tmu0_platform_data,
	},
	.resource = sh7305_tmu0_resources,
	.num_resources = ARRAY_SIZE(sh7305_tmu0_resources),
};

static struct platform_device *sh7305_devices[] __initdata = {
	&sh7305_tmu0_device,
};

static int __init sh7305_devices_setup(void)
{
	return platform_add_devices(sh7305_devices,
				    ARRAY_SIZE(sh7305_devices));
}
arch_initcall(sh7305_devices_setup);

/*
 * TMU must also be available during the SH early-timer phase.
 */
static struct platform_device *sh7305_early_devices[] __initdata = {
	&sh7305_tmu0_device,
};

void __init plat_early_device_setup(void)
{
	sh_early_platform_add_devices(sh7305_early_devices,
				      ARRAY_SIZE(sh7305_early_devices));
}

enum {
	UNUSED = 0,
	ENABLED,
	DISABLED,

	TMU0_TUNI0,
	TMU0_TUNI1,
	TMU0_TUNI2,
};

/*
 * SH7305 TMU0 event codes.
 *
 * These match the SH7724/SH4A TMU0 event layout.
 */
static struct intc_vect vectors[] __initdata = {
	INTC_VECT(TMU0_TUNI0, 0x400),
	INTC_VECT(TMU0_TUNI1, 0x420),
	INTC_VECT(TMU0_TUNI2, 0x440),
};

/*
 * SH7305 IMR4 / IMCR4
 *
 * TMU0 channels occupy bits corresponding to TUNI0..2.
 */
static struct intc_mask_reg mask_registers[] __initdata = {
	{
		0xa4080090, 0xa40800d0, 8,
		{
			0,
			TMU0_TUNI2,
			TMU0_TUNI1,
			TMU0_TUNI0,
			0,
			0,
			0,
			0,
		}
	},
};

/*
 * SH7305 IPRA
 *
 * Four 4-bit interrupt priority fields:
 *
 *   bits 15..12  TMU0 channel 0
 *   bits 11..8   TMU0 channel 1
 *   bits  7..4   TMU0 channel 2
 *   bits  3..0   unused here
 */
static struct intc_prio_reg prio_registers[] __initdata = {
	{
		0xa4080000, 0, 16, 4,
		{
			TMU0_TUNI0,
			TMU0_TUNI1,
			TMU0_TUNI2,
			0,
		}
	},
};

static struct intc_desc intc_desc __initdata = {
	.name		= "sh7305",
	.force_enable	= ENABLED,
	.force_disable	= DISABLED,

	.hw = INTC_HW_DESC(
		vectors,
		NULL,
		mask_registers,
		prio_registers,
		NULL,
		NULL
	),
};

void __init plat_irq_setup(void)
{
	register_intc_controller(&intc_desc);
}
