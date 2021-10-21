// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016 - Beniamino Galvani <b.galvani@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/pinctrl.h>
#include <fdt_support.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/sizes.h>
#include <asm/gpio.h>

#include "pinctrl-meson.h"

DECLARE_GLOBAL_DATA_PTR;

static const char *meson_pinctrl_dummy_name = "_dummy";

int meson_pinctrl_get_groups_count(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->num_groups;
}

const char *meson_pinctrl_get_group_name(struct udevice *dev,
						unsigned selector)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	if (!priv->data->groups[selector].name)
		return meson_pinctrl_dummy_name;

	return priv->data->groups[selector].name;
}

int meson_pinmux_get_functions_count(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->num_funcs;
}

const char *meson_pinmux_get_function_name(struct udevice *dev,
						  unsigned selector)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->funcs[selector].name;
}

static int meson_gpio_get_bank(struct meson_pinctrl *priv, unsigned int offset,
				struct meson_bank **bank)
{
	int i;

	for (i = 0; i < priv->data->num_banks; i++) {
		if (offset >= priv->data->banks[i].first &&
		    offset <= priv->data->banks[i].last) {
			*bank = &priv->data->banks[i];
			return 0;
		}
	}

	return -EINVAL;
}

static void meson_gpio_calc_reg_and_bit(struct meson_bank *bank,
			unsigned int offset, enum meson_reg_type reg_type,
			unsigned int *reg, unsigned int *bit)
{
	struct meson_reg_desc *desc = &bank->regs[reg_type];

	*reg = desc->reg << 2;
	*bit = desc->bit + offset - bank->first;
}

int meson_pinconf_set(struct udevice *dev, unsigned int offset,
		unsigned int param, unsigned int arg)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
		debug("pin %u: disable bias\n", offset);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_PULLEN, &reg, &bit);
		clrsetbits_le32(priv->reg_pullen + reg, BIT(bit), 0);

		break;
	case PIN_CONFIG_BIAS_PULL_UP:
		debug("pin %u: enable pull-up\n", offset);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_PULL, &reg, &bit);
		clrsetbits_le32(priv->reg_pull + reg, BIT(bit), BIT(bit));

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_PULLEN, &reg, &bit);
		clrsetbits_le32(priv->reg_pullen + reg, BIT(bit), BIT(bit));

		break;
	case PIN_CONFIG_BIAS_PULL_DOWN:
		debug("pin %u: enable pull-down\n", offset);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_PULL, &reg, &bit);
		clrsetbits_le32(priv->reg_pull + reg, BIT(bit), 0);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_PULLEN, &reg, &bit);
		clrsetbits_le32(priv->reg_pullen + reg, BIT(bit), BIT(bit));

		break;
	case PIN_CONFIG_INPUT_ENABLE:
		debug("pin %u: enable input\n", offset);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_DIR, &reg, &bit);
		clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), BIT(bit));

		break;
	case PIN_CONFIG_OUTPUT:
		debug("pin %u: output %s\n", offset, arg ? "high" : "low");

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_OUT, &reg, &bit);
		clrsetbits_le32(priv->reg_gpio + reg, BIT(bit),
				arg ? BIT(bit) : 0);

		meson_gpio_calc_reg_and_bit(bank, offset,
					REG_DIR, &reg, &bit);
		clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), 0);

		break;
#ifdef CONFIG_PINCONF_MESON_G12A
	case PIN_CONFIG_DRIVE_STRENGTH:
		if (!priv->data->drv_data) {
			debug("pin [%u] does not support drive-strength\n",
					offset);
			return -EINVAL;
		}
		ret = meson_pinconf_set_drive_strength(priv, offset, arg);
		if (ret)
			return ret;

		break;
#endif
	default:
	      return -ENOTSUPP;
	}

	return 0;
}

int meson_pinconf_group_set(struct udevice *dev, unsigned int group_selector,
				unsigned int param, unsigned int arg)
{
	int i;
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct meson_pmx_group *group = &priv->data->groups[group_selector];

	for (i = 0; i < group->num_pins; i++)
		meson_pinconf_set(dev, group->pins[i], param, arg);

	return 0;
}

int meson_gpio_find_offset_by_name(struct udevice *dev,
				const char *name, ulong *offset)
{
	int i;
	int len;
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);

	for (i = 0; i < priv->data->num_banks; i++) {
		len = priv->data->banks[i].name ?
			strlen(priv->data->banks[i].name) : 0;
		if (!strncasecmp(priv->data->banks[i].name, name, len)) {
			if (!strict_strtoul(name + len, 10, offset)) {
				*offset = *offset + priv->data->banks[i].first;
				break;
			}
		}
	}

	if (priv->data->num_banks == i)
		return -EINVAL;

	return 0;
}

static int meson_gpio_request(struct udevice *dev, unsigned int offset,
				const char *label)
{
	return pinctrl_set_gpio_mux(dev->parent, 0, offset);
}

static int meson_gpio_get(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	meson_gpio_calc_reg_and_bit(bank, offset, REG_IN, &reg, &bit);

	return !!(readl(priv->reg_gpio + reg) & BIT(bit));
}

static int meson_gpio_set(struct udevice *dev, unsigned int offset, int value)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	meson_gpio_calc_reg_and_bit(bank, offset, REG_OUT, &reg, &bit);

	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), value ? BIT(bit) : 0);

	return 0;
}

static int meson_gpio_get_direction(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct meson_bank *bank;
	unsigned int reg, bit, val;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	meson_gpio_calc_reg_and_bit(bank, offset, REG_DIR, &reg, &bit);

	val = readl(priv->reg_gpio + reg);

	return (val & BIT(bit)) ? GPIOF_INPUT : GPIOF_OUTPUT;
}

static int meson_gpio_direction_input(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	meson_gpio_calc_reg_and_bit(bank, offset, REG_DIR, &reg, &bit);

	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), BIT(bit));

	return 0;
}

static int meson_gpio_direction_output(struct udevice *dev,
				       unsigned int offset, int value)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct meson_bank *bank;
	unsigned int reg, bit;
	int ret;

	ret = meson_gpio_get_bank(priv, offset, &bank);
	if (ret)
		return ret;

	meson_gpio_calc_reg_and_bit(bank, offset, REG_OUT, &reg, &bit);

	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), value ? BIT(bit) : 0);

	meson_gpio_calc_reg_and_bit(bank, offset, REG_DIR, &reg, &bit);

	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), 0);

	return 0;
}

static int meson_gpio_probe(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	struct gpio_dev_priv *uc_priv;

	uc_priv = dev_get_uclass_priv(dev);
	uc_priv->bank_name = priv->data->name;
	uc_priv->gpio_count = priv->data->num_pins;

	return 0;
}

static const struct dm_gpio_ops meson_gpio_ops = {
	.request = meson_gpio_request,
	.set_value = meson_gpio_set,
	.get_value = meson_gpio_get,
	.get_function = meson_gpio_get_direction,
	.direction_input = meson_gpio_direction_input,
	.direction_output = meson_gpio_direction_output,
};

static struct driver meson_gpio_driver = {
	.name	= "meson-gpio",
	.id	= UCLASS_GPIO,
	.probe	= meson_gpio_probe,
	.ops	= &meson_gpio_ops,
};

static int check_string(int offset, const char *property, const char *string)
{
	const void *prop;
	int len;

	prop = fdt_getprop(gd->fdt_blob, offset, property, &len);
	if (!prop)
		return len;

	return !fdt_stringlist_contains(prop, len, string);
}

static fdt_addr_t parse_address(int offset, const char *name, int na, int ns)
{
	int index, len = 0;
	const fdt32_t *reg;

	index = fdt_stringlist_search(gd->fdt_blob, offset, "reg-names", name);
	if (index < 0)
		return FDT_ADDR_T_NONE;

	reg = fdt_getprop(gd->fdt_blob, offset, "reg", &len);
	if (!reg || (len <= (index * sizeof(fdt32_t) * (na + ns))))
		return FDT_ADDR_T_NONE;

	reg += index * (na + ns);

	return fdt_translate_address((void *)gd->fdt_blob, offset, reg);
}

int meson_pinctrl_probe(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct uclass_driver *drv;
	struct udevice *gpio_dev;
	fdt_addr_t addr;
	int node, gpio = -1, len;
	int na, ns;
	char *name;

	na = fdt_address_cells(gd->fdt_blob, dev_of_offset(dev->parent));
	if (na < 1) {
		debug("bad #address-cells\n");
		return -EINVAL;
	}

	ns = fdt_size_cells(gd->fdt_blob, dev_of_offset(dev->parent));
	if (ns < 1) {
		debug("bad #size-cells\n");
		return -EINVAL;
	}

	fdt_for_each_subnode(node, gd->fdt_blob, dev_of_offset(dev)) {
		if (fdt_getprop(gd->fdt_blob, node, "gpio-controller", &len)) {
			gpio = node;
			break;
		}
	}

	if (!gpio) {
		debug("gpio node not found\n");
		return -EINVAL;
	}

	addr = parse_address(gpio, "mux", na, ns);
	if (addr == FDT_ADDR_T_NONE) {
		debug("mux address not found\n");
		return -EINVAL;
	}
	priv->reg_mux = (void __iomem *)addr;

	addr = parse_address(gpio, "gpio", na, ns);
	if (addr == FDT_ADDR_T_NONE) {
		debug("gpio address not found\n");
		return -EINVAL;
	}
	priv->reg_gpio = (void __iomem *)addr;

	if (check_string(gpio, "reg-names", "pull")) {
		priv->reg_pull = priv->reg_gpio;
	} else {
		addr = parse_address(gpio, "pull", na, ns);
		if (addr == FDT_ADDR_T_NONE) {
			debug("pull address not found\n");
			return -EINVAL;
		}
		priv->reg_pull = (void __iomem *)addr;
	}

	if (check_string(gpio, "reg-names", "pull-enable")) {
		priv->reg_pullen = priv->reg_pull;
	} else {
		addr = parse_address(gpio, "pull-enable", na, ns);
		if (addr == FDT_ADDR_T_NONE) {
			debug("pull address not found\n");
			return -EINVAL;
		}
		priv->reg_pullen = (void __iomem *)addr;
	}

	if (!check_string(gpio, "reg-names", "drive-strength")) {
		addr = parse_address(gpio, "drive-strength", na, ns);
		if (addr == FDT_ADDR_T_NONE) {
			debug("drive address not found\n");
			return -EINVAL;
		}
		priv->reg_drive = (void __iomem *)addr;
	}

	priv->data = (struct meson_pinctrl_data *)dev_get_driver_data(dev);

	/* Lookup GPIO driver */
	drv = lists_uclass_lookup(UCLASS_GPIO);
	if (!drv) {
		puts("Cannot find GPIO driver\n");
		return -ENOENT;
	}

	name = calloc(1, 32);
	sprintf(name, "meson-gpio");

	/* Create child device UCLASS_GPIO and bind it */
	device_bind(dev, &meson_gpio_driver, name, NULL, gpio, &gpio_dev);
	dev_set_of_offset(gpio_dev, gpio);

	return 0;
}
