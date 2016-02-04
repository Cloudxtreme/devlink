/*
 *   dl.c - Devlink tool
 *   Copyright (C) 2016 Jiri Pirko <jiri@mellanox.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <linux/genetlink.h>
#include <linux/devlink.h>
#include <libmnl/libmnl.h>
#include <mnlg.h>

#include <private/misc.h>
#include <private/list.h>

enum verbosity_level {
	VERB1,
	VERB2,
	VERB3,
	VERB4,
};

#define DEFAULT_VERB VERB1
static int g_verbosity = DEFAULT_VERB;

#define pr_err(args...) fprintf(stderr, ##args)
#define pr_outx(verb_level, args...) \
	do { \
		if (verb_level <= g_verbosity) { \
			fprintf(stdout, ##args); \
		} \
	} while (0);
#define pr_out(args...) pr_outx(DEFAULT_VERB, ##args)
#define pr_out2(args...) pr_outx(VERB2, ##args)
#define pr_out3(args...) pr_outx(VERB3, ##args)
#define pr_out4(args...) pr_outx(VERB4, ##args)

static int _mnlg_socket_send(struct mnlg_socket *nlg,
			     const struct nlmsghdr *nlh)
{
	int err;

	err = mnlg_socket_send(nlg, nlh);
	if (err < 0) {
		pr_err("Failed to call mnlg_socket_send\n");
		return -errno;
	}
	return 0;
}

static int _mnlg_socket_recv_run(struct mnlg_socket *nlg,
				 mnl_cb_t data_cb, void *data)
{
	int err;

	err = mnlg_socket_recv_run(nlg, data_cb, data);
	if (err < 0) {
		pr_err("Failed to call mnlg_socket_recv_run\n");
		return -errno;
	}
	return 0;
}

static int _mnlg_socket_group_add(struct mnlg_socket *nlg,
				  const char *group_name)
{
	int err;

	err = mnlg_socket_group_add(nlg, group_name);
	if (err < 0) {
		pr_err("Failed to call mnlg_socket_group_add\n");
		return -errno;
	}
	return 0;
}

struct index_map {
	struct list_item list;
	uint32_t index;
	char *name;
};

static struct index_map *index_map_alloc(uint32_t index, const char *name)
{
	struct index_map *index_map;

	index_map = myzalloc(sizeof(*index_map));
	if (!index_map)
		return NULL;
	index_map->index = index;
	index_map->name = strdup(name);
	if (!index_map->name) {
		free(index_map);
		return NULL;
	}
	return index_map;
}

static void index_map_free(struct index_map *index_map)
{
	free(index_map->name);
	free(index_map);
}

struct dl {
	struct mnlg_socket *nlg;
	struct list_item index_map_list;
	int argc;
	char **argv;
};

static int dl_argc(struct dl *dl)
{
	return dl->argc;
}

static char *dl_argv(struct dl *dl)
{
	if (dl_argc(dl) == 0)
		return NULL;
	return *dl->argv;
}

static void dl_arg_inc(struct dl *dl)
{
	if (dl_argc(dl) == 0)
		return;
	dl->argc--;
	dl->argv++;
}

static char *dl_argv_next(struct dl *dl)
{
	char *ret;

	if (dl_argc(dl) == 0)
		return NULL;

	ret = *dl->argv;
	dl_arg_inc(dl);
	return ret;
}

static int strcmpx(const char *str1, const char *str2)
{
	if (strlen(str1) > strlen(str2))
                return -1;
	return strncmp(str1, str2, strlen(str1));
}

static bool dl_argv_match(struct dl *dl, const char *pattern)
{
	if (dl_argc(dl) == 0)
		return false;
	return strcmpx(dl_argv(dl), pattern) == 0;
}

static bool dl_no_arg(struct dl *dl)
{
	return dl_argc(dl) == 0;
}

static int attr_cb(const struct nlattr *attr, void *data)
{
	const struct nlattr **tb = data;
	int type;

	type = mnl_attr_get_type(attr);

	if (mnl_attr_type_valid(attr, DEVLINK_ATTR_MAX) < 0)
		return MNL_CB_ERROR;

	if (type == DEVLINK_ATTR_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_BUS_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_DEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_INDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_DESIRED_TYPE &&
	    mnl_attr_validate(attr, MNL_TYPE_U16) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_NETDEV_IFINDEX &&
	    mnl_attr_validate(attr, MNL_TYPE_U32) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_NETDEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	if (type == DEVLINK_ATTR_PORT_IBDEV_NAME &&
	    mnl_attr_validate(attr, MNL_TYPE_NUL_STRING) < 0)
		return MNL_CB_ERROR;
	tb[type] = attr;
	return MNL_CB_OK;
}

static int index_map_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
	struct dl *dl = data;
	struct index_map *index_map;

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_INDEX] || !tb[DEVLINK_ATTR_NAME])
		return MNL_CB_ERROR;

	index_map = index_map_alloc(mnl_attr_get_u32(tb[DEVLINK_ATTR_INDEX]),
				    mnl_attr_get_str(tb[DEVLINK_ATTR_NAME]));
	if (!index_map)
		return MNL_CB_ERROR;
	list_add_tail(&dl->index_map_list, &index_map->list);

	return MNL_CB_OK;
}

static void index_map_fini(struct dl *dl)
{
	struct index_map *index_map, *tmp;

	list_for_each_node_entry_safe(index_map, tmp,
				      &dl->index_map_list, list) {
		list_del(&index_map->list);
		index_map_free(index_map);
	}
}

static int index_map_init(struct dl *dl)
{
	struct nlmsghdr *nlh;
	int err;

	list_init(&dl->index_map_list);

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_GET,
			       NLM_F_REQUEST | NLM_F_ACK | NLM_F_DUMP);

	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, index_map_cb, dl);
	if (err) {
		index_map_fini(dl);
		return err;
	}
	return 0;
}

static int index_map_get_index(struct dl *dl, const char *name)
{
	struct index_map *index_map;

	list_for_each_node_entry(index_map, &dl->index_map_list, list) {
		if (strcmp(name, index_map->name) == 0)
			return index_map->index;
	}
	return -ENOENT;
}

static const char *index_map_get_name(struct dl *dl, uint32_t index)
{
	static char tmp[32];
	struct index_map *index_map;

	list_for_each_node_entry(index_map, &dl->index_map_list, list) {
		if (index_map->index == index)
			return index_map->name;
	}
	sprintf(tmp, "<index %d>", index);
	return tmp;
}

static int dl_argv_index(struct dl *dl, uint32_t *p_index)
{
	const char *name = dl_argv_next(dl);
	int index;

	if (!name) {
		pr_err("Device name expected\n");
		return -EINVAL;
	}
	index = index_map_get_index(dl, name);
	if (index < 0) {
		pr_err("Device \"%s\" not found\n", name);
		return index;
	}
	*p_index = index;
	return 0;
}

static int slashsplit(char *str, char **before, char **after)
{
	char *slash;

	slash = strchr(str, '/');
	if (!slash)
		return -EINVAL;
	*slash = '\0';
	*before = str;
	*after = slash + 1;
	return 0;
}

static int strtouint(const char *str)
{
	char *endptr;
	unsigned long int val;

	val = strtoul(str, &endptr, 10);
	if (endptr == str || *endptr != '\0')
		return -EINVAL;
	if (val > INT_MAX)
		return -ERANGE;
	return val;
}

static int dl_argv_indexes(struct dl *dl, uint32_t *p_index,
			   uint32_t *p_port_index)
{
	char *str = dl_argv_next(dl);
	char *devstr;
	char *portstr;
	int index;
	int err;

	if (!str) {
		pr_err("Port identification (\"device/port_index\") expected\n");
		return -EINVAL;
	}

	err = slashsplit(str, &devstr, &portstr);
	if (err) {
		pr_err("Wrong port identification string format. Expected \"device/port_index\"\n");
		return err;
	}
	index = index_map_get_index(dl, devstr);
	if (index < 0) {
		pr_err("Device \"%s\" not found\n", devstr);
		return index;
	}
	*p_index = index;

	index = strtouint(portstr);
	if (index < 0) {
		pr_err("Port index \"%s\" is not a number\n", portstr);
		return index;
	}
	*p_port_index = index;
	return 0;
}

static int dl_argv_uint32_t(struct dl *dl, uint32_t *p_val)
{
	char *str = dl_argv_next(dl);
	int val;

	if (!str) {
		pr_err("Unsigned number expected\n");
		return -EINVAL;
	}

	val = strtouint(str);
	if (val < 0) {
		pr_err("\"%s\" is not a number\n", str);
		return val;
	}
	*p_val = val;
	return 0;
}

static void pr_out_dev(struct nlattr **tb)
{
	pr_out("%d: %s:", mnl_attr_get_u32(tb[DEVLINK_ATTR_INDEX]),
			  mnl_attr_get_str(tb[DEVLINK_ATTR_NAME]));
	if (tb[DEVLINK_ATTR_BUS_NAME])
		pr_out(" bus %s", mnl_attr_get_str(tb[DEVLINK_ATTR_BUS_NAME]));
	if (tb[DEVLINK_ATTR_DEV_NAME])
		pr_out(" dev %s", mnl_attr_get_str(tb[DEVLINK_ATTR_DEV_NAME]));
	pr_out("\n");
}

static int cmd_dev_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_INDEX] || !tb[DEVLINK_ATTR_NAME])
		return MNL_CB_ERROR;
	pr_out_dev(tb);
	return MNL_CB_OK;
}

static int cmd_dev_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_GET, flags);
	if (dl_argc(dl) == 1) {
		int index = index_map_get_index(dl, dl_argv(dl));

		if (index < 0)
			return index;
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);
	}

	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_dev_show_cb, NULL);
	if (err)
		return err;

	return 0;
}

static int cmd_dev_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	uint32_t index;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_SET, flags);
	err = dl_argv_index(dl, &index);
	if (err)
		return err;
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);

	while (dl_argc(dl)) {
		if (dl_argv_match(dl, "name")) {
			const char *name;

			dl_arg_inc(dl);
			name = dl_argv_next(dl);
			if (!name) {
				pr_err("Name argument expected\n");
				return -EINVAL;
			}
			mnl_attr_put_strz(nlh, DEVLINK_ATTR_NAME, name);
		}
		dl_arg_inc(dl);
	}
	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_dev_show_cb, NULL);
	if (err)
		return err;

	return 0;
}

static void cmd_dev_help() {
	pr_out("Usage: dl dev show [DEV]\n");
	pr_out("Usage: dl dev set DEV [ name NEWNAME ]\n");
}

static int cmd_dev(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_dev_help();
		return 0;
	} else if (dl_argv_match(dl, "show") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_dev_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_dev_set(dl);
	} else {
		pr_err("Command \"%s\" not found\n", dl_argv(dl));
		return -ENOENT;
	}
	return 0;
}

static const char *port_type_name(uint32_t type)
{
	switch (type) {
	case DEVLINK_PORT_TYPE_NOTSET: return "notset";
	case DEVLINK_PORT_TYPE_AUTO: return "auto";
	case DEVLINK_PORT_TYPE_ETH: return "eth";
	case DEVLINK_PORT_TYPE_IB: return "ib";
	default: return "<unknown type>";
	}
}

static void pr_out_port(struct dl *dl, struct nlattr **tb)
{
	pr_out("%s/%d:",
	       index_map_get_name(dl, mnl_attr_get_u32(tb[DEVLINK_ATTR_INDEX])),
	       mnl_attr_get_u32(tb[DEVLINK_ATTR_PORT_INDEX]));
	if (tb[DEVLINK_ATTR_PORT_TYPE]) {
		uint16_t port_type = mnl_attr_get_u16(tb[DEVLINK_ATTR_PORT_TYPE]);

		pr_out(" type %s", port_type_name(port_type));
		if (tb[DEVLINK_ATTR_PORT_DESIRED_TYPE]) {
			uint16_t des_port_type = mnl_attr_get_u16(tb[DEVLINK_ATTR_PORT_DESIRED_TYPE]);

			if (port_type != des_port_type)
				pr_out("(%s)", port_type_name(des_port_type));
		}
	}
	if (tb[DEVLINK_ATTR_PORT_NETDEV_NAME])
		pr_out(" netdev %s",
		       mnl_attr_get_str(tb[DEVLINK_ATTR_PORT_NETDEV_NAME]));
	if (tb[DEVLINK_ATTR_PORT_IBDEV_NAME])
		pr_out(" ibdev %s",
		       mnl_attr_get_str(tb[DEVLINK_ATTR_PORT_IBDEV_NAME]));
	pr_out("\n");
}

static int cmd_port_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);

	mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
	if (!tb[DEVLINK_ATTR_INDEX] || !tb[DEVLINK_ATTR_PORT_INDEX])
		return MNL_CB_ERROR;
	pr_out_port(dl, tb);
	return MNL_CB_OK;
}

static int cmd_port_show(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	int err;

	if (dl_argc(dl) == 0)
		flags |= NLM_F_DUMP;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_GET, flags);
	if (dl_argc(dl) == 1) {
		uint32_t index;
		uint32_t port_index;

		err = dl_argv_indexes(dl, &index, &port_index);
		if (err)
			return err;
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);
		mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_INDEX, port_index);
	}

	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_port_show_cb, dl);
	if (err)
		return err;

	return 0;
}

static int port_type_get(const char *typestr, enum devlink_port_type *p_type)
{
	if (strcmp(typestr, "auto") == 0) {
		*p_type = DEVLINK_PORT_TYPE_AUTO;
	} else if (strcmp(typestr, "eth") == 0) {
		*p_type = DEVLINK_PORT_TYPE_ETH;
	} else if (strcmp(typestr, "ib") == 0) {
		*p_type = DEVLINK_PORT_TYPE_IB;
	} else {
		pr_err("Unknown port type \"%s\"\n", typestr);
		return -EINVAL;
	}
	return 0;
}

static int cmd_port_set(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	uint32_t index;
	uint32_t port_index;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_SET, flags);
	err = dl_argv_indexes(dl, &index, &port_index);
	if (err)
		return err;
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_INDEX, port_index);

	while (dl_argc(dl)) {
		if (dl_argv_match(dl, "type")) {
			const char *typestr;
			enum devlink_port_type type;

			dl_arg_inc(dl);
			typestr = dl_argv_next(dl);
			if (!typestr) {
				pr_err("Type argument expected\n");
				return -EINVAL;
			}
			err = port_type_get(typestr, &type);
			if (err)
				return err;

			mnl_attr_put_u16(nlh, DEVLINK_ATTR_PORT_TYPE, type);
		}
		dl_arg_inc(dl);
	}
	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_dev_show_cb, NULL);
	if (err)
		return err;

	return 0;
}

static int cmd_port_split(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	uint32_t index;
	uint32_t port_index;
	uint32_t count;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_SPLIT, flags);
	err = dl_argv_indexes(dl, &index, &port_index);
	if (err)
		return err;
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_INDEX, port_index);

	err = dl_argv_uint32_t(dl, &count);
	if (err)
		return err;
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_SPLIT_COUNT, count);

	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_dev_show_cb, NULL);
	if (err)
		return err;

	return 0;
}

static int cmd_port_unsplit(struct dl *dl)
{
	struct nlmsghdr *nlh;
	uint16_t flags = NLM_F_REQUEST | NLM_F_ACK;
	uint32_t index;
	uint32_t port_index;
	int err;

	nlh = mnlg_msg_prepare(dl->nlg, DEVLINK_CMD_PORT_UNSPLIT, flags);
	err = dl_argv_indexes(dl, &index, &port_index);
	if (err)
		return err;
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_INDEX, index);
	mnl_attr_put_u32(nlh, DEVLINK_ATTR_PORT_INDEX, port_index);

	err = _mnlg_socket_send(dl->nlg, nlh);
	if (err)
		return err;

	err = _mnlg_socket_recv_run(dl->nlg, cmd_dev_show_cb, NULL);
	if (err)
		return err;

	return 0;
}

static void cmd_port_help() {
	pr_out("Usage: dl port show [DEV/PORT_INDEX]\n");
	pr_out("Usage: dl port set DEV/PORT_INDEX [ type { eth | ib | auto} ]\n");
	pr_out("Usage: dl port split DEV/PORT_INDEX count\n");
	pr_out("Usage: dl port unsplit DEV/PORT_INDEX\n");
}

static int cmd_port(struct dl *dl)
{
	if (dl_argv_match(dl, "help")) {
		cmd_port_help();
		return 0;
	} else if (dl_argv_match(dl, "show") || dl_no_arg(dl)) {
		dl_arg_inc(dl);
		return cmd_port_show(dl);
	} else if (dl_argv_match(dl, "set")) {
		dl_arg_inc(dl);
		return cmd_port_set(dl);
	} else if (dl_argv_match(dl, "split")) {
		dl_arg_inc(dl);
		return cmd_port_split(dl);
	} else if (dl_argv_match(dl, "unsplit")) {
		dl_arg_inc(dl);
		return cmd_port_unsplit(dl);
	} else {
		pr_err("Command \"%s\" not found\n", dl_argv(dl));
		return -ENOENT;
	}
	return 0;
}

static const char *cmd_name(uint8_t cmd)
{
	switch (cmd) {
	case DEVLINK_CMD_UNSPEC: return "unspec";
	case DEVLINK_CMD_GET: return "dev get";
	case DEVLINK_CMD_SET: return "dev set";
	case DEVLINK_CMD_NEW: return "dev new";
	case DEVLINK_CMD_DEL: return "dev del";
	case DEVLINK_CMD_HWMSG_NEW: return "hwmsg";
	case DEVLINK_CMD_PORT_GET: return "port get";
	case DEVLINK_CMD_PORT_SET: return "port set";
	case DEVLINK_CMD_PORT_NEW: return "port net";
	case DEVLINK_CMD_PORT_DEL: return "port del";
	default: return "<unknown cmd>";
	}
}

static void pr_out_mon_header(uint8_t cmd)
{
	pr_out("[%s] ", cmd_name(cmd));
}

static const char *hwmsg_type_name(uint32_t type)
{
	switch (type) {
	case DEVLINK_HWMSG_TYPE_MLX_EMAD: return "mlx_emad";
	case DEVLINK_HWMSG_TYPE_MLX_CMD_REG: return "mlx_cmd_reg";
	default: return "<unknown type>";
	}
}

static const char *hwmsg_dir_name(uint8_t dir)
{
	switch (dir) {
	case DEVLINK_HWMSG_DIR_TO_HW: return "to_hw";
	case DEVLINK_HWMSG_DIR_FROM_HW: return "from_hw";
	default: return "<unknown dir>";
	}
}

static void pr_out_hwmsg(struct nlattr **tb)
{
	uint32_t index = mnl_attr_get_u32(tb[DEVLINK_ATTR_INDEX]);
	uint32_t type = mnl_attr_get_u32(tb[DEVLINK_ATTR_HWMSG_TYPE]);
	uint8_t dir = mnl_attr_get_u8(tb[DEVLINK_ATTR_HWMSG_DIR]);
	uint16_t payload_len = mnl_attr_get_payload_len(tb[DEVLINK_ATTR_HWMSG_PAYLOAD]);
	unsigned char *payload = mnl_attr_get_payload(tb[DEVLINK_ATTR_HWMSG_PAYLOAD]);
	int i;

	pr_out("%d: %s %s %d bytes\n", index, hwmsg_type_name(type),
	       hwmsg_dir_name(dir), payload_len);
	for (i = 0; i < payload_len; i++) {
		if (i) {
			if (i % 8 == 0) {
				pr_out2("\n");
			} else {
				pr_out2(" ");
			}
		}
		if (i % 8 == 0)
			pr_out2("  0x%04x:  ", i);
		pr_out2("%02x", payload[i]);
	}
	if (i != 0)
		pr_out2("\n");
}

static bool check_cmd_hwmsg(struct nlattr **tb)
{
	uint32_t type;
	uint8_t dir;

	if (!tb[DEVLINK_ATTR_INDEX] ||
	    !tb[DEVLINK_ATTR_HWMSG_PAYLOAD] ||
	    !tb[DEVLINK_ATTR_HWMSG_TYPE] ||
	    !tb[DEVLINK_ATTR_HWMSG_DIR])
		return false;
	type = mnl_attr_get_u32(tb[DEVLINK_ATTR_HWMSG_TYPE]);
	if (type != DEVLINK_HWMSG_TYPE_MLX_EMAD)
		return false;
	dir = mnl_attr_get_u8(tb[DEVLINK_ATTR_HWMSG_DIR]);
	if (dir != DEVLINK_HWMSG_DIR_TO_HW && dir != DEVLINK_HWMSG_DIR_FROM_HW)
		return false;
	return true;
}

static int cmd_mon_show_cb(const struct nlmsghdr *nlh, void *data)
{
	struct dl *dl = data;
	struct nlattr *tb[DEVLINK_ATTR_MAX + 1] = {};
	struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
	uint8_t cmd = genl->cmd;

	switch (cmd) {
	case DEVLINK_CMD_GET: /* fall through */
	case DEVLINK_CMD_SET: /* fall through */
	case DEVLINK_CMD_NEW: /* fall through */
	case DEVLINK_CMD_DEL:
		mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
		if (!tb[DEVLINK_ATTR_INDEX] || !tb[DEVLINK_ATTR_NAME])
			return MNL_CB_ERROR;
		pr_out_mon_header(genl->cmd);
		pr_out_dev(tb);
		break;
	case DEVLINK_CMD_HWMSG_NEW:
		mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
		if (!check_cmd_hwmsg(tb))
			return MNL_CB_ERROR;
		pr_out_mon_header(genl->cmd);
		pr_out_hwmsg(tb);
		break;
	case DEVLINK_CMD_PORT_GET: /* fall through */
	case DEVLINK_CMD_PORT_SET: /* fall through */
	case DEVLINK_CMD_PORT_NEW: /* fall through */
	case DEVLINK_CMD_PORT_DEL:
		mnl_attr_parse(nlh, sizeof(*genl), attr_cb, tb);
		if (!tb[DEVLINK_ATTR_INDEX] || !tb[DEVLINK_ATTR_PORT_INDEX])
			return MNL_CB_ERROR;
		pr_out_mon_header(genl->cmd);
		pr_out_port(dl, tb);
		break;
	}
	return MNL_CB_OK;
}

static int cmd_monitor(struct dl *dl)
{
	int err;

	err = _mnlg_socket_group_add(dl->nlg, DEVLINK_GENL_MCGRP_CONFIG_NAME);
	if (err)
		return err;
	err = _mnlg_socket_group_add(dl->nlg, DEVLINK_GENL_MCGRP_HWMSG_NAME);
	if (err)
		return err;
	err = _mnlg_socket_recv_run(dl->nlg, cmd_mon_show_cb, dl);
	if (err)
		return err;
	return 0;
}

static void help() {
	pr_out("Usage: dl [ OPTIONS ] OBJECT { COMMAND | help }\n"
	       "where  OBJECT := { dev | port | monitor }\n"
	       "       OPTIONS := { -v/--verbose }\n");
}

static int dl_cmd(struct dl *dl)
{
	if (dl_argv_match(dl, "help") || dl_no_arg(dl)) {
		help();
		return 0;
	} else if (dl_argv_match(dl, "dev")) {
		dl_arg_inc(dl);
		return cmd_dev(dl);
	} else if (dl_argv_match(dl, "port")) {
		dl_arg_inc(dl);
		return cmd_port(dl);
	} else if (dl_argv_match(dl, "monitor")) {
		dl_arg_inc(dl);
		return cmd_monitor(dl);
	} else {
		pr_err("Object \"%s\" not found\n", dl_argv(dl));
		return -ENOENT;
	}
	return 0;
}

static int dl_init(struct dl *dl, int argc, char **argv)
{
	int err;

	dl->argc = argc;
	dl->argv = argv;

	dl->nlg = mnlg_socket_open(DEVLINK_GENL_NAME, DEVLINK_GENL_VERSION);
	if (!dl->nlg) {
		pr_err("Failed to connect to devlink Netlink\n");
		return -errno;
	}

	err = index_map_init(dl);
	if (err) {
		pr_err("Failed to create index map\n");
		goto err_index_map_create;
	}
	return 0;

err_index_map_create:
	mnlg_socket_close(dl->nlg);
	return err;
}

static void dl_fini(struct dl *dl)
{
	index_map_fini(dl);
	mnlg_socket_close(dl->nlg);
}

static struct dl *dl_alloc()
{
	struct dl *dl;

	dl = myzalloc(sizeof(*dl));
	if (!dl)
		return NULL;
	return dl;
}

static void dl_free(struct dl *dl)
{
	free(dl);
}

int main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "verbose",		no_argument,		NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};
	struct dl *dl;
	int opt;
	int err;
	int ret;

	while ((opt = getopt_long(argc, argv, "v",
				  long_options, NULL)) >= 0) {

		switch(opt) {
		case 'v':
			g_verbosity++;
			break;
		default:
			pr_err("Unknown option.\n");
			help();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	dl = dl_alloc();
	if (!dl) {
		pr_err("Failed to allocate memory for devlink\n");
		return EXIT_FAILURE;
	}

	err = dl_init(dl, argc, argv);
	if (err) {
		ret = EXIT_FAILURE;
		goto dl_free;
	}

	err = dl_cmd(dl);
	if (err) {
		pr_err("Command call failed (%s)\n", strerror(-err));
		ret = EXIT_FAILURE;
		goto dl_fini;
	}

	ret = EXIT_SUCCESS;

dl_fini:
	dl_fini(dl);
dl_free:
	dl_free(dl);

	return ret;
}
