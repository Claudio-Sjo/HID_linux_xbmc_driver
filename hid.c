/*
 * This file is part of hid_mapper.
 * 
 * hid_mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hid_mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hid_mapper. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Thibault Kummer <bob@coldsource.net>
 */

#include <hid.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define SYSFS_HIDRAW_CLASS_PATH "/sys/class/hidraw"

void init_hid_device(struct st_hid_device *hid_device)
{
	int i;
	
	hid_device->num_interfaces = 0;
	
	for(i=0;i<HID_MAX_INTERFACES;i++)
		hid_device->interface_fd[i] = -1;
	
	hid_device->interface_fd_max = -1;
}

static int get_file_contents(const char *filename,char *buf, unsigned int buf_len)
{
	int fd,re;
	
	fd = open(filename,O_RDONLY);
	if(fd<0) {
#ifdef DEBUG
		printf("Error opening file: %s\n", filename);
#endif
		buf = "Unknown";
		return -1;
	}
	
	re = read(fd,buf,buf_len);
	close(fd);
	
	if(re<0 || re>=buf_len) {
		printf("Error reading file: %s\n", filename);
		return -1;
	}
	
	buf[re] = '\0';
	if(re>0 && buf[re-1]=='\n')
		buf[re-1] = '\0';
	
	return 0;
}

/* 
Patched by Claudio 2013-05-24
Problem to solve : the file list is quite long, and the wished device is not the last
Next problem to solve : sometimes the driver is not able to start
*/

int lookup_hid_product(int lookup_mode,const char *manufacturer,const char *product,struct st_hid_device *hid_device)
{
	DIR *dh;
	struct dirent *entry;
	char buf[256],filename[PATH_MAX],symlink_filename[PATH_MAX];
	char dev_vendor[255],dev_manufacturer[255],dev_product[255];
	int re;

	printf("Looking for Manufacturer : %s\n", manufacturer);
	printf("Looking for Product : %s\n", product);
	
	if(manufacturer!=0)
		hid_device->num_interfaces = 0;

	// Open sys FS to lookup all HID devices
	dh = opendir(SYSFS_HIDRAW_CLASS_PATH);
	if(dh==0)
		return -1;
	
	while(entry = readdir(dh))
	{
// New patch
		{
		FILE *device_fd;
		char device_fn[255];
		char descr[255];
		int  found=0,exit=0;

		sprintf(dev_vendor,"\n");
		sprintf(dev_manufacturer,"\n");
		sprintf(dev_product,"\n");

//		printf("Entry name -->%s\n",entry->d_name);
		sprintf(device_fn,"%s/%s/%s",SYSFS_HIDRAW_CLASS_PATH,entry->d_name,"device/uevent");
//		printf("uevent file ->%s\n",device_fn);
		if(strlen("hidraw") < strlen(entry->d_name))
		{
			device_fd = fopen(device_fn,"r");
			do
			{
				fgets(descr,255,device_fd);
				if(feof(device_fd))
					exit++;
				else
				{
				int  lg = strlen(descr);
				char *ptr = descr;
//				printf("%s",descr);
				if(NULL!=strstr(descr,"HID_NAME="))
					sprintf(dev_vendor,"%s",(char *)(ptr+9));
                                if(NULL!=strstr(descr,"HID_ID="))
                                        sprintf(dev_manufacturer,"%s",(char *)(ptr+(lg-14)));
                                if(NULL!=strstr(descr,"HID_ID="))
                                        sprintf(dev_product,"%s",(char *)(ptr+lg-5));

				found++;
				} // else
			} while(!exit);
			fclose(device_fd);
		}

		if (found)
		{
		char	*p;
		dev_manufacturer[4]=(char)0;
		dev_product[4]=(char)0;
		for (p = dev_manufacturer;*p;++p) 
			*p=((*p>0x40)&&(*p<0x5b)? *p|0x60 : *p);
		for (p = dev_product;*p;++p) 
			*p=((*p>0x40) && (*p<0x5b)? *p|0x60 : *p);

		if (0 == manufacturer)
			{
			printf("Found HID device at /dev/%s\n",entry->d_name);
			printf("--- Vendor = %s",dev_vendor);
			printf("--- manufacturer = %s, device = %s\n",dev_manufacturer,dev_product);
			} // if 0 == manufacturer

		if(manufacturer!=0)
			if ((0==strcmp(manufacturer,dev_manufacturer) &&
				(0==strcmp(product,dev_product))))
				{
//				printf("##### Found ####\n");
	                        printf("Found HID device at /dev/%s\n",entry->d_name);
        	                printf("--- Vendor = %s",dev_vendor);
                	        printf("--- manufacturer = %s, device = %s\n",dev_manufacturer,dev_product);

				re = snprintf(hid_device->interface_device[hid_device->num_interfaces],PATH_MAX,"/dev/%s",entry->d_name);
				if(re>=PATH_MAX)
					{
					closedir(dh);
					return -7;
					} // if re
				
				hid_device->num_interfaces++;

				} // if (0==strcmp...
			} // if (found)
		} // End of new patch
	} // While 
	
	closedir(dh);
	
	if(manufacturer==0)
		return 0;
	
	if(hid_device->num_interfaces>0)
		return 0;
	return -8;
}

int open_hid_device(struct st_hid_device *hid_device)
{
	int i;

	
	hid_device->interface_fd_max = -1;
	for(i=0;i<hid_device->num_interfaces;i++)
	{
		hid_device->interface_fd[i] = open(hid_device->interface_device[i],O_RDONLY);
		if(hid_device->interface_fd[i]<0)
		{
			printf("Cannot Open device %s with code %d\n",hid_device->interface_device[i],hid_device->interface_fd[i]);
			close_hid_device(hid_device);
			return -1;
		}
		
		if(hid_device->interface_fd[i]>hid_device->interface_fd_max)
			hid_device->interface_fd_max = hid_device->interface_fd[i];
		
		
		printf("Opened HID interface on %s\n",hid_device->interface_device[i]);
	}
	
	return 0;
}

int close_hid_device(struct st_hid_device *hid_device)
{
	int i,exit_code;
	
	exit_code = 0;
	for(i=0;i<hid_device->num_interfaces;i++)
	{
		if(hid_device->interface_device[i]>=0)
		{
			if(close(hid_device->interface_fd[i])<0)
				exit_code = -1;
			else
				hid_device->interface_fd[i] = -1;
		}
	}
	
	return exit_code;
}

int read_hid_event(struct st_hid_device *hid_device,char *event,unsigned int *length)
{
	fd_set rfds;
	int i,re;
	
	FD_ZERO(&rfds);
	
	for(i=0;i<hid_device->interface_fd[i];i++)
		FD_SET(hid_device->interface_fd[i],&rfds);
	
	re = select(hid_device->interface_fd_max+1,&rfds,NULL,NULL,NULL);
	if(re<0)
		return -1;
	
	for(i=0;i<hid_device->interface_fd[i];i++)
	{
		if(FD_ISSET(hid_device->interface_fd[i],&rfds))
		{
			re = read(hid_device->interface_fd[i],event,*length);
			if(re<=0)
				return -1;

			*length = re;
		}
	}
	
	
	return 0;
}
