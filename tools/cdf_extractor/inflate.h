#ifndef INFLATE_H__
#define INFLATE_H__



#include "typedefs.h"



#ifdef __cplusplus
extern "C"
{
#endif



    u32 inflate_data(u8 *, u8 *, u32);
    u32 inflate_data_bounded(u8 *, u32, u8 *, u32, int *);



#ifdef __cplusplus
}
#endif



#endif
