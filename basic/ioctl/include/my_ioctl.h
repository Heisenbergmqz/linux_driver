#ifndef _MY_IOCTL_H
#define _MY_IOCTL_H

#define     IOC_MAGIC   'c'
#define     CLEAR       _IO(IOC_MAGIC, 0)
#define     SET_VALUE   _IOW(IOC_MAGIC, 1, int)
#define     GET_VALUE   _IOR(IOC_MAGIC, 2, int)     

#endif
