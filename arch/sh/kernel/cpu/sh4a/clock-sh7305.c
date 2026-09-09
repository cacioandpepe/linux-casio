// SPDX-License-Identifier: GPL-2.0
/*
 * Renesas SH7305 clock framework support
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/sh_clk.h>

#include <asm/clock.h>

/*
 * SH7305 Clock Pulse Generator
 */
#define SH7305_FRQCR		0xa4150000
#define SH7305_PLLCR		0xa4150024
#define SH7305_FLLFRQ		0xa4150050

/*
 * FRQCR:
 *   STC   bits 29..24
 *   IFC   bits 23..20
 *   BFC   bits 11..8
 *   P1FC  bits 3..0
 *
 * PLLCR:
 *   PLLE  bit 14
 *   FLLE  bit 12
 *
 * FLLFRQ:
 *   SELXM bits 15..14
 *   FLF   bits 10..0
 */

static unsigned long sh7305_base_rate(void)
{
	u32 frqcr  = __raw_readl(SH7305_FRQCR);
	u32 pllcr  = __raw_readl(SH7305_PLLCR);
	u32 fllfrq = __raw_readl(SH7305_FLLFRQ);

	unsigned int pll;
	unsigned int fll;
	unsigned long rate = 32768;

	/*
	 * SH7305 PLL ratio is STC + 1.
	 * Unlike SH7724 it must NOT be doubled.
	 */
	pll = ((frqcr >> 24) & 0x3f) + 1;

	/*
	 * FLL ratio is FLF, halved when SELXM == 1.
	 */
	fll = fllfrq & 0x7ff;

	if (((fllfrq >> 14) & 0x3) == 1)
		fll >>= 1;

	if (pllcr & (1 << 12))
		rate *= fll;

	if (pllcr & (1 << 14))
		rate *= pll;

	return rate;
}

static unsigned int sh7305_divisor(unsigned int shift)
{
	u32 frqcr = __raw_readl(SH7305_FRQCR);
	unsigned int field = (frqcr >> shift) & 0xf;

	/*
	 * SH7305 divider:
	 *
	 *     divisor = 2^(field + 1)
	 */
	return 1U << (field + 1);
}


/*
 * master_clk is the clock before the B/I/P divider stages.
 */
static void master_clk_init(struct clk *clk)
{
	clk->rate = sh7305_base_rate();
}

static struct sh_clk_ops sh7305_master_clk_ops = {
	.init = master_clk_init,
};


/*
 * Pphi - peripheral clock.
 *
 * P1FC occupies FRQCR bits 3..0.
 */
static unsigned long peripheral_clk_recalc(struct clk *clk)
{
	return clk->parent->rate / sh7305_divisor(0);
}

static struct sh_clk_ops sh7305_peripheral_clk_ops = {
	.recalc = peripheral_clk_recalc,
};


/*
 * Bphi - bus clock.
 *
 * BFC occupies FRQCR bits 11..8.
 */
static unsigned long bus_clk_recalc(struct clk *clk)
{
	return clk->parent->rate / sh7305_divisor(8);
}

static struct sh_clk_ops sh7305_bus_clk_ops = {
	.recalc = bus_clk_recalc,
};


/*
 * Iphi - CPU/internal clock.
 *
 * IFC occupies FRQCR bits 23..20.
 */
static unsigned long cpu_clk_recalc(struct clk *clk)
{
	return clk->parent->rate / sh7305_divisor(20);
}

static struct sh_clk_ops sh7305_cpu_clk_ops = {
	.recalc = cpu_clk_recalc,
};


/*
 * Ordering must match arch/sh/kernel/cpu/clock-cpg.c:
 *
 *   0 master
 *   1 peripheral
 *   2 bus
 *   3 cpu
 */
static struct sh_clk_ops *sh7305_clk_ops[] = {
	&sh7305_master_clk_ops,
	&sh7305_peripheral_clk_ops,
	&sh7305_bus_clk_ops,
	&sh7305_cpu_clk_ops,
};

void __init arch_init_clk_ops(struct sh_clk_ops **ops, int idx)
{
	if (idx < ARRAY_SIZE(sh7305_clk_ops))
		*ops = sh7305_clk_ops[idx];
}

int __init arch_clk_init(void)
{
	return cpg_clk_init();
}
