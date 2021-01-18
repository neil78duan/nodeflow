/* file logicEndian.h
 *
 * define logic parser byte endian 
 *
 * create by duan 
 *
 * 2016-8-15
 */

 #ifndef _LOGIC_ENDIAN_H_
 #define _LOGIC_ENDIAN_H_
 

#include "nd_common/nd_common.h"
#include "nd_net/byte_order.h"


typedef char *lp_stream_t;

//read data from little-endian-stream
static inline lp_stream_t lp_read_stream(lp_stream_t addr, NDUINT8 &val, int streamOrder = ND_L_ENDIAN)
{
	val = (NDUINT8) (*addr);
	return addr + 1;
}

static inline lp_stream_t lp_read_stream(lp_stream_t addr, NDUINT16 &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[1];
		p[1] = addr[0];
		addr += sizeof(NDUINT16);
	}
	return addr;
}

static inline lp_stream_t lp_read_stream(lp_stream_t addr, NDUINT32 &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[3];
		p[1] = addr[2];
		p[2] = addr[1];
		p[3] = addr[0];
		addr += sizeof(NDUINT32);
	}
	return addr;
}

static inline lp_stream_t lp_read_stream(lp_stream_t addr, int &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[3];
		p[1] = addr[2];
		p[2] = addr[1];
		p[3] = addr[0];
		addr += sizeof(int);
	}
	return addr;
}

static inline lp_stream_t lp_read_stream(lp_stream_t addr, NDUINT64 &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[7];
		p[1] = addr[6];
		p[2] = addr[5];
		p[3] = addr[4];
		p[4] = addr[3];
		p[5] = addr[2];
		p[6] = addr[1];
		p[7] = addr[0];
		addr += sizeof(NDUINT64);
	}
	return addr;
}

static inline lp_stream_t lp_read_stream(lp_stream_t addr, float &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	return addr;
}
static inline lp_stream_t lp_read_stream(lp_stream_t addr, double &val, int streamOrder = ND_L_ENDIAN)
{
	unsigned char *p = (unsigned char*)&val;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	return addr;
}

static inline lp_stream_t lp_write_stream(lp_stream_t p, NDUINT8 val, int )
{
	*p++ = val;
	return p;
}

static inline lp_stream_t lp_write_stream(lp_stream_t p, NDUINT16 val, int streamOrder)
{
	unsigned char *addr = (unsigned char *)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[1];
		p[1] = addr[0];
		p += sizeof(NDUINT16);
	}
	return p;
}
static inline lp_stream_t lp_write_stream(lp_stream_t p, NDUINT32 val, int streamOrder)
{
	unsigned char *addr = (unsigned char *)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[3];
		p[1] = addr[2];
		p[2] = addr[1];
		p[3] = addr[0];
		p += sizeof(NDUINT32);
	}
	return p;
}
static inline lp_stream_t lp_write_stream(lp_stream_t p, int val, int streamOrder)
{
	unsigned char *addr = (unsigned char *)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[3];
		p[1] = addr[2];
		p[2] = addr[1];
		p[3] = addr[0];
		p += sizeof(int);
	}
	return p;
}
static inline lp_stream_t lp_write_stream(lp_stream_t p, NDUINT64 val, int streamOrder)
{
	unsigned char *addr = (unsigned char *)&val;

	if (nd_byte_order() == streamOrder) {
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
		*p++ = *addr++;
	}
	else {
		p[0] = addr[7];
		p[1] = addr[6];
		p[2] = addr[5];
		p[3] = addr[4];
		p[4] = addr[3];
		p[5] = addr[2];
		p[6] = addr[1];
		p[7] = addr[0];
		p += sizeof(NDUINT64);
	}
	return p;
}

static inline lp_stream_t lp_write_stream(lp_stream_t p, float val, int )
{
	unsigned char *addr = (unsigned char*)&val;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	return p;
}
static inline lp_stream_t lp_write_stream(lp_stream_t p, double val, int )
{
	unsigned char *addr = (unsigned char*)&val;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;

	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	*p++ = *addr++;
	return p;
}

////////////////////////// little to host
static inline int lp_little_2cur(int val)
{
	if (nd_byte_order() == ND_L_ENDIAN) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT16 lp_little_2cur(NDUINT16 val)
{
	if (nd_byte_order() == ND_L_ENDIAN) {
		return val;
	}
	return nd_order_change_short(val);
}
static inline NDUINT32 lp_little_2cur(NDUINT32 val)
{
	if (nd_byte_order() == ND_L_ENDIAN) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT64 lp_little_2cur(NDUINT64 val)
{
	if (nd_byte_order() == ND_L_ENDIAN) {
		return val;
	}
	return nd_order_change_longlong(val);
}

//// big 2 host

static inline int lp_big_2cur(int val)
{
	if (nd_byte_order() == ND_B_ENDIAN) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT16 lp_big_2cur(NDUINT16 val)
{
	if (nd_byte_order() == ND_B_ENDIAN) {
		return val;
	}
	return nd_order_change_short(val);
}
static inline NDUINT32 lp_big_2cur(NDUINT32 val)
{
	if (nd_byte_order() == ND_B_ENDIAN) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT64 lp_big_2cur(NDUINT64 val)
{
	if (nd_byte_order() == ND_B_ENDIAN) {
		return val;
	}
	return nd_order_change_longlong(val);
}


/// host->stream
static inline int lp_order_2aim(int val, int outOrder)
{
	if (nd_byte_order() == outOrder) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT16 lp_order_2aim(NDUINT16 val, int outOrder)
{
	if (nd_byte_order() == outOrder) {
		return val;
	}
	return nd_order_change_short(val);
}
static inline NDUINT32 lp_order_2aim(NDUINT32 val, int outOrder)
{
	if (nd_byte_order() == outOrder) {
		return val;
	}
	return nd_order_change_long(val);
}
static inline NDUINT64 lp_order_2aim(NDUINT64 val, int outOrder)
{
	if (nd_byte_order() == outOrder) {
		return val;
	}
	return nd_order_change_longlong(val);
}

#define lp_host2stream lp_order_2aim 

#define lp_stream2host lp_order_2aim 

#endif
 