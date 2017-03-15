/*
 * u_audio.h -- interface to USB gadget "ALSA AUDIO" utilities
 *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __U_AUDIO_H
#define __U_AUDIO_H

#include <linux/device.h>
#include <linux/err.h>
#include <linux/usb/audio.h>
#include <linux/usb/composite.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "gadget_chips.h"

#define CONFIG_CAP_MIC
/*
 * This represents the USB side of an audio card device, managed by a USB
 * function which provides control and stream interfaces.
 */

#define MIC_SATE	16000
#define MIC_CH		1
#define SPK_SATE	48000
#define SPK_CH		2


struct gaudio_snd_dev {
	struct gaudio			*card;
	struct file			*filp;
	struct snd_pcm_substream	*substream;
	int				access;
	int				format;
	int				channels;
	int				rate;
};

struct gaudio {
	struct usb_function		func;
	struct usb_gadget		*gadget;

	/* ALSA sound device interfaces */
	struct gaudio_snd_dev		control;
	struct gaudio_snd_dev		playback;
	struct gaudio_snd_dev		capture;

	/* TODO */
};

struct f_audio_buf {
	u8 *buf;
	int actual;
	struct list_head list;
};

struct f_audio {
	struct gaudio			card;

	/* endpoints handle full and/or high speeds */
	struct usb_ep			*out_ep;
	struct usb_endpoint_descriptor	*out_desc;

	spinlock_t			lock;
	struct f_audio_buf *copy_buf;
	struct work_struct playback_work;
	struct list_head play_queue;
	int out_online;
	atomic_t  interface_conn;

	struct list_head out_reqs_list;

#if defined(CONFIG_CAP_MIC) || defined(CONFIG_CAP_FILE)
	struct usb_ep			*in_ep;
	struct usb_endpoint_descriptor	*in_desc;
	int in_online;
#endif

#if defined(CONFIG_CAP_FILE)
	/* abort record */
	spinlock_t audio_list_lock;
	struct list_head idle_reqs;
	struct list_head audio_data_list;
	volatile u8  mic_disconn;

	wait_queue_head_t online_wq;
	wait_queue_head_t write_wq;
	wait_queue_head_t read_wq;
	atomic_t open_excl;
	atomic_t write_excl;
	atomic_t read_excl;
	struct usb_request * cur_req;
#elif defined(CONFIG_CAP_MIC)
	struct task_struct *capture_thread;
	spinlock_t cb_list_lock;
	struct list_head cb_list_filled;
	struct list_head cb_list_free;
	struct list_head cb_list_queued;
#endif

	/* Control Set command */
	struct list_head cs;
	u8 set_cmd;
	struct usb_audio_control *set_con;
};

int gaudio_setup(struct f_audio *);
void gaudio_cleanup(void);
#define CONFIG_ANDROID

#endif /* __U_AUDIO_H */
