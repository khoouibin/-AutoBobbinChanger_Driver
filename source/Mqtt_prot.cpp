/*#include <iostream>
#include <chrono>
#include "notification_pkg.h"
#include "Mqtt_prot.h"
#include "logger_wrapper.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

void DrvDownlinkService::runTask()
{
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    mqtt::async_client cli(SERVER_ADDRESS, CLIENT_ID);

    const auto TIMEOUT = std::chrono::seconds(10);
    try
    {
        cout << "Connecting to the MQTT server..." << flush;
        cli.connect(connOpts)->wait();
        cli.start_consuming();
        cli.subscribe(TOPIC, QOS)->wait();

        // Consume messages
        while (m_b_running)
        {
            auto msg = cli.consume_message();
            // if (!msg)
            //     break;
            cout << msg->get_topic() << ": " << msg->to_string() << endl;
            _queue.enqueueNotification(new NotificationFromHost(msg->to_string()));
        }

        // Disconnect
        cout << "\nShutting down and disconnecting from the MQTT server..." << flush;
        cli.unsubscribe(TOPIC)->wait();
        cli.stop_consuming();
        cli.disconnect()->wait();
    }
    catch (const mqtt::exception &exc)
    {
        cerr << exc.what() << endl;
    }
}

/////////////////////////////////////////////////////////////////////////////

//
// A callback class for use with the main MQTT client.
//
class callback : public virtual mqtt::callback
{
public:
    void connection_lost(const string &cause) override
    {
        cout << "\nConnection lost" << endl;
        if (!cause.empty())
            cout << "\tcause: " << cause << endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override
    {
        // cout << "\tDelivery complete for token: "
        //      << (tok ? tok->get_message_id() : -1) << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

//
// A base action listener.
//
class action_listener : public virtual mqtt::iaction_listener
{
protected:
    void on_failure(const mqtt::token &tok) override
    {
        cout << "\tListener failure for token: "
             << tok.get_message_id() << endl;
    }

    void on_success(const mqtt::token &tok) override
    {
        cout << "\tListener success for token: "
             << tok.get_message_id() << endl;
    }
};

/////////////////////////////////////////////////////////////////////////////

//
// A derived action listener for publish events.
//
class delivery_action_listener : public action_listener
{
    atomic<bool> done_;

    void on_failure(const mqtt::token &tok) override
    {
        action_listener::on_failure(tok);
        done_ = true;
    }

    void on_success(const mqtt::token &tok) override
    {
        action_listener::on_success(tok);
        done_ = true;
    }

public:
    delivery_action_listener() : done_(false) {}
    bool is_done() const { return done_; }
};

void DrvUplinkService::runTask()
{
    const char *LWT_PAYLOAD = "Last will and testament.";
    const auto TIMEOUT = std::chrono::seconds(2);

    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

    callback cb;
    client.set_callback(cb);

    mqtt::connect_options conopts;
    mqtt::message willmsg(TOPIC, LWT_PAYLOAD, 1, true);
    mqtt::will_options will(willmsg);
    conopts.set_will(will);

    try
    {
        std::string log;
        mqtt::token_ptr conntok = client.connect(conopts);
        conntok->wait();
        log = "connect mqtt success";
        goDriverLogger.Log("info", log);

        while (m_b_running)
        {
            Notification::Ptr pNf(_queue.waitDequeueNotification(1000));
            if (pNf)
            {
                NotificationToHost::Ptr pWorkNf = pNf.cast<NotificationToHost>();
                if (pWorkNf)
                {
                    try
                    {
                        std::string str_msg = pWorkNf->data();
                        const char *cstr = str_msg.c_str();
                        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, cstr);
                        pubmsg->set_qos(QOS);
                        client.publish(pubmsg)->wait_for(TIMEOUT);
                        log = "publish " + str_msg + " on " + TOPIC;
                        goDriverLogger.Log("debug", log);
                    }
                    catch (std::exception &e)
                    {
                        log = "Exception " + std::string(e.what());
                        goDriverLogger.Log("error", log);
                    }
                }
            }
        }

        auto toks = client.get_pending_delivery_tokens();
        if (!toks.empty())
            goDriverLogger.Log("error", "Error: There are pending delivery tokens!");

        conntok = client.disconnect();
        conntok->wait();
        goDriverLogger.Log("error", "disconnect mqtt success");
    }

    catch (const mqtt::exception &exc)
    {
        cerr << exc.what() << endl;
    }
}
#include <global.h>
void publish_uplink_message2(std::string status, int error_code, std::string message)
{
    json notification;
	notification["cmd"] = "notification";
    notification["status"] = status;
    notification["error_code"] = error_code;
    notification["message"] = message;

    const std::string SERVER_ADDRESS{"tcp://localhost:1883"};
    const std::string CLIENT_ID{"rtc_status"};
    const std::string TOPIC{"/uhf/drv/"};
    const int QOS = 1;

    const auto TIMEOUT = std::chrono::seconds(2);
    const char *LWT_PAYLOAD = "Last will and testament.";

    mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

    callback cb;
    client.set_callback(cb);

    mqtt::connect_options conopts;
    mqtt::message willmsg(TOPIC, LWT_PAYLOAD, 1, true);
    mqtt::will_options will(willmsg);
    conopts.set_will(will);

    mqtt::token_ptr conntok = client.connect(conopts);
// 若Host未啟動, 使用wait()會等待1秒以上才會跳脫, 造成程序卡在這裡, 和USB封包爆倉. Jack
timestamp_t time_delay;
time_delay.time_pre = time_stamp();
    if (conntok->wait_for(2000)==false){
        
	time_delay.time_now = time_stamp();
	time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
    printf("Fail to wait mqtt, time=%d\n", time_delay.time_diff);
        return;
    }
	time_delay.time_now = time_stamp();
	time_delay.time_diff= time_delay.time_now - time_delay.time_pre;
    printf("success to wait mqtt, time=%d\n", time_delay.time_diff);
 //   conntok->wait();
    std::string log;
    try
    {
        std::string str_msg = notification.dump();
        const char *cstr = str_msg.c_str();
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, cstr);
        pubmsg->set_qos(QOS);
        client.publish(pubmsg)->wait_for(TIMEOUT);
        log = "publish " + str_msg + " on " + TOPIC;
        goDriverLogger.Log("debug", log);
    }
    catch (std::exception &e)
    {
        log = "Exception " + std::string(e.what());
        goDriverLogger.Log("error", log);
    }

    auto toks = client.get_pending_delivery_tokens();
    if (!toks.empty())
        goDriverLogger.Log("error", "Error: There are pending delivery tokens!");

    conntok = client.disconnect();
    conntok->wait();
    goDriverLogger.Log("debug", "disconnect mqtt success");
}
*/
