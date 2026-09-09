// SPDX-License-Identifier: GPL-2.0
/*
 * Casio fx-9750GIII platform support
 *
 * SH7305 Linux port by Artem Novak
 */

#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/machvec.h>

static const char artem_banner[] __initconst =
"\n"
"              _.-._\n"
"           .-'     `-.\n"
"         .'    _      `.\n"
"        /    .' `-.     \\\n"
"       ;    /      \\     ;\n"
"       |    \\      /     |\n"
"       ;     `-..-'      ;\n"
"        \\       \\       /\n"
"         `.      \\    .'\n"
"           `-._   \\.-'\n"
"                \\ /\n"
"                 '\n"
"\n"
"             Artem Novak\n"
"       Linux on Casio SH7305\n"
"           fx-9750GIII\n"
"\n";

static void __init fx9750giii_setup(char **cmdline_p)
{
	pr_info("%s", artem_banner);
	pr_info("fx-9750GIII SH7305 Linux port by Artem Novak\n");
}

static struct sh_machine_vector mv_fx9750giii __initmv = {
	.mv_name	= "Casio fx-9750GIII",
	.mv_setup	= fx9750giii_setup,
};
