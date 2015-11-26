/*
 *   libmnlg.h - Generic Netlink helpers for libmnl
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

#ifndef _MNLG_H_
#define _MNLG_H_

#include <libmnl/libmnl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mnlg_socket;

struct nlmsghdr *mnlg_msg_prepare(struct mnlg_socket *nlg, uint8_t cmd,
				  uint16_t flags);
int mnlg_socket_send(struct mnlg_socket *nlg, const struct nlmsghdr *nlh);
int mnlg_socket_recv_run(struct mnlg_socket *nlg, mnl_cb_t data_cb, void *data);
int mnlg_socket_group_add(struct mnlg_socket *nlg, const char *group_name);
struct mnlg_socket *mnlg_socket_open(const char *family_name, uint8_t version);
void mnlg_socket_close(struct mnlg_socket *nlg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _MNLG_H_ */
