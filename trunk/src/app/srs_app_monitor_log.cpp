#include <srs_app_monitor_log.hpp>

#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sstream>
#include <stdio.h>

#include <srs_app_config.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_app_utility.hpp>

using namespace std;


SrsMonitorLog::SrsMonitorLog()
{
	fd = -1;
}

SrsMonitorLog::~SrsMonitorLog()
{
	if (fd > 0) {
		::close(fd);
		fd = -1;
	}

	if (_srs_config) {
		_srs_config->unsubscribe(this);
	}
}

int SrsMonitorLog::initialize()
{
	int ret = ERROR_SUCCESS;

	if (_srs_config) {
		_srs_config->subscribe(this);

        monitor_interval = _srs_config->get_monitor_log_interval();
        interval = monitor_interval * 1000;
	}

	return ret;
}

void SrsMonitorLog::reopen()
{
	if (fd > 0) {
		::close(fd);
	}

	open_monitor_log_file();
}

int SrsMonitorLog::on_reload_monitor_log_file()
{
	int ret = ERROR_SUCCESS;

	if (!_srs_config) {
		return ret;
	}

	if (fd > 0) {
		::close(fd);
	}

	open_monitor_log_file();

	return ret;
}

int SrsMonitorLog::on_reload_monitor_log_interval()
{
	int ret = ERROR_SUCCESS;

	if (!_srs_config) {
		return ret;
	}

    monitor_interval = _srs_config->get_monitor_log_interval();
    interval = monitor_interval * 1000;

	return ret;
}



void SrsMonitorLog::open_monitor_log_file()
{
	if (!_srs_config) {
		return;
	}

	std::string filename = _srs_config->get_moinitor_log_file();

	if (filename.empty()) {
		return;
	}

	fd = ::open(filename.c_str(), O_RDWR | O_APPEND);

	if(fd == -1 && errno == ENOENT) {
		fd = open(filename.c_str(),
				O_RDWR | O_CREAT | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
		);
	}
}

void SrsMonitorLog::write_monitor_event_log(SrsMonitorLogInfo *info, bool c_t)
{
	//first check fd
	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (!info) {
		//info not exist.
		return;
	}

    if (c_t && !create_time(info, true)) {
		return;
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_EVENT;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
    // write srs log id
    ss << _srs_context->get_id() << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write event log
	ss << info->event_info.is_error << SRS_MONITOR_LOG_SPLIT_FIELD << info->event_info.error_code << SRS_MONITOR_LOG_SPLIT_FIELD
       << info->event_info.error_desc;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
}

void SrsMonitorLog::write_monitor_stat_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}
	if(info->common_info.record_time - info->common_info.last_time <= 1) {
		//interval is too short, no need record stat info.
		info->common_info.record_time = info->common_info.last_time;
		return;
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_STAT;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write stat log
	ss << info->stat_info.total_user << SRS_MONITOR_LOG_SPLIT_FIELD << info->stat_info.online_in_stat << SRS_MONITOR_LOG_SPLIT_FIELD
	<< info->stat_info.offline_in_stat;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}

	//set stat info to zero,total no need change.
	info->stat_info.online_in_stat = info->stat_info.offline_in_stat = 0;
}

bool SrsMonitorLog::check_monitor_time(SrsMonitorLogInfo *info)
{
	if (!info) {
		//info not exist.
		return false;
	}
	return check_if_can_write(info);
}

void SrsMonitorLog::write_monitor_pull_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_FLOW;
	info->pull_info.flow_info.duration = info->common_info.record_time - info->common_info.last_time;
	//get flow common info.
	if (info->pull_info.flow_info.duration > 0)
		info->pull_info.flow_info.current_fps = info->pull_info.flow_info.video_msgs/info->pull_info.flow_info.duration;
	int average_interval = 0;
	if (info->pull_info.flow_info.video_interval_count > 0)
		average_interval = info->pull_info.flow_info.total_video_ts_interval/info->pull_info.flow_info.video_interval_count;
	 if (average_interval > 0)
	   info->pull_info.flow_info.fps = 1000/average_interval;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write flow common data;
	ss << info->pull_info.flow_info.duration << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.send_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.recv_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.audio_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.video_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.first_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.last_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.first_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.last_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.audio_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.audio_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.video_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.video_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.nodata_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.flow_info.current_fps << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.fps << SRS_MONITOR_LOG_SPLIT_FIELD;
	//
	ss << info->pull_info.max_msg_list << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.shrink_count << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->pull_info.loss_frame_count;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
    //
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.total_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.total_recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.first_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.end_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.first_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.end_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.max_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.max_recv_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.max_send_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.cur_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->pull_info.flow_info.cur_bytes;
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
	reset_flow_info(&info->pull_info.flow_info);
}

void SrsMonitorLog::write_monitor_push_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_FLOW;
	info->push_info.flow_info.duration = info->common_info.record_time - info->common_info.last_time;
	//get flow common info.
	if (info->push_info.flow_info.duration > 0)
		info->push_info.flow_info.current_fps = info->push_info.flow_info.video_msgs/info->push_info.flow_info.duration;
	int average_interval = 0;
	if (info->push_info.flow_info.video_interval_count > 0)
		average_interval = info->push_info.flow_info.total_video_ts_interval/info->push_info.flow_info.video_interval_count;
	if (average_interval > 0)
		info->push_info.flow_info.fps = 1000/average_interval;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write flow common data;
	ss << info->push_info.flow_info.duration << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.send_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.recv_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.audio_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.video_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.first_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.last_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.first_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.last_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.audio_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.audio_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.video_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.video_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.nodata_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.flow_info.current_fps << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.fps << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write push special data
	ss << info->push_info.kframe_gop_count << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.gop_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->push_info.kframe_interval_min << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.kframe_interval_max;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
    //
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.total_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.total_recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.first_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.end_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.first_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.end_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.max_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.max_recv_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.max_send_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.cur_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->push_info.flow_info.cur_bytes;
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
	reset_flow_info(&info->push_info.flow_info);
}

void SrsMonitorLog::write_monitor_source_pull_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_FLOW;
	info->source_pull_info.flow_info.duration = info->common_info.record_time - info->common_info.last_time;
	//get flow common info.
	if (info->source_pull_info.flow_info.duration > 0)
		info->source_pull_info.flow_info.current_fps = info->source_pull_info.flow_info.video_msgs/info->source_pull_info.flow_info.duration;
	int average_interval = 0;
	if (info->source_pull_info.flow_info.video_interval_count > 0)
		average_interval = info->source_pull_info.flow_info.total_video_ts_interval/info->source_pull_info.flow_info.video_interval_count;
	if (average_interval > 0)
		info->source_pull_info.flow_info.fps = 1000/average_interval;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write flow common data;
	ss << info->source_pull_info.flow_info.duration << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.send_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.recv_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.audio_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.video_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.first_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.last_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.first_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.last_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.audio_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.audio_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.video_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.video_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.nodata_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.flow_info.current_fps << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.fps << SRS_MONITOR_LOG_SPLIT_FIELD;
	//
	ss << info->source_pull_info.kframe_gop_count << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.gop_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_pull_info.kframe_interval_min << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.kframe_interval_max;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
    //
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.total_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.total_recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.first_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.end_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.first_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.end_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.max_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.max_recv_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.max_send_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.cur_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_pull_info.flow_info.cur_bytes;
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
	reset_flow_info(&info->source_pull_info.flow_info);
}

void SrsMonitorLog::write_monitor_source_push_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_FLOW;
	info->source_push_info.flow_info.duration = info->common_info.record_time - info->common_info.last_time;
	//get flow common info.
	if (info->source_push_info.flow_info.duration > 0)
		info->source_push_info.flow_info.current_fps = info->source_push_info.flow_info.video_msgs/info->source_push_info.flow_info.duration;
	int average_interval = 0;
	if (info->source_push_info.flow_info.video_interval_count > 0)
		average_interval = info->source_push_info.flow_info.total_video_ts_interval/info->source_push_info.flow_info.video_interval_count;
	if (average_interval > 0)
		info->source_push_info.flow_info.fps = 1000/average_interval;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write flow common data;
	ss << info->source_push_info.flow_info.duration << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.send_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.recv_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.audio_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.video_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.first_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.last_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.first_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.last_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.audio_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.audio_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.video_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.video_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.nodata_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.flow_info.current_fps << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.fps << SRS_MONITOR_LOG_SPLIT_FIELD;
	//
	ss << info->source_push_info.max_msg_list << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.shrink_count << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->source_push_info.loss_frame_count;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
    //
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.total_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.total_recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.first_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.end_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.first_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.end_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.max_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.max_recv_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.max_send_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.cur_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->source_push_info.flow_info.cur_bytes;
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
	reset_flow_info(&info->source_push_info.flow_info);
}

void SrsMonitorLog::write_monitor_forward_log(SrsMonitorLogInfo *info, bool need_time)
{
	if (!info) {
		//info not exist.
		return;
	}

	if (fd < 0 ) {
		open_monitor_log_file();
	}

	if (need_time) {
		if (!create_time(info, false)) {
			return;
		}
	}

	info->common_info.log_type = SRS_MONITOR_LOG_TYPE_FLOW;
	info->forward_info.flow_info.duration = info->common_info.record_time - info->common_info.last_time;
	//get flow common info.
	if (info->forward_info.flow_info.duration > 0)
		info->forward_info.flow_info.current_fps = info->forward_info.flow_info.video_msgs/info->forward_info.flow_info.duration;
	int average_interval = 0;
	if (info->forward_info.flow_info.video_interval_count > 0)
		average_interval = info->forward_info.flow_info.total_video_ts_interval/info->forward_info.flow_info.video_interval_count;
	if (average_interval > 0)
		info->forward_info.flow_info.fps = 1000/average_interval;
	stringstream ss;
	//write common log.
	ss << info->common_info.session_id << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.device_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_type << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.client_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.client_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.server_ip << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.server_port << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.protocol << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.tc_url << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.origin_vhost << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.vhost << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.app << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.stream << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_session_id << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.location_status << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.source_name << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->common_info.record_time_str << SRS_MONITOR_LOG_SPLIT_FIELD << info->common_info.log_type << SRS_MONITOR_LOG_SPLIT_FIELD;
	//write flow common log
	ss << info->forward_info.flow_info.duration << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.send_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.recv_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.audio_bytes << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.video_bytes << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.first_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.last_audio_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.first_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.last_video_ts << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.audio_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.audio_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.video_ts_min << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.video_ts_max << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.nodata_duration << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.flow_info.current_fps << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.fps << SRS_MONITOR_LOG_SPLIT_FIELD;
	//
	ss << info->forward_info.max_msg_list << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.shrink_count << SRS_MONITOR_LOG_SPLIT_FIELD
	   << info->forward_info.loss_frame_count;
	//
	ss << SRS_MONITOR_LOG_SPLIT_FIELD << getpid();
    //
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.total_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.total_recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.first_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.end_audio_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.first_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.end_video_ori_timestamp;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.recv_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.max_send_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.max_recv_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.max_send_duration;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.cur_pkts;
    ss << SRS_MONITOR_LOG_SPLIT_FIELD << info->forward_info.flow_info.cur_bytes;
	//
	ss << "\r\n";

    string data = ss.str();

    _srs_syslog->write(string(data.c_str(), data.size() - 2), "monitor");

	// write log to file.
	if (fd > 0) {
		::write(fd, data.c_str(), data.size());
	}
	reset_flow_info(&info->forward_info.flow_info);
}

/*string SrsMonitorLog::transfer_action_code(int action_code)
{
	switch(action_code) {
		case ActionUnknown: return "Unknown Action";
		case ActionOverMaxConnections: return "Over Max Connections";
		case ActionSecurityCheckFail: return "Security Check Fail";
		case ActionPublishClientDisconnect: return "Publish Client Disconnect";
		case ActionPlayClientDisconnect: return "Play Client Disconnect";
		default: return "Error Code Define";
	}
}*/

//
void SrsMonitorLog::set_address_info(SrsMonitorLogInfo *info, int fd, bool is_client)
{
	if (is_client) {
		info->common_info.client_ip = srs_get_local_ip(fd);
		info->common_info.client_port = srs_get_local_port(fd);
		info->common_info.server_ip = srs_get_peer_ip(fd);
		info->common_info.server_port = srs_get_peer_port(fd);
	}
	else {
		info->common_info.client_ip = srs_get_peer_ip(fd);
		info->common_info.client_port = srs_get_peer_port(fd);
		info->common_info.server_ip = srs_get_local_ip(fd);
		info->common_info.server_port = srs_get_local_port(fd);
	}
}

//check if can write.
bool SrsMonitorLog::check_if_can_write(SrsMonitorLogInfo *info)
{
    int current_time = srs_get_system_time_ms()/1000;
    if (current_time - info->common_info.record_time < monitor_interval) {
		return false;
	}
	timeval tv;
	struct tm* tm;
	char buf[128] = "";
	if (!gettimeofday(&tv, NULL)) {
		// to calendar time
		if ((tm = localtime(&tv.tv_sec)) == NULL) {
			return false;
		}
	}

	info->common_info.last_time = info->common_info.record_time;

	info->common_info.record_time = tv.tv_sec;

	snprintf(buf, 128, "%d-%02d-%02d %02d:%02d:%02d.%03d",
			1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));

	info->common_info.record_time_str = buf;
	return true;
}

bool SrsMonitorLog::create_time(SrsMonitorLogInfo *info, bool is_event)
{
	//calc monitor time.
	char buf[128] = "";
	timeval tv;
	struct tm* tm;
	if (!gettimeofday(&tv, NULL)) {
		// to calendar time
		if ((tm = localtime(&tv.tv_sec)) == NULL) {
			return false;
		}
	}

	if (!is_event) {
		info->common_info.last_time = info->common_info.record_time;
		info->common_info.record_time = tv.tv_sec;
	}
	else {
		info->common_info.event_time = tv.tv_sec;
	}
	snprintf(buf, 128, "%d-%02d-%02d %02d:%02d:%02d.%03d",
			1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));

	info->common_info.record_time_str = buf;
	return true;
}

void SrsMonitorLog::reset_flow_info(SrsMonitorFlowCommonInfo *flow_info)
{
	flow_info->duration = 0;
	flow_info->send_bytes = 0;
	flow_info->recv_bytes = 0;
	flow_info->audio_bytes = 0;
	flow_info->video_bytes = 0;
	flow_info->last_audio_ts = -1;
	flow_info->last_video_ts = -1;
	flow_info->first_audio_ts = -1;
	flow_info->first_video_ts = -1;
	flow_info->audio_ts_min = -1;
	flow_info->audio_ts_max = -1;
	flow_info->video_ts_min = -1;
	flow_info->video_ts_max = -1;
	flow_info->nodata_duration = 0;
	flow_info->current_fps = 0;
	flow_info->fps = 0;
	flow_info->video_msgs = 0;
	flow_info->last_play_video_timestamp = -1;
	flow_info->total_video_ts_interval = 0;
    flow_info->video_interval_count = 0;

    flow_info->first_audio_ori_timestamp = -1;
    flow_info->end_audio_ori_timestamp = -1;
    flow_info->first_video_ori_timestamp = -1;
    flow_info->end_video_ori_timestamp = -1;
    flow_info->send_pkts = 0;
    flow_info->recv_pkts = 0;
    flow_info->max_send_pkts = 0;
    flow_info->max_recv_duration = 0;
    flow_info->max_send_duration = 0;
    flow_info->cur_pkts = 0;
    flow_info->cur_bytes = 0;
}

void SrsMonitorLog::record_monitor_time(SrsMonitorLogInfo *info)
{
	create_time(info, false);
}
