//
// Created by xzl on 1/1/18.
//

#ifndef VIDEO_STREAMER_RX_MANAGER_H
#define VIDEO_STREAMER_RX_MANAGER_H

#include <memory>
#include <tuple>
#include <vector>
#include <map>

#include "tensorflow/noscope/vs-types.h"
#include "tensorflow/noscope/log.h"

#include <zmq.hpp>

//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/imgproc/imgproc.hpp"

//using namespace std;

namespace vs {

	/* used to manage recv'd frames yet to be consumed.
	 * assumption: there won't be too many frames
	 */
//	using Frame = std::tuple<vs::data_desc, std::shared_ptr<zmq::message_t>>;

	/* two level heaps. lv1: chunks; lv2: frames
	 *
	 * rationale: we'd like both quick insertion and quick iteration.
	 * considered but rejected: vector of vector, vector of deque, priority queues
	 */

	/* ------------- frames -------------- */

	struct Frame {
		vs::data_desc desc; /* keep this info as metadata (stream id, etc) */
		std::shared_ptr<zmq::message_t> msg_p;
	};

	template<class FrameT>
	struct FrameCompGt {
		bool operator() (FrameT const & lhs, FrameT const & rhs) const {
			/* since we are going to build a min heap .. */
			xzl_assert(lhs.desc.f_seq != rhs.desc.f_seq);
			return (lhs.desc.f_seq > rhs.desc.f_seq);
		}
	};

//	using frame_heap_t = priority_queue<Frame, vector<Frame>, FrameCmpGt<Frame>>;
	using frame_map_t = std::map<seq_t, Frame>;

	/* ---------- chunks ------------------ */

	struct Chunk {
		unsigned int c_seq; /* we don't need other things in chunk's desc */
		long eof_f = -1; /* eof of frames */
		static_assert(sizeof(eof_f) > sizeof(seq_t), "must > seq_t");

		frame_map_t frames; /* recv'd frames so far. note that eof seq won't enter this map */
	};

	template<class FrameT>
	struct ChunkCompGt {
		bool operator() (Chunk const & lhs, Chunk const & rhs) const {
			/* since we are going to build a min heap .. */
			xzl_assert(lhs.c_seq != rhs.c_seq);
			return (lhs.c_seq > rhs.c_seq);
		}
	};

	/* comp two frames based on their desc. first chunk seq, then frame seq */
	template<class FrameT>
	struct ChunkFrameCmpGt {
		bool operator() (FrameT const & lhs, FrameT const & rhs) const {
			/* since we are going to build a min heap .. */
			if (lhs.desc.c_seq == rhs.msg.c_seq)
				return (lhs.desc.f_seq > rhs.desc.f_seq);
			else
				return (lhs.desc.c_seq > rhs.desc.c_seq);
		}
	};

//	using chunk_heap_t = priority_queue<Chunk, vector<Chunk>, ChunkCmpGt<Chunk>>;
	using chunk_map_t = std::map<seq_t, Chunk>;

	class RxManager {

		/* manage all incoming frames that belong to a given stream.
		 * organize the frames and reveal them in order to the consumer.
		 */

//		std::priority_queue<Frame, vector<Frame>, ChunkFrameCmpGt<Frame>> queue;

	private:
		chunk_map_t chunks; /* recv'd chunks so far. chunk eof won't enter the map */

		/* chunk eof */
		long eof_c = -1;
		static_assert(sizeof(eof_c) > sizeof(seq_t), "must > seq_t");

		/* watermarks: the next frame that the consumer shall read.
		 * may point to eof {frame, chunk} */
		seq_t wm_chunk = 0, wm_frame = 0;

		seq_t stream_id = 0xffff; /* shall check against any incoming frames */

	public:
		int
		DepositAFrame(data_desc const & fdesc, std::shared_ptr<zmq::message_t> const & msg_p) {

			auto & c_seq = fdesc.c_seq;
			auto & f_seq = fdesc.f_seq;

			xzl_bug_on(!is_type_frame(fdesc.type));

			/* new chunk */
			if (chunks.find(c_seq) == chunks.end()) {
				xzl_bug_on_msg(eof_c >= 0 && c_seq >= eof_c, "exceed chunk eof");
				chunks[c_seq].c_seq = c_seq; /* will auto create a new @Chunk (and its frame map)*/
			}

			/* chunk exists */
			auto & frame_map = chunks[c_seq].frames;
			auto & eof_f = chunks[c_seq].eof_f;
			xzl_bug_on_msg(eof_f >= 0 && f_seq >= eof_f, "exceed frame eof?");
			xzl_bug_on_msg(frame_map.find(f_seq) != frame_map.end(), "dup frames?");

			VV("insert %u", f_seq);
			frame_map.emplace(f_seq, Frame {.desc = fdesc, .msg_p = msg_p});

			return 0;
		}

		/* handling various eof */
		void DepositADesc(data_desc const &desc) {
			xzl_bug_on(!is_type_eof(desc.type));

			if (desc.type == TYPE_CHUNK_EOF) {
				xzl_bug_on_msg(eof_c >= 0, "dup chunk eof?");
				xzl_bug_on_msg(chunks.rbegin() != chunks.rend() && desc.c_seq <= chunks.rbegin()->first,
											 "eof smaller than existing last chunk seq?");
				eof_c = desc.c_seq;
				return;
			}

			if (desc.type == TYPE_DECODED_FRAME_EOF ||
					desc.type == TYPE_RAW_FRAME_EOF) {
				/* new chunk -- meaning frame eof arrives before any frames in that chunk. ever possible? */
				auto it_chunk = chunks.find(desc.c_seq);
				xzl_bug_on_msg(it_chunk == chunks.end(), "tbd");

				/* chunk exists */
				auto & chunk = it_chunk->second;
				auto & frame_map = chunk.frames;
				xzl_bug_on_msg(chunk.eof_f >= 0, "already frame eof?");
				VV("%u %zu", frame_map.rbegin()->first, frame_map.size());
				xzl_bug_on_msg(desc.f_seq <= frame_map.rbegin()->first,
											 "eof smaller than existing frame seq?");
				chunk.eof_f = desc.f_seq;
				return;
			}

			xzl_bug("unknown desc type");
		}

		/* get and remove a frame from the manager.
		 * will update watermarks accordingly.
		 *
		 * @frame: [out]
		 *
		 * return 0: ok
		 * -1: no more chunks (we're done)
		 * -2: no more; there should be more.
		 */
		int RetrieveAFrame(Frame * f) {
		again:
			if (wm_chunk == eof_c) { /* we're done! */
				xzl_bug_on(wm_frame != 0);
				return VS_ERR_EOF_CHUNKS;
			}

			auto it_chunk = chunks.find(wm_chunk);
			if (it_chunk == chunks.end()) {	/* chunk not arrived yet */
				xzl_bug_on(wm_frame != 0);
				return VS_ERR_AGAIN;
			}

			auto & chunk = chunks[wm_chunk];
			auto & frames = chunks[wm_chunk].frames;

			if (wm_frame == chunk.eof_f) { /* wm points to frame eof of a chunk. advance to next chunk */
				xzl_assert(chunk.eof_f >= 0);
				wm_chunk ++;
				wm_frame = 0;
				chunks.erase(it_chunk);
				goto again;
			} else {
				auto it = frames.find(wm_frame);
				if (it == frames.end()) {
					/* next frame has not arrived yet.
					 * wm may point to a future eof_f, or a future frame */
					return VS_ERR_AGAIN;
				}

				/* frame exists */
				f->desc = it->second.desc;
				f->msg_p = it->second.msg_p;

				wm_frame ++;
				frames.erase(it);
			}

			return 0;
		}

		void GetWatermarks(seq_t * chunk, seq_t * frame) {
			*chunk = this->wm_chunk;
			*frame = this->wm_frame;
		}

	};
};

#endif //VIDEO_STREAMER_RX_MANAGER_H
