/*
 * lowlevel/asynch_io/aio_tools.h
 * created: 2007-Apr-23 PW
 * $ZEL: aio_tools.h,v 1.2 2007/11/03 16:00:23 wuestner Exp $
 */

#ifndef _aio_tools_h_
#define _aio_tools_h_

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

int async_init(const char *fname, int flags, int perm,
        size_t bufsize, int bufnum, void **handle);
int async_done(void **handle);
ssize_t async_write(void *handle, const void *data, size_t count);
ssize_t async_read(void *handle, void **buf, size_t count);

size_t async_defbufsize(void);
int    async_defbufnum(void);

/*
async_init:

async_init opens the file 'fname' for read or write (depending on 'flags')
flags should be O_WRONLY|O_DIRECT[|O_CREAT|O_TRUNC] for write
and O_RDONLY|O_DIRECT for read
if O_DIRECT is ommitted the page cache is used (not recommended)
'perm' are permission bits of the file if it will be created
'bufnum' is number of buffers used for asynchronous IO
'bufsize' is the size of each buffer, it is rounded up to a multiple of
REC_XFER_ALIGN
'handle' is a pointer to an opaque structure used for all further calls

async_done:

wait for completion of all IO activities, closes the file and releases
all used recources

async_write:

like write(2)
waits until a buffer is available and queues the request
it returns the number of bytes written or -1 in case of error. errno should
be set correctly but this is not tested yet
O_NONBLOCK or O_NDELAY: (not yet implemented)
async_write will only queue the data which it can queue without blocking.

async_read:

like read(2)
waits until some data are available and return a pointer to these data in 
*buf
The data have to be used (or copied) before the next call to async_read
O_NONBLOCK or O_NDELAY: (not yet implemented)
if no data are ready async_read does not wait and will return 0
*/

#endif
