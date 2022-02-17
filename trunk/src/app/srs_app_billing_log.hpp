#ifndef SRS_APP_BILLING_LOG_HPP
#define SRS_APP_BILLING_LOG_HPP

#include <srs_core.hpp>
#include <srs_rtmp_stack.hpp>
#include <srs_app_reload.hpp>

class SrsRequest;
class ISrsProtocolStatistic;

#define SRS_BILLING_HTTP_LIVE           "HTS"
#define SRS_BILLING_EDGE_FORWARD        "EFW"
#define SRS_BILLING_EDGE_INGEST         "EIG"
#define SRS_BILLING_FORWARD             "FWR"
#define SRS_BILLING_RTMP_PUBLISH        "CPB"
#define SRS_BILLING_RTMP_PUBLISH_INNER  "IPB"
#define SRS_BILLING_RTMP_PLAY           "RPL"
#define SRS_BILLING_RTMP_PLAY_INNER     "IPL"
#define SRS_BILLING_EDGE_ORIGIN_INGEST  "EOIG"
#define SRS_BILLING_EDGE_ORIGIN_FORWARD "EOFW"

#define SRS_BILLING_OPERATE_PLAY        "play"
#define SRS_BILLING_OPERATE_PUBLISH     "publish"

class SrsBillingLogInfo : public ISrsReloadHandler
{
public:
    std::string local_ip;
    int local_port;
    std::string peer_ip;
    int peer_port;

    int64_t begin_time;

    ISrsProtocolStatistic *skt;
    SrsRequest *req;
    // rtmp | http
    std::string protocol;

    // total recv bytes in duration
    int64_t delta_recv_bytes;
    // total send bytes in duration
    int64_t delta_send_bytes;

    int64_t duration;
    // HTS | EFW | EIG | FWR | CPB | IPB | RPL | IPL
    std::string service_type;

    // handshake | connect | CreateStream | play | publish | get
    std::string operate;
    // .ts | .flv | .mp3 | .aac
    std::string http_ext;

    int64_t current_duration;

    int error_no;

    std::string session_id;
    std::string sp_id;

    std::string status;

public:
    int64_t previous_time;
    int64_t previous_recv;
    int64_t previous_send;

public:
    SrsBillingLogInfo(int fd, SrsRequest *_req, ISrsProtocolStatistic *_skt, const std::string &_protocol);
    virtual ~SrsBillingLogInfo();

    bool can_print();
    void resample();
    void update_service_type(std::string type, bool is_inner);

public:

};

class SrsBillingLog : public ISrsReloadHandler
{
public:
    SrsBillingLog();
    virtual ~SrsBillingLog();

public:
    void initialize();
    void write_log(SrsBillingLogInfo *info);
    void write_log(SrsBillingLogInfo *info, int err);
    bool write_interval(SrsBillingLogInfo *info);
    void reopen();

public:
    virtual int on_reload_billing_log_file();
    virtual int on_reload_billing_log_interval();

private:
    void open_log_file();

private:
    int fd;

public:
    int64_t interval;
};

extern SrsBillingLog* _billing_log;

#endif // SRS_APP_BILLING_LOG_HPP
