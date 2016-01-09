#ifndef _LINUX_POCKET_MOD_H
#define _LINUX_POCKET_MOD_H

extern int is_screen_on;
extern char alsps_dev;

int tmd2771_pocket_detection_check(void);
int cm36283_pocket_detection_check(void);

int device_is_pocketed(void);

#endif //_LINUX_POCKET_MOD_H
