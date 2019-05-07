//
// Created by xzl on 12/29/17.
//
// refactor out of project creek

#ifndef VIDEO_STREAMER_STATCOLLECTOR_H
#define VIDEO_STREAMER_STATCOLLECTOR_H

#include <memory>

/* for time measurement */
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "CallBackTimer.h"

using namespace std;

/*
 * statistics
 */
struct Statstics {
//	const char * name;
	double byte_persec, rec_persec; /* byte per sec, rec per sec, in the last check interval */
	double byte_persec_avg, rec_persec_avg; 	/* byte per sec, rec per sec, total avg */
	double total_bytes;
	double total_rec;
};

struct StatCollector {

private:
	atomic<unsigned long> byte_counter_, record_counter_;

	/* last time we report */
	unsigned long last_bytes = 0, last_records = 0;
	boost::posix_time::ptime last_check, start_time;
	int once = 1;

public:
	/* @return: is stat filled */
	bool ReportStatistics(Statstics* stat) {
		/* internal accounting */
		unsigned long total_records =
				record_counter_.load(std::memory_order_relaxed);
		unsigned long total_bytes =
				byte_counter_.load(std::memory_order_relaxed);

		auto now = boost::posix_time::microsec_clock::local_time();

		if (once) {
			once = 0;
			last_check = now;
			start_time = now;
			last_records = total_records;
			return false;
		}

		boost::posix_time::time_duration diff = now - last_check;

		{
			double interval_sec = (double) diff.total_milliseconds() / 1000;
			double total_sec = (double) (now - start_time).total_milliseconds() / 1000;

//			stat->name = this->name.c_str();
			stat->byte_persec_avg = (double) total_bytes / total_sec;
			stat->rec_persec_avg = (double) total_records / total_sec;

			stat->byte_persec = (double) (total_bytes - last_bytes) / interval_sec;
			stat->rec_persec = (double) (total_records - last_records) / interval_sec;

			stat->total_rec = (double) total_records;
			stat->total_bytes = (double) total_bytes;

			last_check = now;
			last_bytes = total_bytes;
			last_records = total_records;
		}

		return true;
	}

public:

	void inc_rec_counter(int delta) {
		record_counter_.fetch_add(delta, std::memory_order_relaxed);
	}

	void inc_byte_counter(int delta) {
		byte_counter_.fetch_add(delta, std::memory_order_relaxed);
	}

};
#endif //VIDEO_STREAMER_STATCOLLECTOR_H
