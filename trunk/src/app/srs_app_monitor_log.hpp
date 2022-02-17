#ifndef SRS_APP_MONITOR_LOG_HPP
#define SRS_APP_MONITOR_LOG_HPP

/*
#include <srs_app_monitor_log.hpp>
*/

#include <srs_core.hpp>
#include <srs_kernel_utility.hpp>
#include <srs_app_reload.hpp>

#include <string.h>

#include <string>
#include <map>

//log parameter macro define.
//protocol macros define
#define SRS_MONITOR_PROTOCOL_RTMP		"rtmp"
#define SRS_MONITOR_PROTOCOL_HTTP		"http"
#define	SRS_MONITOR_PROTOCOL_HLS		"hls"
#define	SRS_MONITOR_PROTOCOL_RTSP		"rtsp"
//client type define
#define SRS_MONITOR_CLIENT_PULL			"Pull"
#define SRS_MONITOR_CLIENT_PUSH			"Push"
#define	SRS_MONITOR_CLIENT_SOURCE_PULL	"SourcePull"
#define SRS_MONITOR_CLIENT_SOURCE_PUSH	"SourcePush"
#define SRS_MONITOR_CLIENT_FORWARD		"Forward"
//log type define
#define SRS_MONITOR_LOG_TYPE_EVENT		"event"
#define SRS_MONITOR_LOG_TYPE_STAT		"stat"
#define	SRS_MONITOR_LOG_TYPE_FLOW		"flow"
//location status define
#define SRS_MONITOR_STATUS_CLIENT_EDGE	"CE"
#define SRS_MONITOR_STATUS_EDGE_EDGE	"EE"
#define	SRS_MONITOR_STATUS_EDGE_UPPER	"EU"
#define	SRS_MONITOR_STATUS_SUPER_ORIGIN "SO"
#define SRS_MONITOR_STATUS_SUPER_SUPER	"SS"
#define SRS_MONITOR_STATUS_UPPER_ORIGIN	"UO"
#define SRS_MONITOR_STATUS_UPPER_SUPER  "US"
#define	SRS_MONITOR_STATUS_UPPER_UPPER	"UU"
//default define.
#define	SRS_MONITOR_LOG_SPLIT_FIELD		"|"
#define	SRS_MONITOR_LOG_DEFAULT_VALUE	"-"

enum SrsMonitorActionCode{
	ActionUnknown = 0x0,
	ActionStartConnection,
	ActionStartRtmpHandShake,
	ActionStartPush,
	ActionStartPull,
	ActionStartSourcePush,
	ActionStartSourcePull,
	ActionStartForward,
	ActionSourcePushRetry,
	ActionSourcePushAddrChange,
	ActionSourcePullRetry,
	ActionSourcePullAddrChange,
	ActionSourceForwardAddrChange,
	ActionPublishClientDisconnect,
	ActionPlayClientDisconnect,
	ActionOriginDisconnect,
	ActionForwardDisconnect,
	ActionOverMaxConnections,
	ActionSecurityCheckFail,
	ActionLogTimeCost,         //for log time cost.
};

typedef struct SrsMonitorCommonInfoTag {
	SrsMonitorCommonInfoTag()
	{
		session_id = SRS_MONITOR_LOG_DEFAULT_VALUE;
		device_id = SRS_MONITOR_LOG_DEFAULT_VALUE;
		client_type = SRS_MONITOR_LOG_DEFAULT_VALUE;
		client_ip = SRS_MONITOR_LOG_DEFAULT_VALUE;
		client_port = 0;
		server_ip = SRS_MONITOR_LOG_DEFAULT_VALUE;
		server_port = 0;
		protocol = SRS_MONITOR_LOG_DEFAULT_VALUE;
		tc_url = SRS_MONITOR_LOG_DEFAULT_VALUE;
		origin_vhost = SRS_MONITOR_LOG_DEFAULT_VALUE;
		vhost = SRS_MONITOR_LOG_DEFAULT_VALUE;
		app = SRS_MONITOR_LOG_DEFAULT_VALUE;
		stream = SRS_MONITOR_LOG_DEFAULT_VALUE;
		source_session_id = SRS_MONITOR_LOG_DEFAULT_VALUE;
		location_status = SRS_MONITOR_LOG_DEFAULT_VALUE;
		source_name = SRS_MONITOR_LOG_DEFAULT_VALUE;
		record_time_str = SRS_MONITOR_LOG_DEFAULT_VALUE;
		log_type = SRS_MONITOR_LOG_DEFAULT_VALUE;
		record_time = last_time = srs_get_system_time_ms()/1000;
		event_time = 0;
	}
	std::string		session_id;
	std::string 	device_id;
	std::string 	client_type;
	std::string 	client_ip;
	int				client_port;
	std::string		server_ip;
	int				server_port;
	std::string		protocol;
	std::string		tc_url;
	std::string		origin_vhost;
	std::string		vhost;
	std::string		app;
	std::string		stream;
	std::string		source_session_id;
	std::string		location_status;
	std::string		source_name;
	std::string		record_time_str;  //record time created when write log.
	std::string		log_type;
	int 			last_time;
	int				record_time;
	int				event_time;
}SrsMonitorCommonInfo;

typedef struct SrsMonitorEventInfoTag {
	SrsMonitorEventInfoTag()
	{
		is_error = 0;
		error_code = 0;
		error_desc = SRS_MONITOR_LOG_DEFAULT_VALUE;
	}
	int is_error;
	int error_code;
	std::string error_desc;
}SrsMonitorEventInfo;

typedef struct SrsMonitorStatInfoTag {
	SrsMonitorStatInfoTag()
	{
		total_user = 0;
		online_in_stat = 0;
		offline_in_stat = 0;
	}
	int total_user;
	int online_in_stat;
	int offline_in_stat;
}SrsMonitorStatInfo;

typedef struct SrsMonitorFlowCommonInfoTag {
	SrsMonitorFlowCommonInfoTag()
	{
		duration = 0;
		send_bytes = 0;
		recv_bytes = 0;
		audio_bytes = 0;
		video_bytes = 0;
		last_audio_ts = -1;
		last_video_ts = -1;
		first_audio_ts = -1;
		first_video_ts = -1;
		audio_ts_min = -1;
		audio_ts_max = -1;
		video_ts_min = -1;
		video_ts_max = -1;
		nodata_duration = 0;
		video_msgs = 0;
		current_fps = 0;
		fps = 0;
		last_play_video_timestamp = -1;
		total_video_ts_interval = 0;
        video_interval_count = 0;

        total_recv_pkts = 0;
        total_send_pkts = 0;
        recv_pkts = 0;
        send_pkts = 0;
        max_send_pkts = 0;
        max_send_duration = 0;
		max_recv_duration = 0;
        cur_pkts = 0;
        cur_bytes = 0;
        first_audio_ori_timestamp = -1;
        end_audio_ori_timestamp = -1;
        first_video_ori_timestamp = -1;
        end_video_ori_timestamp = -1;
	}
	int duration;
	int64_t send_bytes;
	int64_t recv_bytes;
    int64_t audio_bytes;
    int64_t video_bytes;
    int64_t last_audio_ts;
    int64_t last_video_ts;
    int64_t first_audio_ts;
    int64_t first_video_ts;
    int64_t audio_ts_min;
    int64_t audio_ts_max;
    int64_t video_ts_min;
    int64_t video_ts_max;
	int nodata_duration;
	int video_msgs; //for calc current fps. record video msgs count.
	int current_fps;
	int fps;
	//add for calc
    int64_t last_play_video_timestamp;
	int total_video_ts_interval;
    int video_interval_count;

    int64_t total_recv_pkts;
    int64_t total_send_pkts;
    // recv pkts in duration
    int recv_pkts;
    // send pkts in duration
    int send_pkts;
    // max send pkts in duration
    int max_send_pkts;
    int max_recv_duration;
    int max_send_duration;
    // send pkts while max send duation
    int cur_pkts;
    // send bytes while max send duation
    int cur_bytes;
    int64_t first_audio_ori_timestamp;
    int64_t end_audio_ori_timestamp;
    int64_t first_video_ori_timestamp;
    int64_t end_video_ori_timestamp;
}SrsMonitorFlowCommonInfo;

typedef struct SrsMonitorPullInfoTag {
	SrsMonitorPullInfoTag()
	{
		max_msg_list = 0;
		shrink_count = 0;
		loss_frame_count = 0;
	}
	SrsMonitorFlowCommonInfo flow_info;
	int max_msg_list;
	int shrink_count;
	int loss_frame_count;
}SrsMonitorPullInfo;

typedef struct SrsMonitorPushInfoTag {
	SrsMonitorPushInfoTag()
	{
		kframe_gop_count = 0;
		gop_duration = 0;
		kframe_interval_min = 0;
		kframe_interval_max = 0;
	}
	SrsMonitorFlowCommonInfo flow_info;
	int kframe_gop_count;  //get current kframe gop count
	int gop_duration; //get current gop cache duration
	int kframe_interval_min; //in calc duration, kframe timestamp interval min
	int kframe_interval_max; //in calc duration, kframe timestamp interval max
}SrsMonitorPushInfo;

typedef struct SrsMonitorSourcePushInfoTag {
	SrsMonitorSourcePushInfoTag()
	{
		max_msg_list = 0;
		shrink_count = 0;
		loss_frame_count = 0;
	}
	SrsMonitorFlowCommonInfo flow_info;
	int max_msg_list;
	int shrink_count;
	int loss_frame_count;
}SrsMonitorSourcePushInfo;

typedef struct SrsMonitorSourcePullInfoTag {
	SrsMonitorSourcePullInfoTag()
	{
		kframe_gop_count = 0;
		gop_duration = 0;
		kframe_interval_min = 0;
		kframe_interval_max = 0;
	}
	SrsMonitorFlowCommonInfo flow_info;
	int kframe_gop_count;  //get current kframe gop count
	int gop_duration; //get current gop cache duration
	int kframe_interval_min; //in calc duration, kframe timestamp interval min
	int kframe_interval_max; //in calc duration, kframe timestamp interval max
}SrsMonitorSourcePullInfo;

typedef struct SrsMonitorForwardInfoTag {
	SrsMonitorForwardInfoTag()
	{
		max_msg_list = 0;
		shrink_count = 0;
		loss_frame_count = 0;
	}
	SrsMonitorFlowCommonInfo flow_info;
	int max_msg_list;
	int shrink_count;
	int loss_frame_count;
}SrsMonitorForwardInfo;

//define for monitor log info write.
typedef struct SrsMonitorLogInfoTag {
	SrsMonitorCommonInfo common_info;
	SrsMonitorEventInfo event_info;
	SrsMonitorStatInfo stat_info;
	SrsMonitorPullInfo pull_info;
	SrsMonitorPushInfo push_info;
	SrsMonitorSourcePushInfo source_push_info;
	SrsMonitorSourcePullInfo source_pull_info;
	SrsMonitorForwardInfo forward_info;
}SrsMonitorLogInfo;

class SrsMonitorLog : public ISrsReloadHandler
{
public:
	SrsMonitorLog();
    virtual ~SrsMonitorLog();

public:
    int initialize();
    void reopen();
    void write_monitor_event_log(SrsMonitorLogInfo *info, bool c_t);
    void write_monitor_stat_log(SrsMonitorLogInfo *info, bool need_time);
    //check if time to write log.
    bool check_monitor_time(SrsMonitorLogInfo *info);
    void write_monitor_pull_log(SrsMonitorLogInfo *info, bool need_time);
    void write_monitor_push_log(SrsMonitorLogInfo *info, bool need_time);
    void write_monitor_source_pull_log(SrsMonitorLogInfo *info, bool need_time);
    void write_monitor_source_push_log(SrsMonitorLogInfo *info, bool need_time);
    void write_monitor_forward_log(SrsMonitorLogInfo *info, bool need_time);
    //
    void record_monitor_time(SrsMonitorLogInfo *info);

    bool create_time(SrsMonitorLogInfo *info, bool is_event);

public:
    //need check local is client or server
    void set_address_info(SrsMonitorLogInfo *info, int fd, bool is_client);
public:
    virtual int on_reload_monitor_log_file();
    virtual int on_reload_monitor_log_interval();
private:
    void open_monitor_log_file();
    //std::string transfer_action_code(int action_code);
    //interval use monitor_interval;
    bool check_if_can_write(SrsMonitorLogInfo *info);
    void reset_flow_info(SrsMonitorFlowCommonInfo *flow_info);
private:
    int fd;
    int monitor_interval;
public:
    int64_t interval;
};

extern SrsMonitorLog* _monitor_log;
#endif
