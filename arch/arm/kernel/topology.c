/*
 * arch/arm/kernel/topology.c
 *
 * Copyright (C) 2011 Linaro Limited.
 * Written by: Vincent Guittot
 *
 * based on arch/sh/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_CMP_TGS
#include <linux/of.h>
#endif
#include <linux/sched.h>
#ifdef CONFIG_MTK_SCHED_CMP_TGS
#include <linux/slab.h>
#endif
=======
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/slab.h>

>>>>>>> v3.10.88
#include <asm/cputype.h>
#ifdef CONFIG_MTK_SCHED_CMP_TGS
#include <asm/smp_plat.h>
#endif
#include <asm/topology.h>

/*
 * cpu power scale management
 */

/*
 * cpu power table
 * This per cpu data structure describes the relative capacity of each core.
 * On a heteregenous system, cores don't have the same computation capacity
 * and we reflect that difference in the cpu_power field so the scheduler can
 * take this difference into account during load balance. A per cpu structure
 * is preferred because each CPU updates its own cpu_power field during the
 * load balance except for idle cores. One idle core is selected to run the
 * rebalance_domains for all idle cores and the cpu_power can be updated
 * during this sequence.
 */
static DEFINE_PER_CPU(unsigned long, cpu_scale);

unsigned long arch_scale_freq_power(struct sched_domain *sd, int cpu)
{
	return per_cpu(cpu_scale, cpu);
}

static void set_power_scale(unsigned int cpu, unsigned long power)
{
	per_cpu(cpu_scale, cpu) = power;
}

#ifdef CONFIG_OF
struct cpu_efficiency {
	const char *compatible;
	unsigned long efficiency;
};

/*
 * Table of relative efficiency of each processors
 * The efficiency value must fit in 20bit and the final
 * cpu_scale value must be in the range
 *   0 < cpu_scale < 3*SCHED_POWER_SCALE/2
 * in order to return at most 1 when DIV_ROUND_CLOSEST
 * is used to compute the capacity of a CPU.
 * Processors that are not defined in the table,
 * use the default SCHED_POWER_SCALE value for cpu_scale.
 */
struct cpu_efficiency table_efficiency[] = {
	{"arm,cortex-a15", 3891},
	{"arm,cortex-a7",  2048},
	{NULL, },
};

struct cpu_capacity {
	unsigned long hwid;
	unsigned long capacity;
};

struct cpu_capacity *cpu_capacity;

unsigned long middle_capacity = 1;

/*
 * Iterate all CPUs' descriptor in DT and compute the efficiency
 * (as per table_efficiency). Also calculate a middle efficiency
 * as close as possible to  (max{eff_i} - min{eff_i}) / 2
 * This is later used to scale the cpu_power field such that an
 * 'average' CPU is of middle power. Also see the comments near
 * table_efficiency[] and update_cpu_power().
 */
static void __init parse_dt_topology(void)
{
	struct cpu_efficiency *cpu_eff;
	struct device_node *cn = NULL;
	unsigned long min_capacity = (unsigned long)(-1);
	unsigned long max_capacity = 0;
	unsigned long capacity = 0;
	int alloc_size, cpu = 0;

	alloc_size = nr_cpu_ids * sizeof(struct cpu_capacity);
	cpu_capacity = kzalloc(alloc_size, GFP_NOWAIT);

	while ((cn = of_find_node_by_type(cn, "cpu"))) {
		const u32 *rate, *reg;
		int len;

		if (cpu >= num_possible_cpus())
			break;

		for (cpu_eff = table_efficiency; cpu_eff->compatible; cpu_eff++)
			if (of_device_is_compatible(cn, cpu_eff->compatible))
				break;

		if (cpu_eff->compatible == NULL)
			continue;

		rate = of_get_property(cn, "clock-frequency", &len);
		if (!rate || len != 4) {
			pr_err("%s missing clock-frequency property\n",
				cn->full_name);
			continue;
		}

		reg = of_get_property(cn, "reg", &len);
		if (!reg || len != 4) {
			pr_err("%s missing reg property\n", cn->full_name);
			continue;
		}

		capacity = ((be32_to_cpup(rate)) >> 20) * cpu_eff->efficiency;

		/* Save min capacity of the system */
		if (capacity < min_capacity)
			min_capacity = capacity;

		/* Save max capacity of the system */
		if (capacity > max_capacity)
			max_capacity = capacity;

		cpu_capacity[cpu].capacity = capacity;
		cpu_capacity[cpu++].hwid = be32_to_cpup(reg);
	}

	if (cpu < num_possible_cpus())
		cpu_capacity[cpu].hwid = (unsigned long)(-1);

	/* If min and max capacities are equals, we bypass the update of the
	 * cpu_scale because all CPUs have the same capacity. Otherwise, we
	 * compute a middle_capacity factor that will ensure that the capacity
	 * of an 'average' CPU of the system will be as close as possible to
	 * SCHED_POWER_SCALE, which is the default value, but with the
	 * constraint explained near table_efficiency[].
	 */
	if (min_capacity == max_capacity)
		cpu_capacity[0].hwid = (unsigned long)(-1);
	else if (4*max_capacity < (3*(max_capacity + min_capacity)))
		middle_capacity = (min_capacity + max_capacity)
				>> (SCHED_POWER_SHIFT+1);
	else
		middle_capacity = ((max_capacity / 3)
				>> (SCHED_POWER_SHIFT-1)) + 1;

}

/*
 * Look for a customed capacity of a CPU in the cpu_capacity table during the
 * boot. The update of all CPUs is in O(n^2) for heteregeneous system but the
 * function returns directly for SMP system.
 */
void update_cpu_power(unsigned int cpu, unsigned long hwid)
{
	unsigned int idx = 0;

	/* look for the cpu's hwid in the cpu capacity table */
	for (idx = 0; idx < num_possible_cpus(); idx++) {
		if (cpu_capacity[idx].hwid == hwid)
			break;

		if (cpu_capacity[idx].hwid == -1)
			return;
	}

	if (idx == num_possible_cpus())
		return;

	set_power_scale(cpu, cpu_capacity[idx].capacity / middle_capacity);

	printk(KERN_INFO "CPU%u: update cpu_power %lu\n",
		cpu, arch_scale_freq_power(NULL, cpu));
}

#else
static inline void parse_dt_topology(void) {}
static inline void update_cpu_power(unsigned int cpuid, unsigned int mpidr) {}
#endif

 /*
 * cpu topology table
 */
struct cputopo_arm cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

#ifdef CONFIG_MTK_SCHED_CMP_TGS
#define CPU_DOMAIN_BUGON
DEFINE_PER_CPU(struct cpu_domain *, cmp_cpu_domain) = {NULL};
spinlock_t cluster_lock;
LIST_HEAD(cpu_domains);
static unsigned int nr_clusters = 0;
static void init_cpu_domains(int cpuid, int socket_id);
#endif
#ifdef CONFIG_MTK_SCHED_CMP_PACK_SMALL_TASK
int arch_sd_share_power_line(void)
{
	return 0*SD_SHARE_POWERLINE;
}
#endif /* CONFIG_MTK_SCHED_CMP_PACK_SMALL_TASK */
const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

void update_siblings_masks(unsigned int cpuid)
{
	struct cputopo_arm *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->socket_id != cpu_topo->socket_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
	smp_wmb();
}

/*
 * store_cpu_topology is called at boot when only one cpu is running
 * and with the mutex cpu_hotplug.lock locked, when several cpus have booted,
 * which prevents simultaneous write access to cpu_topology array
 */
void store_cpu_topology(unsigned int cpuid)
{
	struct cputopo_arm *cpuid_topo = &cpu_topology[cpuid];
	unsigned int mpidr;

	/* If the cpu topology has been already set, just return */
	if (cpuid_topo->core_id != -1)
		return;

	mpidr = read_cpuid_mpidr();

	/* create cpu topology mapping */
	if ((mpidr & MPIDR_SMP_BITMASK) == MPIDR_SMP_VALUE) {
		/*
		 * This is a multiprocessor system
		 * multiprocessor format & multiprocessor mode field are set
		 */

		if (mpidr & MPIDR_MT_BITMASK) {
			/* core performance interdependency */
			cpuid_topo->thread_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 2);
		} else {
			/* largely independent cores */
			cpuid_topo->thread_id = -1;
			cpuid_topo->core_id = MPIDR_AFFINITY_LEVEL(mpidr, 0);
			cpuid_topo->socket_id = MPIDR_AFFINITY_LEVEL(mpidr, 1);
		}
	} else {
		/*
		 * This is an uniprocessor system
		 * we are in multiprocessor format but uniprocessor system
		 * or in the old uniprocessor format
		 */
		cpuid_topo->thread_id = -1;
		cpuid_topo->core_id = 0;
		cpuid_topo->socket_id = -1;
	}
<<<<<<< HEAD
#ifdef CONFIG_MTK_SCHED_CMP_TGS
	if(cpuid_topo->socket_id != -1)	
		init_cpu_domains(cpuid, cpuid_topo->socket_id);
#endif  
	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->socket_id == cpu_topo->socket_id) {
			cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
			if (cpu != cpuid)
				cpumask_set_cpu(cpu,
					&cpuid_topo->core_sibling);

			if (cpuid_topo->core_id == cpu_topo->core_id) {
				cpumask_set_cpu(cpuid,
					&cpu_topo->thread_sibling);
				if (cpu != cpuid)
					cpumask_set_cpu(cpu,
						&cpuid_topo->thread_sibling);
			}
		}
	}
	smp_wmb();
=======

	update_siblings_masks(cpuid);

	update_cpu_power(cpuid, mpidr & MPIDR_HWID_BITMASK);
>>>>>>> v3.10.88

	printk(KERN_INFO "CPU%u: thread %d, cpu %d, socket %d, mpidr %x\n",
		cpuid, cpu_topology[cpuid].thread_id,
		cpu_topology[cpuid].core_id,
		cpu_topology[cpuid].socket_id, mpidr);
}

/*
 * init_cpu_topology is called at boot when only one cpu is running
 * which prevent simultaneous write access to cpu_topology array
 */
void __init init_cpu_topology(void)
{
	unsigned int cpu;

	/* init core mask and power*/
	for_each_possible_cpu(cpu) {
		struct cputopo_arm *cpu_topo = &(cpu_topology[cpu]);

		cpu_topo->thread_id = -1;
		cpu_topo->core_id =  -1;
		cpu_topo->socket_id = -1;
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);

		set_power_scale(cpu, SCHED_POWER_SCALE);
	}
	smp_wmb();

	parse_dt_topology();
}
#ifdef CONFIG_MTK_SCHED_CMP_TGS
int cluster_id(int cpu)
{ 
	if((cpu >= 0) && (cpu < NR_CPUS) && (per_cpu(cmp_cpu_domain, cpu) != NULL))
		return cmp_cpu_domain(cpu)->cluster_id;
	else {
#ifdef CPU_DOMAIN_BUGON
		BUG_ON(1);
#endif  	
		return -1;
	}	
}
int nr_cpu_of_cluster(int cluster_id, bool exclusiveOffline)
{
	unsigned char found = 0;
	struct cpumask dst;
	struct	cpu_domain *cluster;
	struct list_head *pos;
	
	if(!list_empty(&cpu_domains))
	{
		list_for_each(pos, &cpu_domains) {
			cluster = list_entry(pos, struct cpu_domain, cpu_domains);
			if(cluster->cluster_id == cluster_id) //Found
			{
				found =1;
				break;
			}
		}
		if(!found) {
#ifdef CPU_DOMAIN_BUGON
			BUG_ON(1);
#endif 				
			return -1;
		}
		if(exclusiveOffline)
			dst = cluster->cpus;
		else
			dst = cluster->possible_cpus;
		return cpumask_weight(&dst);
	}
	else {
#ifdef CPU_DOMAIN_BUGON
		BUG_ON(1);
#endif 		
		return -1;
	}	 
}
unsigned int cluster_nr(void)
{
	return nr_clusters;
}
struct cpu_domain *get_cpu_domain(int cpu)
{
	if(per_cpu(cmp_cpu_domain, cpu) != NULL)
  	return cmp_cpu_domain(cpu);
  else
  	return NULL;
}

struct cpumask *get_domain_cpus(int cluster_id, bool exclusiveOffline)
{
	unsigned char found = 0;
	struct	cpu_domain *cluster;
	struct list_head *pos;
	
	if(!list_empty(&cpu_domains))
	{
		list_for_each(pos, &cpu_domains) {
			cluster = list_entry(pos, struct cpu_domain, cpu_domains);
			if(cluster->cluster_id == cluster_id) //Found
			{
				found =1;
				break;
			}
		}
		if(!found) {				
			return 0;
		}
		if(exclusiveOffline)
			return &cluster->cpus;
		else
			return &cluster->possible_cpus;
	}
	else {
#ifdef CPU_DOMAIN_BUGON
		BUG_ON(1);
#endif 		
		return 0;
	}
}

void init_cpu_domain_early(void)
{
	unsigned int mpidr;
	int socket_id;
	mpidr = read_cpuid_mpidr();

	if ((mpidr & MPIDR_SMP_BITMASK) == MPIDR_SMP_VALUE) {
		if (mpidr & MPIDR_MT_BITMASK) {
			socket_id = (mpidr >> MPIDR_LEVEL2_SHIFT)
				& MPIDR_LEVEL2_MASK;
		} else {
			/* largely independent cores */
			socket_id = (mpidr >> MPIDR_LEVEL1_SHIFT)
				& MPIDR_LEVEL1_MASK;
		}
	} else {
		socket_id = -1;
	}
	spin_lock_init (&cluster_lock);
	if(socket_id != -1)
		init_cpu_domains(smp_processor_id(), socket_id);
}
static void init_cpu_domains(int cpuid, int socket_id)
{
	
	struct	cpu_domain *cluster;
	struct list_head *pos;
	
	if(per_cpu(cmp_cpu_domain, cpuid) == NULL)
	{
		spin_lock(&cluster_lock);
		if(list_empty(&cpu_domains))
		{
			cluster = (struct cpu_domain *)
				kmalloc(sizeof(struct cpu_domain), GFP_KERNEL);
				memset(cluster, 0, sizeof(struct cpu_domain));
			cluster->cluster_id = socket_id;
			list_add(&cluster->cpu_domains, &cpu_domains);
			per_cpu(cmp_cpu_domain, cpuid) = cluster;
			cpumask_set_cpu(cpuid, &cluster->possible_cpus);
			cpumask_and(&cluster->cpus, cpu_online_mask, &cluster->possible_cpus);
			nr_clusters++;				
		}
		else
		{
			list_for_each(pos, &cpu_domains) {
				cluster = list_entry(pos, struct cpu_domain, cpu_domains);
				if(cluster->cluster_id == socket_id)
				{

					per_cpu(cmp_cpu_domain, cpuid) = cluster;
					cpumask_set_cpu(cpuid, &cluster->possible_cpus);
					cpumask_and(&cluster->cpus, cpu_online_mask, &cluster->possible_cpus);
					//printk("cpu%d cluster info is found\n", cpuid);
			  	break;
			  }
			}
			if(pos == &cpu_domains)//Not found, allocate one
			{
				cluster = (struct cpu_domain *)
				kmalloc(sizeof(struct cpu_domain), GFP_KERNEL);
				memset(cluster, 0, sizeof(struct cpu_domain));
				cluster->cluster_id = socket_id;
				list_add(&cluster->cpu_domains, &cpu_domains);
				per_cpu(cmp_cpu_domain, cpuid) = cluster;
				cpumask_set_cpu(cpuid, &cluster->possible_cpus);
				cpumask_and(&cluster->cpus, cpu_online_mask, &cluster->possible_cpus);
				nr_clusters++;
			}
		}
		spin_unlock(&cluster_lock);
	}	
}
#endif
