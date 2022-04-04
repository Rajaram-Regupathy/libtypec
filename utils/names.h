/*
    Copyright (c) 2021-2022 by Rajaram Regupathy, rajaram.regupathy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    (See the full license text in the LICENSES directory)
*/
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Code based on usbutils: https://github.com/gregkh/usbutils
 */

#ifndef NAMES_H
#define NAMES_H


int names_init(void);
void names_exit(void);

int get_vendor_string(char *buf, size_t size, unsigned short vid);
int get_product_string(char *buf, size_t size, unsigned short vid, unsigned short pid);

#endif //NAMES_H