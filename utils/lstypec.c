#include <stdio.h>
#include "../libtypec.h"
#include <stdlib.h>

#define LSTYPEC_MAJOR_VERSION 0
#define LSTYPEC_MINOR_VERSION 1

struct libtypec_capabiliy_data  get_cap_data;
struct libtypec_connector_cap_data conn_data;
struct libtypec_connector_status conn_sts;
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

    print_session_info(session_info);

    ret = libtypec_get_capability (&get_cap_data);

    if(ret < 0)
        lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

    print_ppm_capability(get_cap_data);

    for(int i=0;i<get_cap_data.bNumConnectors;i++){

        printf("\nConnector %d Capablity/Status\n",i);

        libtypec_get_conn_capability(i,&conn_data);

        print_conn_capability(conn_data);

        printf("\tAlternate Modes Supported:\n");

        ret = libtypec_get_alternate_modes(AM_CONNECTOR,i,am_data);
   
        print_alternate_mode_data(AM_CONNECTOR,ret,am_data);
       
	ret =  libtypec_get_connector_status(i,&conn_sts);

        if(conn_sts.connect_sts)
        {
            ret = libtypec_get_alternate_modes(AM_SOP,i,am_data);

            print_alternate_mode_data(AM_SOP,ret,am_data);
        }
    }

}
