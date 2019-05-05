//
// Created by xzl on 12/27/17.
//
/*
 g++ lmdb-read.cpp -o lmdb-read.bin -I../ -llmdb -std=c++11 -O0 -Wall -g

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#include "log.h"

#include "lmdb.h"

#define LMDB_PATH "/shared/videos/lmdb/"
#define LMDB_RAWFRAME_PATH "/shared/videos/lmdb-rf/"

void test_one_env()
{
	int rc;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key, data;
	MDB_txn *txn;
	MDB_stat mst;
	MDB_cursor *cursor, *cur2;
	MDB_cursor_op op;


	rc = mdb_env_create(&env);
	xzl_bug_on(rc != 0);

	rc = mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
	xzl_bug_on(rc != 0);

	rc = mdb_env_open(env, LMDB_PATH, 0, 0664);
	xzl_bug_on(rc != 0);

	rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
	xzl_bug_on(rc != 0);

	rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
	xzl_bug_on(rc != 0);

	/* dump all key/v */
	rc = mdb_cursor_open(txn, dbi, &cursor);
	xzl_bug_on(rc != 0);

	while (1) {
		rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
		if (rc == MDB_NOTFOUND)
			break;
		I("got one k/v. key: %lu, sz %zu data sz %zu",
			*(uint64_t *)key.mv_data, key.mv_size,
			data.mv_size);

//		printf("key: %p %.*s, data: %p %.*s\n",
//					 key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
//					 data.mv_data, (int) data.mv_size, (char *) data.mv_data);
	}

	mdb_cursor_close(cursor);
	mdb_txn_abort(txn);

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);
}

/* open multi env at the same time */
void test_multi_env()
{
	int rc;
	MDB_env *env, *env2;
	MDB_dbi dbi, dbi2;

	MDB_val key, data;
	MDB_txn *txn;
	MDB_stat mst;
	MDB_cursor *cursor, *cur2;

	{
		rc = mdb_env_create(&env);
		xzl_bug_on(rc != 0);

		rc = mdb_env_set_mapsize(env, 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
		xzl_bug_on(rc != 0);

		rc = mdb_env_open(env, LMDB_PATH, 0, 0664);
		xzl_bug_on(rc != 0);

		rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
		xzl_bug_on(rc != 0);

		rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi);
		xzl_bug_on(rc != 0);

		rc = mdb_txn_commit(txn);
		xzl_bug_on(rc != 0);
	}

	{
		rc = mdb_env_create(&env2);
		xzl_bug_on(rc != 0);

		rc = mdb_env_set_mapsize(env2, 1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
		xzl_bug_on(rc != 0);

		rc = mdb_env_open(env2, LMDB_RAWFRAME_PATH, 0, 0664);
		xzl_bug_on(rc != 0);

		rc = mdb_txn_begin(env2, NULL, MDB_RDONLY, &txn);
		xzl_bug_on(rc != 0);

		rc = mdb_dbi_open(txn, NULL, MDB_INTEGERKEY, &dbi2);
		xzl_bug_on(rc != 0);

		rc = mdb_txn_commit(txn);
		xzl_bug_on(rc != 0);
	}

	W("load from lmdb");
	
	/* db 1 */
	{
		rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
		xzl_bug_on(rc != 0);

		/* dump all key/v */
		rc = mdb_cursor_open(txn, dbi, &cursor);
		xzl_bug_on(rc != 0);

		while (1) {
			rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
			if (rc == MDB_NOTFOUND)
				break;
			I("got one k/v. key: %lu, sz %zu data sz %zu",
				*(uint64_t *)key.mv_data, key.mv_size,
				data.mv_size);
		}

		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
	}

	W("load from lmdb-rf");

	/* db 2 */
	{
		rc = mdb_txn_begin(env2, NULL, MDB_RDONLY, &txn);
		xzl_bug_on(rc != 0);

		/* dump all key/v */
		rc = mdb_cursor_open(txn, dbi2, &cursor);
		xzl_bug_on(rc != 0);

		while (1) {
			rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT);
			if (rc == MDB_NOTFOUND)
				break;
			I("got one k/v. key: %lu, sz %zu data sz %zu",
				*(uint64_t *)key.mv_data, key.mv_size,
				data.mv_size);
		}

		mdb_cursor_close(cursor);
		mdb_txn_abort(txn);
	}

	/* clean up */
	mdb_dbi_close(env, dbi);
	mdb_env_close(env);

	mdb_dbi_close(env2, dbi2);
	mdb_env_close(env2);

}

int main() {

//	test_one_env();

	test_multi_env();
}
