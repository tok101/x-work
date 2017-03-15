#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

/*
 * This expects the new RTC class driver framework, working with
 * clocks that will often not be clones of what the PC-AT had.
 * Use the command line to specify another RTC if you need one.
 */
static const char default_rtc[] = "/dev/rtc0";

static int rtc_set_alarm_test(int fd)
{
	int retval;
	unsigned long data;
	struct rtc_time rtc_tm;

	/* Read the RTC time/date */
	retval = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	if (retval == -1) {
		perror("RTC_RD_TIME ioctl");
		return -1;
	}

	printf("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
	       rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
	       rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	/* Set the alarm to 60 sec in the future, and check for rollover */
	rtc_tm.tm_sec += 60;
	if (rtc_tm.tm_sec >= 60) {
		rtc_tm.tm_sec %= 60;
		rtc_tm.tm_min++;
	}
	if (rtc_tm.tm_min == 60) {
		rtc_tm.tm_min = 0;
		rtc_tm.tm_hour++;
	}
	if (rtc_tm.tm_hour == 24)
		rtc_tm.tm_hour = 0;


	retval = ioctl(fd, RTC_ALM_SET, &rtc_tm);
	if (retval == -1) {
		if (errno == ENOTTY) {
			printf("Alarm IRQS not supported.\n");
			return 0;
		}
		perror("RTC_ALM_SET ioctl");
		return -1;
	}

	/* Read the current alarm settings */
	retval = ioctl(fd, RTC_ALM_READ, &rtc_tm);
	if (retval == -1) {
		perror("RTC_ALM_READ ioctl");
		return -1;
	}

	/* Enable alarm interrupts */
	retval = ioctl(fd, RTC_AIE_ON, 0);
	if (retval == -1) {
		perror("RTC_AIE_ON ioctl");
		return -1;
	}

	printf("Alarm time now set to %02d:%02d:%02d.\n",
	       rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

	printf("System enter poweroff!\n");
	system("poweroff");

	return 0;
}

static int rtc_set_wake_alarm_test(int fd)
{
        int retval;
        unsigned long data;
        struct rtc_wkalrm wkalrm;

        wkalrm.enabled = 0;
        wkalrm.pending = 0;
        /* Read the RTC time/date */
        retval = ioctl(fd, RTC_RD_TIME, &wkalrm.time);
        if (retval == -1) {
                perror("RTC_RD_TIME ioctl");
                return -1;
        }

        printf("Current RTC date/time is %d-%d-%d, %02d:%02d:%02d.\n",
               wkalrm.time.tm_year + 1900, wkalrm.time.tm_mon + 1, wkalrm.time.tm_mday,
               wkalrm.time.tm_hour, wkalrm.time.tm_min, wkalrm.time.tm_sec);

        /* Set the alarm to 60 sec in the future, and check for rollover */
        wkalrm.time.tm_sec += 60;
        if (wkalrm.time.tm_sec >= 60) {
                wkalrm.time.tm_sec %= 60;
                wkalrm.time.tm_min++;
        }
        if (wkalrm.time.tm_min == 60) {
                wkalrm.time.tm_min = 0;
                wkalrm.time.tm_hour++;
        }
        if (wkalrm.time.tm_hour == 24)
                wkalrm.time.tm_hour = 0;


        retval = ioctl(fd, RTC_WKALM_SET, &wkalrm);
        if (retval == -1) {
                if (errno == ENOTTY) {
                        printf("Alarm IRQS not supported.\n");
                        return 0;
                }
                perror("RTC_WKALM_SET ioctl");
                return -1;
        }
        /* Read the current alarm settings */
        retval = ioctl(fd, RTC_WKALM_RD, &wkalrm);
        if (retval == -1) {
                perror("RTC_WKALM_RD ioctl");
                return -1;
        }

        /* Enable alarm interrupts */
        retval = ioctl(fd, RTC_AIE_ON, 0);
        if (retval == -1) {
                perror("RTC_AIE_ON ioctl");
                return -1;
        }

        printf("Alarm time now set to %02d:%02d:%02d.\n",
               wkalrm.time.tm_hour, wkalrm.time.tm_min, wkalrm.time.tm_sec);

        printf("System enter poweroff!\n");
        system("poweroff");

        return 0;

}

int main(int argc, char **argv)
{
	int opt = 0, fd;
	const char *rtc = default_rtc;

	switch (argc) {
        case 2:
		rtc = argv[1];
		/* FALLTHROUGH */
        case 1:
		break;
        default:
		fprintf(stderr, "usage: rtc-wakeup-test [rtcdev]/n");
		return 1;
	}

	printf("======== %s: RTC Wakeup Test. ========\n", rtc);
	fd = open(rtc, O_RDONLY);
	if (fd ==  -1) {
		perror(rtc);
		exit(errno);
	}
#if 0
	printf("\n====> Set alarm <====\n");
	if (rtc_set_alarm_test(fd))
		goto err;
#else
	printf("\n====> Set wake alarm <====\n");
	if (rtc_set_wake_alarm_test(fd))
		goto err;

#endif
	printf("======== Test complete ========\n");
	close(fd);
	return 0;

err:
	printf("[ERROR] Test Fail!!!\n");
	close(fd);
	return -1;
}
