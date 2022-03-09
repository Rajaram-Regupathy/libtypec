/*
    Copyright (c) 2021-2022 by Rajaram Regupathy, rajaram.regupathy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    (See the full license text in the LICENSES directory)
*/
// SPDX-License-Identifier: GPL-2.0-only
/**
 * @file lstypec.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Implements listing of typec port and port partner details
 *
 */

#include <stdio.h>
#include "../libtypec.h"
#include <stdlib.h>

#define LSTYPEC_MAJOR_VERSION 0
#define LSTYPEC_MINOR_VERSION 1

struct libtypec_capabiliy_data  get_cap_data;
struct libtypec_connector_cap_data conn_data;
struct libtypec_connector_status conn_sts;
struct libtypec_cable_property cable_prop;
union libtypec_discovered_identity id;

struct altmode_data am_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

#define LSTYPEC_ERROR 1
#define LSTYPEC_INFO  2

void print_session_info()
{
    printf("lstypec %d.%d Session Info\n",LSTYPEC_MAJOR_VERSION,LSTYPEC_MINOR_VERSION);

    printf("\tUsing %s\n",session_info[LIBTYPEC_VERSION_INDEX]);

    printf("\tOn Kernel %s\n",session_info[LIBTYPEC_KERNEL_INDEX]);

}

void print_ppm_capability(struct libtypec_capabiliy_data ppm_data)
{
    printf("\nUSB-C Platform Policy Manager Capability\n");

    printf("\tNumber of Connectors:\t\t%d\n",ppm_data.bNumConnectors);

    printf("\tNumber of Alternate Modes:\t%d\n",ppm_data.bNumAltModes);

    printf("\tUSB Power Delivery Revision:\t%d.%d\n",(ppm_data.bcdPDVersion >>8) & 0XFF,(ppm_data.bcdPDVersion) & 0XFF);

    printf("\tUSB Type-C Revision:\t\t%d.%d\n",(ppm_data.bcdTypeCVersion >>8) & 0XFF,(ppm_data.bcdTypeCVersion) & 0XFF);

}

void print_conn_capability(struct libtypec_connector_cap_data conn_data)
{
    char *opr_mode_str[] = {"Source","Sink","DRP","Analog Audio","Debug Accessory","USB2","USB3","Alternate Mode"};

    printf("\tOperation Mode:\t\t%s\n",opr_mode_str[conn_data.opr_mode]);

}

void print_cable_prop(struct libtypec_cable_property cable_prop,int conn_num)
{
    char *cable_type[] = {"Passive","Active","Unknown"};
    char *cable_plug_type[] = {"USB Type A","USB Type B","USB Type C", "Non-USB Type","Unknown"};

    printf("\tCable Property in Port %d:\n",conn_num);

    printf("\t\tCable Type\t:\t%s\n",cable_type[cable_prop.cable_type]);

    printf("\t\tCable Plug Type\t:\t%s\n",cable_plug_type[cable_prop.plug_end_type]);

}

void  print_alternate_mode_data(int recepient,int num_mode,struct altmode_data *am_data)
{

    if(recepient == AM_CONNECTOR) {

      for(int i=0;i<num_mode;i++){

            printf("\tLocal Modes %d:\n",i);

            printf("\t\tSVID\t:\t0x%04lx\n",am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n",am_data[i].vdo);
        }
    }

    if(recepient == AM_SOP) {

        for(int i=0;i<num_mode;i++){

            printf("\tPartner Modes %d:\n",i);

            printf("\t\tSVID\t:\t0x%04lx\n",am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n",am_data[i].vdo);
        }
    }

    if(recepient == AM_SOP_PR) {

        for(int i=0;i<num_mode;i++){

            printf("\tCable Plug Modes %d:\n",i);

            printf("\t\tSVID\t:\t0x%04lx\n",am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n",am_data[i].vdo);
        }
    }

}

void  print_identity_data(int recepient,union libtypec_discovered_identity id)
{
   if(recepient == AM_SOP) {

        printf("\tPartner Identity :\n");

        printf("\t\tCertificate\t:\t0x%04lx\n",id.disc_id.cert_stat);

        printf("\t\tID Header\t:\t0x%04lx\n",id.disc_id.id_header);

        printf("\t\tProduct\t\t:\t0x%04lx\n",id.disc_id.product);

        printf("\t\tProduct VDO 1\t:\t0x%04lx\n",id.disc_id.product_type_vdo1);

        printf("\t\tProduct VDO 2\t:\t0x%04lx\n",id.disc_id.product_type_vdo2);

        printf("\t\tProduct VDO 3\t:\t0x%04lx\n",id.disc_id.product_type_vdo3);
    }

    if(recepient == AM_SOP_PR) {

        printf("\tCable Identity :\n");

        printf("\t\tCertificate\t:\t0x%04lx\n",id.disc_id.cert_stat);

        printf("\t\tID Header\t:\t0x%04lx\n",id.disc_id.id_header);

        printf("\t\tProduct\t\t:\t0x%04lx\n",id.disc_id.product);

        printf("\t\tProduct VDO 1\t:\t0x%04lx\n",id.disc_id.product_type_vdo1);

        printf("\t\tProduct VDO 2\t:\t0x%04lx\n",id.disc_id.product_type_vdo2);

        printf("\t\tProduct VDO 3\t:\t0x%04lx\n",id.disc_id.product_type_vdo3);
   }
}

void lstypec_print(char* val, int type)
{
    if(type == LSTYPEC_ERROR ){
        printf("lstypec - ERROR - %s\n",val);
        exit(1);
    } else
       printf("lstypec - INFO - %s\n",val);
}

int main(void) {

    int ret;

    ret = libtypec_init(session_info);

    if(ret <0)
        lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

    print_session_info();

    ret = libtypec_get_capability (&get_cap_data);

    if(ret < 0)
        lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

    print_ppm_capability(get_cap_data);

    for(int i=0;i<get_cap_data.bNumConnectors;i++){
        /* Resetting port properties */
        cable_prop.cable_type = CABLE_TYPE_UNKNOWN;
        cable_prop.plug_end_type = PLUG_TYPE_OTH;

        printf("\nConnector %d Capablity/Status\n",i);

        libtypec_get_conn_capability(i,&conn_data);

        print_conn_capability(conn_data);

        ret = libtypec_get_cable_properties(i,&cable_prop);

        if(ret>=0)
           print_cable_prop(cable_prop,i);

        printf("\tAlternate Modes Supported:\n");

        ret = libtypec_get_alternate_modes(AM_CONNECTOR,i,am_data);

        if(ret > 0)
            print_alternate_mode_data(AM_CONNECTOR,ret,am_data);
        else
            printf("\t\tNo Local Modes listed with typec class\n");

            ret =  libtypec_get_connector_status(i,&conn_sts);

        if((ret == 0) && conn_sts.connect_sts)
        {
            ret = libtypec_get_alternate_modes(AM_SOP_PR,i,am_data);

            print_alternate_mode_data(AM_SOP_PR,ret,am_data);

            ret = libtypec_get_pd_message(AM_SOP_PR,i,24,DISCOVER_ID_REQ,id.buf_disc_id);

            if(ret>=0)
                print_identity_data(AM_SOP_PR,id);

            ret = libtypec_get_alternate_modes(AM_SOP,i,am_data);

            print_alternate_mode_data(AM_SOP,ret,am_data);

            ret = libtypec_get_pd_message(AM_SOP,i,24,DISCOVER_ID_REQ,id.buf_disc_id);

            if(ret>=0)
                print_identity_data(AM_SOP,id);

        }
    }

    printf("\n");
}
