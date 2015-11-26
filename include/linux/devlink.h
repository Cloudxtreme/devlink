/*
 * include/uapi/linux/devlink.h - Network physical device Netlink interface
 * Copyright (c) 2016 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2016 Jiri Pirko <jiri@mellanox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UAPI_LINUX_DEVLINK_H_
#define _UAPI_LINUX_DEVLINK_H_

#define DEVLINK_GENL_NAME "devlink"
#define DEVLINK_GENL_VERSION 0x1
#define DEVLINK_GENL_MCGRP_CONFIG_NAME "config"
#define DEVLINK_GENL_MCGRP_HWMSG_NAME "hwmsg"

enum devlink_command {
	/* don't change the order or add anything between, this is ABI! */
	DEVLINK_CMD_UNSPEC,

	DEVLINK_CMD_GET,		/* can dump */
	DEVLINK_CMD_SET,
	DEVLINK_CMD_NEW,
	DEVLINK_CMD_DEL,

	DEVLINK_CMD_HWMSG_NEW,

	DEVLINK_CMD_PORT_GET,		/* can dump */
	DEVLINK_CMD_PORT_SET,
	DEVLINK_CMD_PORT_NEW,
	DEVLINK_CMD_PORT_DEL,

	/* add new commands above here */

	__DEVLINK_CMD_MAX,
	DEVLINK_CMD_MAX = __DEVLINK_CMD_MAX - 1
};

enum devlink_hwmsg_type {
	DEVLINK_HWMSG_TYPE_MLX_EMAD, /* Mellanox EMAD packet */
	DEVLINK_HWMSG_TYPE_MLX_CMD_REG, /* Mellanox CMD iface register access */
};

enum devlink_hwmsg_dir {
	DEVLINK_HWMSG_DIR_TO_HW,
	DEVLINK_HWMSG_DIR_FROM_HW,
};

enum devlink_port_type {
	DEVLINK_PORT_TYPE_NOTSET,
	DEVLINK_PORT_TYPE_AUTO,
	DEVLINK_PORT_TYPE_ETH,
	DEVLINK_PORT_TYPE_IB,
};

enum devlink_attr {
	/* don't change the order or add anything between, this is ABI! */
	DEVLINK_ATTR_UNSPEC,

	DEVLINK_ATTR_INDEX,			/* u32 */
#define DEVLINK_ATTR_NAME_MAX_LEN 32
	DEVLINK_ATTR_NAME,			/* string */
	DEVLINK_ATTR_BUS_NAME,			/* string */
	DEVLINK_ATTR_DEV_NAME,			/* string */
	DEVLINK_ATTR_HWMSG_PAYLOAD,		/* data */
	DEVLINK_ATTR_HWMSG_TYPE,		/* u32 */
	DEVLINK_ATTR_HWMSG_DIR,			/* u8 */
	DEVLINK_ATTR_PORT_INDEX,		/* u32 */
	DEVLINK_ATTR_PORT_TYPE,			/* u16 */
	DEVLINK_ATTR_PORT_DESIRED_TYPE,		/* u16 */
	DEVLINK_ATTR_PORT_NETDEV_IFINDEX,	/* u32 */
	DEVLINK_ATTR_PORT_NETDEV_NAME,		/* string */
	DEVLINK_ATTR_PORT_IBDEV_NAME,		/* string */

	/* add new attributes above here, update the policy in devlink.c */

	__DEVLINK_ATTR_MAX,
	DEVLINK_ATTR_MAX = __DEVLINK_ATTR_MAX - 1
};

#endif /* _UAPI_LINUX_DEVLINK_H_ */
