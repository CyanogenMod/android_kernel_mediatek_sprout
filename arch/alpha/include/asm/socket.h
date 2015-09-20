#ifndef _ASM_SOCKET_H
#define _ASM_SOCKET_H

#include <uapi/asm/socket.h>

#ifdef __KERNEL__
/* O_NONBLOCK clashes with the bits used for socket types.  Therefore we
 * have to define SOCK_NONBLOCK to a different value here.
 */
#define SOCK_NONBLOCK	0x40000000
<<<<<<< HEAD
#endif /* __KERNEL__ */

=======
>>>>>>> v3.10.88
#endif /* _ASM_SOCKET_H */
