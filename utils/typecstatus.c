/*
MIT License

Copyright (c) 2022 Rajaram Regupathy <rajaram.regupathy@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
// SPDX-License-Identifier: MIT
/**
 * @file typecstatus.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief  Initial prototype. Performs status check of typec ports.
 */

#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "../libtypec.h"
#include <stdlib.h>
#include "names.h"

struct libtypec_connector_status conn_sts;


static unsigned long get_dword_from_path(char *path)
{
	char buf[64];
	unsigned long dword;

	FILE *fp = fopen(path, "r");
	
    if (fp == NULL)
		return -1;

	if(fgets(buf, 64, fp) == NULL)
		return -1;

	dword = strtoul(buf, NULL, 10);

	fclose(fp);

	return dword;
}

/* Check all typec ports */
static int all_flag;
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

int main (int argc, char **argv)
{
    int ret,index=0;

    if(argc == 1)
    {
	    printf("typecstatus - Check status of typec ports\n Usage:\t typecstatus --all \n --all\t All typec ports\n");
        exit(0);
    }

    static struct option options[] =
    {
        /* These options set a flag. */
        {"all", no_argument,&all_flag, 1},
    };

    ret = getopt_long (argc, argv, "", options, &index);

    if(ret != 0)    
        printf("typecstatus - Check status of typec ports\n Usage:\t typecstatus --all \n");

    names_init();

    ret = libtypec_init(session_info);

    if (ret < 0)
        printf("Failed in Initializing libtypec\n");

    if(all_flag)
    {
        unsigned long tdp, bst_pwr;

        ret = libtypec_get_connector_status(0, &conn_sts);

        if(conn_sts.rdo)
        {
            printf("USB-C Power Status\n==================\n");

            printf("\tUSB-C power contract Operating Power %ld W, with Max Power %ld W\n",(((conn_sts.rdo>>10)&0x3ff) * 250)/1000,((conn_sts.rdo&0x3ff) * 250)/1000);

            tdp = get_dword_from_path("/sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw")/1000000;

            bst_pwr = get_dword_from_path("/sys/class/powercap/intel-rapl:0/constraint_1_power_limit_uw")/1000000;

            printf("\tCharging System with TDP %ld W, with boost power requirement of %ld W\n",tdp,bst_pwr);
        }        
    }
    names_exit();
}