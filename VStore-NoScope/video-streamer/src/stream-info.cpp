//
// Created by xzl on 12/30/17.
//

#include <array>

#include "config.h"
#include "vs-types.h"

using namespace vs;

namespace vs {
	std::array<const char *, TYPE_INVALID + 1> data_type_str{
			"chunk",
			"raw_frame",
			"decoded_frame",

			"chunk_eof",
			"raw_frame_eof",
			"decoded_frame_eof",

			"invalid"
	};
}

static stream_desc const all_streams_[] = {
		{
				stream_id : 0,
				.db_path = DB_PATH,
				is_encoded: true,
				width : 320,
				height : 240,
				fps: 10,
				yuv_mode : -1,
				start : 0,
				// duration?
		},
		{
				stream_id : 1,
				db_path : {DB_RAW_FRAME_PATH},
				is_encoded: false,
				width : 320,
				height : 240,
				fps: 10,
				yuv_mode : 420,
				start : 0,
				// duration?
		},
#if 0
		/* last one must have stream_id == -1 */
		{
				.stream_id = -1
		}
#endif
};

/* ease of iteration, auto boundary check, etc. */
//std::array<stream_desc, sizeof(all_streams_)/sizeof(stream_desc)> sd;
std::vector<stream_desc> const sd(std::begin(all_streams_),
																	std::end(all_streams_));

#if 0 /* can't use designated init */
std::array<stream_desc, 2> as = {
		{
				stream_id : 0,
				.db_path = DB_PATH,
				is_encoded: true,
				width : 320,
				height : 240,
				fps: 10,
				yuv_mode : -1,
				start : 0,
				// duration?
		},
		{
				stream_id : 1,
				db_path : {DB_RAW_FRAME_PATH},
				is_encoded: false,
				width : 320,
				height : 240,
				fps: 10,
				yuv_mode : 420,
				start : 0,
				// duration?
		},
#if 0
/* last one must have stream_id == -1 */
		{
				.stream_id = -1
		}
#endif
};
#endif
