#ifndef _LINUX_ICMPV6_H
#define _LINUX_ICMPV6_H

#include <linux/skbuff.h>
#include <uapi/linux/icmpv6.h>

static inline struct icmp6hdr *icmp6_hdr(const struct sk_buff *skb)
{
	return (struct icmp6hdr *)skb_transport_header(skb);
}

<<<<<<< HEAD
#define ICMPV6_NI_QUERY			139
#define ICMPV6_NI_REPLY			140

#define ICMPV6_MLD2_REPORT		143

#define ICMPV6_DHAAD_REQUEST		144
#define ICMPV6_DHAAD_REPLY		145
#define ICMPV6_MOBILE_PREFIX_SOL	146
#define ICMPV6_MOBILE_PREFIX_ADV	147

/*
 *	Codes for Destination Unreachable
 */
#define ICMPV6_NOROUTE			0
#define ICMPV6_ADM_PROHIBITED		1
#define ICMPV6_NOT_NEIGHBOUR		2
#define ICMPV6_ADDR_UNREACH		3
#define ICMPV6_PORT_UNREACH		4
#define ICMPV6_POLICY_FAIL		5
#define ICMPV6_REJECT_ROUTE		6

/*
 *	Codes for Time Exceeded
 */
#define ICMPV6_EXC_HOPLIMIT		0
#define ICMPV6_EXC_FRAGTIME		1

/*
 *	Codes for Parameter Problem
 */
#define ICMPV6_HDR_FIELD		0
#define ICMPV6_UNK_NEXTHDR		1
#define ICMPV6_UNK_OPTION		2

/*
 *	constants for (set|get)sockopt
 */

#define ICMPV6_FILTER			1

/*
 *	ICMPV6 filter
 */

#define ICMPV6_FILTER_BLOCK		1
#define ICMPV6_FILTER_PASS		2
#define ICMPV6_FILTER_BLOCKOTHERS	3
#define ICMPV6_FILTER_PASSONLY		4

struct icmp6_filter {
	__u32		data[8];
};
=======
#include <linux/netdevice.h>
>>>>>>> v3.10.88

#if IS_ENABLED(CONFIG_IPV6)
extern void icmpv6_send(struct sk_buff *skb, u8 type, u8 code, __u32 info);

typedef void ip6_icmp_send_t(struct sk_buff *skb, u8 type, u8 code, __u32 info);
extern int inet6_register_icmp_sender(ip6_icmp_send_t *fn);
extern int inet6_unregister_icmp_sender(ip6_icmp_send_t *fn);

#else

static inline void icmpv6_send(struct sk_buff *skb,
			       u8 type, u8 code, __u32 info)
{

}
#endif

extern int				icmpv6_init(void);
extern int				icmpv6_err_convert(u8 type, u8 code,
							   int *err);
extern void				icmpv6_cleanup(void);
extern void				icmpv6_param_prob(struct sk_buff *skb,
							  u8 code, int pos);

struct flowi6;
struct in6_addr;
extern void				icmpv6_flow_init(struct sock *sk,
							 struct flowi6 *fl6,
							 u8 type,
							 const struct in6_addr *saddr,
							 const struct in6_addr *daddr,
							 int oif);
#endif
