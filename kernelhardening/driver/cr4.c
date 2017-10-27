/*
* This is an example ikgt usage driver.
* Copyright (c) 2015, Intel Corporation.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms and conditions of the GNU General Public License,
* version 2, as published by the Free Software Foundation.
*
* This program is distributed in the hope it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*/

#include <linux/module.h>

#include "policy_common.h"
#include "common.h"
#include "vmx_common.h"

static name_value_map cr4_bits[] = {
	{"VME",        VME},
	{"PVI",        PVI},
	{"TSD",        TSD},
	{"DE",         DE},
	{"PSE",        PSE},
	{"PAE",        PAE},
	{"MCE",        MCE},
	{"PGE",        PGE},
	{"PCE",        PCE},
	{"OSFXSR",     OSFXSR},
	{"OSXMMEXCPT", OSXMMEXCPT},
	{"VMXE",       VMXE},
	{"SMXE",       SMXE},
	{"PCIDE",      PCIDE},
	{"OSXSAVE",    OSXSAVE},
	{"SMEP",       SMEP},
	{"SMAP",       SMAP},

	/* Table terminator */
	{}
};

static ssize_t cr4_cfg_enable_store(struct config_item *item,
									const char *page,
									size_t count);

static ssize_t cr4_cfg_write_store(struct config_item *item,
								   const char *page,
								   size_t count);

static ssize_t cr4_cfg_sticky_value_store(struct config_item *item,
										  const char *page,
										  size_t count);

/* to_cr4_cfg() function */
IKGT_CONFIGFS_TO_CONTAINER(cr4_cfg);

/* item operations */
IKGT_UINT32_SHOW(cr4_cfg, enable);
IKGT_UINT32_HEX_SHOW(cr4_cfg, write);
IKGT_ULONG_HEX_SHOW(cr4_cfg, sticky_value);

/* attributes */
CONFIGFS_ATTR(cr4_cfg_, enable);
CONFIGFS_ATTR(cr4_cfg_, write);
CONFIGFS_ATTR(cr4_cfg_, sticky_value);

static struct configfs_attribute *cr4_cfg_attrs[] = {
	&cr4_cfg_attr_enable,
	&cr4_cfg_attr_write,
	&cr4_cfg_attr_sticky_value,
	NULL,
};

static int valid_cr4_attr(const char *name)
{
	int i;

	for (i = 0; cr4_bits[i].name; i++) {
		if (strcasecmp(cr4_bits[i].name, name) == 0) {
			return i;
		}
	}

	return -1;
}

static bool policy_set_cr4(struct cr4_cfg *cr4_cfg, bool enable)
{
//	policy_message_t *msg = NULL;
//	policy_update_rec_t *entry = NULL;
//	message_id_t msg_id;
//	uint64_t in_offset;
	int idx = valid_cr4_attr(cr4_cfg->item.ci_name);

	if (idx < 0)
		return false;

/*	msg = (policy_message_t *)allocate_in_buf(sizeof(policy_message_t), &in_offset);
	if (msg == NULL)
		return false;

	msg_id = enable?POLICY_ENTRY_ENABLE:POLICY_ENTRY_DISABLE;
	msg->count = 1;

	entry = &msg->policy_data[0];

	POLICY_SET_RESOURCE_ID(entry, cr4_bits[idx].res_id);
	POLICY_SET_WRITE_ACTION(entry, cr4_cfg->write);

	POLICY_SET_STICKY_VALUE(entry, cr4_cfg->sticky_value);

	POLICY_INFO_SET_MASK(entry, cr4_bits[idx].value);

	POLICY_INFO_SET_CPU_MASK_1(entry, -1);
	POLICY_INFO_SET_CPU_MASK_2(entry, -1);

	PRINTK_INFO("cpumask: %llx, %llx\n",
		POLICY_INFO_GET_CPU_MASK_1(entry), POLICY_INFO_GET_CPU_MASK_2(entry));

	ret = ikgt_hypercall(msg_id, in_offset, 0);
	if (SUCCESS != ret) {
		PRINTK_ERROR("%s: ikgt_hypercall failed, ret=%llu\n", __func__, ret);
	}

	free_in_buf(in_offset);

	return (ret == SUCCESS)?true:false;*/
	return true;
}

static ssize_t cr4_cfg_write_store(struct config_item *item,
								   const char *page,
								   size_t count)
{
	unsigned long value;
	struct cr4_cfg *cr4_cfg = to_cr4_cfg(item);

	if (cr4_cfg->locked)
		return -EPERM;

	if (kstrtoul(page, 0, &value))
		return -EINVAL;

	cr4_cfg->write = value;

	return count;
}

static ssize_t cr4_cfg_sticky_value_store(struct config_item *item,
										  const char *page,
										  size_t count)
{
	unsigned long value;
	struct cr4_cfg *cr4_cfg = to_cr4_cfg(item);

	if (cr4_cfg->locked)
		return -EPERM;

	if (kstrtoul(page, 0, &value))
		return -EINVAL;

	cr4_cfg->sticky_value = value;

	return count;
}

static ssize_t cr4_cfg_enable_store(struct config_item *item,
									const char *page,
									size_t count)
{
	unsigned long value;
	bool ret = false;
	struct cr4_cfg *cr4_cfg = to_cr4_cfg(item);

	if (kstrtoul(page, 0, &value))
		return -EINVAL;

	if (cr4_cfg->locked) {
		return -EPERM;
	}

	ret = policy_set_cr4(cr4_cfg, value);

	if (ret) {
		cr4_cfg->enable = value;
	}

	if (ret && (cr4_cfg->write & POLICY_ACT_STICKY))
		cr4_cfg->locked = true;

	return count;
}


static void cr4_cfg_release(struct config_item *item)
{
	kfree(to_cr4_cfg(item));
}

static struct configfs_item_operations cr4_cfg_ops = {
	.release = cr4_cfg_release,
};

static struct config_item_type cr4_cfg_type = {
	.ct_item_ops = &cr4_cfg_ops,
	.ct_attrs = cr4_cfg_attrs,
	.ct_owner = THIS_MODULE,
};


static struct config_item *cr4_make_item(struct config_group *group,
										 const char *name)
{
	struct cr4_cfg *cr4_cfg;

	PRINTK_INFO("CR4 create attribute file %s\n", name);

	if (valid_cr4_attr(name) == -1) {
		PRINTK_ERROR("Invalid CR4 bit name\n");
		return ERR_PTR(-EINVAL);
	}

	cr4_cfg = kzalloc(sizeof(struct cr4_cfg), GFP_KERNEL);
	if (!cr4_cfg) {
		return ERR_PTR(-ENOMEM);
	}

	config_item_init_type_name(&cr4_cfg->item, name,
		&cr4_cfg_type);

	return &cr4_cfg->item;
}

static ssize_t cr4_children_description_show(struct config_item *item,
				      char *page)
{
	return sprintf(page,
		"CR4\n"
		"\n"
		"Used in protected mode to control operations .  \n"
		"items are readable and writable.\n");
}

CONFIGFS_ATTR_RO(cr4_children_, description);

static struct configfs_attribute *cr4_children_attrs[] = {
	&cr4_children_attr_description,
	NULL,
};

static void cr4_children_release(struct config_item *item)
{
	kfree(to_node(item));
}

static struct configfs_item_operations cr4_children_item_ops = {
	.release	= cr4_children_release,
};

static struct configfs_group_operations cr4_children_group_ops = {
	.make_item	= cr4_make_item,
};

static struct config_item_type cr4_children_type = {
	.ct_item_ops	= &cr4_children_item_ops,
	.ct_group_ops	= &cr4_children_group_ops,
	.ct_attrs	= cr4_children_attrs,
	.ct_owner	= THIS_MODULE,
};

struct config_item_type *get_cr4_children_type(void)
{
	return &cr4_children_type;
}

