/**
 * @file libtypec_ops.h
 * @author Rajaram Regupathy
 * @brief 
 * 
 * Copyright (c) 2021 Rajaram Regupathy <>
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 */

#ifndef LIBTYPEC_OPS_H
#define LIBTYPEC_OPS_H

#include "libtypec.h"

#define SYSFS_TYPEC_PATH "/sys/class/typec"

/**
 * @brief 
 * 
 */
extern const struct libtypec_os_backend libtypec_lnx_sysfs_backend;

struct libtypec_os_backend {

    int (*init) (char **);

    int (*exit) (void);

    int (*get_capability_ops) (struct libtypec_capabiliy_data  *cap_data);

    int (*get_conn_capability_ops) (int conn_num, struct libtypec_connector_cap_data *conn_cap_data);

    int (*get_alternate_modes) (int recipient, int conn_num, struct altmode_data *alt_mode_data);

    int (*get_cam_supported_ops) (int conn_num, char *cam_data);

    int (*get_current_cam_ops) (char *cur_cam_data);

    int (*get_pdos_ops) (int conn_num, int partner,int offset, int num_pdo,int src_snk,int type,char *pdo_data);

    int (*get_cable_properties_ops) (int conn_num,char *cbl_prop_data);

    int (*get_connector_status_ops) (int conn_num, struct libtypec_connector_status *conn_sts);

};

#endif /*LIBTYPEC_OPS_H*/
