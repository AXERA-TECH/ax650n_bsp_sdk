/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include<linux/input.h>
#include <string.h>


#define RTC_ALARM_PWR_MODE      _IOW('p', 0x21, unsigned int)

#define RTC_PWR_POWPER_ON        0X0
#define RTC_PWR_SHUTDOWN         0X1
#define RTC_PWR_SOFT_RESTART     0X2
#define RTC_PWR_HARD_RESTART     0X3


/*
* This expects the new RTC class driver framework, working with
* clocks that will often not be clones of what the PC-AT had.
* Use the command line to specify another RTC if you need one.
*/
static const char default_rtc[] = "/dev/rtc0";


int main(int argc, char **argv)
{
    int fd, retval, irqcount = 0;
    unsigned long data;
    struct rtc_time rtc_tm;
    const char *rtc = default_rtc;
    struct timeval tv_start, tv_end;
    int turn_mode;

    if(argc < 2){
        fprintf(stderr, "\n\t\t\t please see README.\n\n");
        return -1;
    }

    if(!strcmp(argv[1],"/dev/rtc0")) {
        fd = open("/dev/rtc0", O_RDONLY);
        if (fd ==  -1) {
            perror(rtc);
            return -1;
     }

    fprintf(stderr, "\n\t\t\tRTC Driver Test Example.\n\n");

    /* Read the RTC time/date */
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) {
        perror("RTC_RD_TIME ioctl");
        goto fail;
    }

    fprintf(stderr, "\n\nCurrent RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
            rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    /* Set the RTC time/date */
    rtc_tm.tm_sec += 10;
    rtc_tm.tm_min += 10;
    rtc_tm.tm_hour += 1;
    if (rtc_tm.tm_sec >= 60) {
        rtc_tm.tm_sec %= 60;
        rtc_tm.tm_min++;
    }
    if (rtc_tm.tm_min >= 60) {
        rtc_tm.tm_min %= 60;
        rtc_tm.tm_hour++;
    }
    if (rtc_tm.tm_hour >= 24) {
        rtc_tm.tm_hour = 0;
    }

    fprintf(stderr, "\n\nRTC date/time set to %d-%d-%d, %02d:%02d:%02d.\n",
            rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
    if (retval == -1) {
        perror("RTC_RD_TIME ioctl");
        goto fail;
    }

    /* Read the RTC time/date */
    retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
    if (retval == -1) {
        perror("RTC_RD_TIME ioctl");
        goto fail;
    }
	gettimeofday(&tv_start, NULL);
    fprintf(stderr, "\n\nAfter set current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
            rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    /* Set the alarm to 5 sec in the future, and check for rollover */
    rtc_tm.tm_sec += 15;

    if (rtc_tm.tm_sec >= 60) {
        rtc_tm.tm_sec %= 60;
        rtc_tm.tm_min++;
    }
    if (rtc_tm.tm_min >= 60) {
        rtc_tm.tm_min %= 60;
        rtc_tm.tm_hour++;
    }
    if (rtc_tm.tm_hour >= 24) {
        rtc_tm.tm_hour = 0;
    }

    if(argc == 3) {

	if(!strcmp(argv[2],"power_on")) {
		turn_mode = 0;
		ioctl(fd, RTC_ALARM_PWR_MODE, &turn_mode);
	} else if(!strcmp(argv[2],"shut_down")) {
		turn_mode = 1;
		ioctl(fd, RTC_ALARM_PWR_MODE, &turn_mode);
	} else if (!strcmp(argv[2],"soft_restart")) {
		turn_mode = 2;
		ioctl(fd, RTC_ALARM_PWR_MODE, &turn_mode);
	} else if (!strcmp(argv[2],"hard_restart")) {
		turn_mode = 3;
		ioctl(fd, RTC_ALARM_PWR_MODE, &turn_mode);
	}
    }

    retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
    if (retval == -1) {
        if (errno == ENOTTY) {
            fprintf(stderr,
                    "\n...Alarm IRQs not supported.\n");
        }
        perror("RTC_ALM_SET ioctl");
        goto fail;
    }

    /* Read the current alarm settings */
    retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
    if (retval == -1) {
        perror("RTC_ALM_READ ioctl");
        goto fail;
    }

    fprintf(stderr, "Alarm time now set to %02d:%02d:%02d.\n",
            rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

    /* Enable alarm interrupts */
    retval = ioctl(fd, RTC_AIE_ON, 0);
    if (retval == -1) {
        perror("RTC_AIE_ON ioctl");
        goto fail;
    }

    fprintf(stderr, "Waiting 15 seconds for alarm...\n");
    fflush(stderr);
    /* This blocks until the alarm ring causes an interrupt */
    retval = read(fd, &data, sizeof(unsigned long));
    if (retval == -1) {
        perror("read");
        goto fail;
    }
	gettimeofday(&tv_end, NULL);
    irqcount++;
    fprintf(stderr, " okay. Alarm rang. System elapsed: %ldms\n", (tv_end.tv_sec*1000 + tv_end.tv_usec/1000 - tv_start.tv_sec*1000 - tv_start.tv_usec/1000));

    /* Disable alarm interrupts */
    retval = ioctl(fd, RTC_AIE_OFF, 0);
    if (retval == -1) {
        perror("RTC_AIE_OFF ioctl");
        goto fail;
    }

    fprintf(stderr, "\n\n\t\t\t *** Test complete ***\n");

    close(fd);

    return 0;

	} else if(!strcmp(argv[1],"/dev/input/event0")) {

		int fd;
		struct input_event ev;
		fd = open("/dev/input/event0",O_RDWR);
		if (fd ==  -1) {
			perror(rtc);
			return -1;
		}

		while(1)
		{
			printf("start\n");
			read(fd,&ev,sizeof(ev));
			if(ev.type == EV_KEY)
				printf("code:%d,value:%d\n",ev.code,ev.value);
		}
		close(fd);
		return 0;

	}

fail:
    fprintf(stderr, "\n\n\t\t\t *** Test fail ***\n");



    return 0;
}
