//
// Created by xzl on 12/25/17.
//

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
extern "C" {

#include <libavutil/mem.h>
}
#include "tensorflow/noscope/log.h"
#include "tensorflow/noscope/mm.h"


void my_free(void *data, void *hint)
{
	xzl_bug_on(!data || !hint);

	struct my_alloc_hint *h = (my_alloc_hint *)hint;

	switch (h->flag) {
		case USE_MALLOC:
			free(data);
			break;
		case USE_MMAP_REFCNT:
			if (h->refcnt.fetch_sub(1) != 1)
				return; /* do nothing. don't free @h yet */
			else { /* refcnt drops to zero, fall through */
				auto ret = munmap(h->base, h->length);
				xzl_bug_on(ret != 0);
			}
			break;
		case USE_LMDB_REFCNT:
			if (h->refcnt.fetch_sub(1) != 1)
				return; /* do nothing. don't free @h yet */
			else {
				W("close the tx");
				mdb_cursor_close(h->cursor);
				mdb_txn_abort(h->txn);
			}
			break;
		case USE_MMAP: {
			xzl_bug_on(data != h->base);
			auto ret = munmap(data, h->length);
			xzl_bug_on(ret != 0);
			break;
		}
		case USE_AVMALLOC:
			av_freep(&data);
			break;

		default:
			xzl_bug("unsupported?");
	}

	delete h; /* free hint -- too expensive? */
}


/* create mmap for the whole given file
 * @p: to be unmapped by caller */
void map_file(const char *fname, uint8_t **p, size_t *sz)
{
	/* get file sz */
	struct stat finfo;
	int fd = open(fname, O_RDONLY);
	xzl_bug_on_msg(fd < 0, "failed to open file");
	int ret = fstat(fd, &finfo);
	xzl_bug_on(ret != 0);

	uint8_t *buff = (uint8_t *)mmap(NULL, finfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	xzl_bug_on(buff == MAP_FAILED);

	*p = buff;
	*sz = finfo.st_size;
}
