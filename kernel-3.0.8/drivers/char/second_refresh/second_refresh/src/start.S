/*
 * start.S
 *
 * Entry point of the Voice Wakeup Module.
 * The firmware code are executed in the L2Cache.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 *
 */

	.text

	.globl	start

	.extern open
	.extern handler
	.extern close
	.extern cache_prefetch
	.extern set_handler
start:
	.word open
	.word handler
	.word close
	.word cache_prefetch
	.word set_handler
