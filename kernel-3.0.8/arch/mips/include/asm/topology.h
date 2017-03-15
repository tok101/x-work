/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2007 by Ralf Baechle
 */
#ifndef __ASM_TOPOLOGY_H
#define __ASM_TOPOLOGY_H

#include <topology.h>

#ifdef CONFIG_SMP
#define smt_capable()   (smp_num_siblings > 1)
/* FIXME: cpu_sibling_map is not a per_cpu variable */
/*#define topology_thread_cpumask(cpu)    (&per_cpu(cpu_sibling_map, cpu)) */
#define topology_thread_cpumask(cpu)    (&cpu_sibling_map[cpu])
#endif

#endif /* __ASM_TOPOLOGY_H */
