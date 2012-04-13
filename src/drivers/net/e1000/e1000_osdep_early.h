// XXX fix header dependancies

#ifndef _E1000_OSDEP_EARLY_H_
#define _E1000_OSDEP_EARLY_H_

#define bool       boolean_t
#define dma_addr_t unsigned long
#define __le16     u16
#define __le32     u32
#define __le64     u64

#define __iomem

#define ETH_FCS_LEN 4

typedef enum {
    false = 0,
    true = 1
} boolean_t;

#endif
