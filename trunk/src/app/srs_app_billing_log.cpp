#include "srs_app_billing_log.hpp"
#include "srs_app_config.hpp"
#include "srs_app_rtmp_conn.hpp"
#include "srs_kernel_error.hpp"
#include "srs_app_utility.hpp"
#include "srs_kernel_utility.hpp"
#include "srs_rtmp_amf0.hpp"
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

SrsBillingLogInfo::SrsBillingLogInfo(int fd, SrsRequest *_req, ISrsProtocolStatistic *_skt, const std::string &_protocol)
{
    local_ip = srs_get_local_ip(fd);
    local_port = srs_get_local_port(fd);
    peer_ip = srs_get_peer_ip(fd);
    peer_port = srs_get_peer_port(fd);

    req = _req;
    skt = _skt;
    protocol = _protocol;

    begin_time = previous_time = srs_get_system_time_ms();

    previous_recv = skt->get_recv_bytes();
    previous_send = skt->get_send_bytes();

    delta_recv_bytes = delta_send_bytes = 0;
    duration = current_duration = 0;

    error_no = 0;

    _srs_config->subscribe(this);
}

SrsBillingLogInfo::~SrsBillingLogInfo()
{
    _srs_config->unsubscribe(this);
}

bool SrsBillingLogInfo::can_print()
{
    int64_t cur_time = srs_get_system_time_ms();

    if (cur_time - previous_time >= _billing_log->interval) {
        return true;
    }

    return false;
}

void SrsBillingLogInfo::resample()
{
    int64_t cur_time = srs_get_system_time_ms();

    int64_t cur_recv = skt->get_recv_bytes();
    int64_t cur_send = skt->get_send_bytes();

    delta_recv_bytes = cur_recv - previous_recv;
    delta_send_bytes = cur_send - previous_send;

    duration = (cur_time - begin_time) / 1000;

    current_duration = (cur_time - previous_time) / 1000;

    previous_recv = cur_recv;
    previous_send = cur_send;
    previous_time = cur_time;
}

void SrsBillingLogInfo::update_service_type(std::string type, bool is_inner)
{
    if (type == SRS_BILLING_OPERATE_PUBLISH) {
        if (is_inner) {
            service_type = SRS_BILLING_RTMP_PUBLISH_INNER;
        } else {
            service_type = SRS_BILLING_RTMP_PUBLISH;
        }
    } else if (type == SRS_BILLING_OPERATE_PLAY) {
        if (is_inner) {
            service_type = SRS_BILLING_RTMP_PLAY_INNER;
        } else {
            service_type = SRS_BILLING_RTMP_PLAY;
        }
    }
}

SrsBillingLog::SrsBillingLog()
{
    fd = -1;
}

SrsBillingLog::~SrsBillingLog()
{
    if (fd > 0) {
        ::close(fd);
        fd = -1;
    }

    if (_srs_config) {
        _srs_config->unsubscribe(this);
    }
}

void SrsBillingLog::initialize()
{
    if (_srs_config) {
        _srs_config->subscribe(this);

        interval = _srs_config->get_billing_log_interval() * 1000;
    }
}

void SrsBillingLog::write_log(SrsBillingLogInfo *info)
{
    if (info->operate.empty()) {
        return;
    }

    // separator
    static char sp = '\001';

    info->resample();

    std::stringstream ss;

    char buf[128] = {0};
    struct timeval tv = {0};

    if (!gettimeofday(&tv, NULL)) {
        struct tm* tm = localtime(&tv.tv_sec);
        if (tm != NULL) {
            snprintf(buf, 128, "%d-%02d-%02d %02d:%02d:%02d.%03d",
                1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));
        }
    }

    int64_t now_us = ((int64_t)tv.tv_sec) * 1000 * 1000 + (int64_t)tv.tv_usec;

    ss << now_us / 1000;

    ss << sp << "[" << buf << "]";

    ss << sp << srs_get_host_name();
    ss << sp << _srs_config->get_srs_id();
    ss << sp << info->session_id;

    ss << sp << (info->service_type.size() ? info->service_type : "-");
    ss << sp << info->protocol;
    ss << sp << info->operate;
    ss << sp << (info->http_ext.size() ? info->http_ext : "-");

    ss << sp << (info->peer_ip.size() ? info->peer_ip : "-");

    if (info->peer_port > 0) {
        ss << sp << info->peer_port;
    } else {
        ss << sp << "-";
    }

    ss << sp << (info->local_ip.size() ? info->local_ip : "-");

    if (info->local_port > 0) {
        ss << sp << info->local_port;
    } else {
        ss << sp << "-";
    }

    ss << sp << info->begin_time;

    ss << sp << info->duration;

    ss << sp << (info->req->wildcard_vhost.size() ? info->req->wildcard_vhost : "-");
    ss << sp << (info->req->vhost.size() ? info->req->vhost : "-");
    ss << sp << (info->req->app.size() ? info->req->app : "-");
    ss << sp << (info->req->stream.size() ? info->req->stream : "-");
    ss << sp << (info->req->param.size() ? info->req->param : "-");
    ss << sp << (info->req->tcUrl.size() ? info->req->tcUrl : "-");
    ss << sp << (info->req->pageUrl.size() ? info->req->pageUrl : "-");

    ss << sp << info->current_duration;

    ss << sp << info->delta_recv_bytes;
    ss << sp << info->skt->get_recv_bytes();

    ss << sp << info->delta_send_bytes;
    ss << sp << info->skt->get_send_bytes();

    ss << sp << info->error_no;
    ss << sp << (info->sp_id.size() ? info->sp_id : "-");
    ss << sp << (info->status.size() ? info->status : "-");

    ss << "\r\n";

    if (fd < 0) {
        open_log_file();
    }

    std::string data = ss.str();

    _srs_syslog->write(std::string(data.c_str(), data.size() - 2), "billing");

    // write log to file.
    if (fd > 0) {
        ::write(fd, data.c_str(), data.size());
    }
}

void SrsBillingLog::write_log(SrsBillingLogInfo *info, int err)
{
    info->error_no = err;
    write_log(info);
    info->error_no = 0;
}

bool SrsBillingLog::write_interval(SrsBillingLogInfo *info)
{
    if (info->can_print()) {
        write_log(info);
        return true;
    }

    return false;
}

void SrsBillingLog::reopen()
{
    if (fd > 0) {
        ::close(fd);
    }

    open_log_file();
}

int SrsBillingLog::on_reload_billing_log_file()
{
    int ret = ERROR_SUCCESS;

    if (!_srs_config) {
        return ret;
    }

    if (fd > 0) {
        ::close(fd);
    }
    open_log_file();

    return ret;
}

int SrsBillingLog::on_reload_billing_log_interval()
{
    interval = _srs_config->get_billing_log_interval() * 1000;
    return ERROR_SUCCESS;
}

void SrsBillingLog::open_log_file()
{
    if (!_srs_config) {
        return;
    }

    std::string filename = _srs_config->get_billing_log_file();

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
