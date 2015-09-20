#ifndef CEPH_CRUSH_MAPPER_H
#define CEPH_CRUSH_MAPPER_H

/*
 * CRUSH functions for find rules and then mapping an input to an
 * output set.
 *
 * LGPL2
 */

#include <linux/crush/crush.h>

extern int crush_find_rule(const struct crush_map *map, int ruleset, int type, int size);
extern int crush_do_rule(const struct crush_map *map,
			 int ruleno,
			 int x, int *result, int result_max,
<<<<<<< HEAD
			 int forcefeed,    /* -1 for none */
=======
>>>>>>> v3.10.88
			 const __u32 *weights);

#endif
