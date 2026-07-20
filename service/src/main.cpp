#include "httplib.h"
#include "markdown.h"
#include "message_handler.h"

#include <librdkafka/rdkafkacpp.h>

#include <sstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <memory>

std::atomic<bool> running{true};

std::string convertMarkdown(const std::string &input) {
    markdown::Document doc;
    doc.read(input);
    std::ostringstream out;
    doc.write(out);
    return out.str();
}

void kafkaConsumerLoop(const std::string &brokers) {
    std::string errstr;

    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    conf->set("bootstrap.servers", brokers, errstr);
    conf->set("group.id", "conversion-service-group", errstr);

    std::unique_ptr<RdKafka::KafkaConsumer> consumer(RdKafka::KafkaConsumer::create(conf.get(), errstr));
    if (!consumer) {
        std::cerr << "Failed to create Kafka consumer: " << errstr << std::endl;
        return;
    }

    std::vector<std::string> topics = {"documents-to-convert"};
    RdKafka::ErrorCode err = consumer->subscribe(topics);
    if (err) {
        std::cerr << "Failed to subscribe: " << RdKafka::err2str(err) << std::endl;
        return;
    }

    std::unique_ptr<RdKafka::Conf> pconf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    pconf->set("bootstrap.servers", brokers, errstr);
    std::unique_ptr<RdKafka::Producer> producer(RdKafka::Producer::create(pconf.get(), errstr));
    if (!producer) {
        std::cerr << "Failed to create Kafka producer: " << errstr << std::endl;
        return;
    }

    std::cout << "Kafka consumer/producer started, listening on documents-to-convert" << std::endl;

    while (running) {
        std::unique_ptr<RdKafka::Message> msg(consumer->consume(1000));

        if (msg->err() == RdKafka::ERR_NO_ERROR) {
            std::string raw(static_cast<const char *>(msg->payload()), msg->len());

            auto parsed = parseKafkaMessage(raw);
            if (!parsed) {
                std::cerr << "Skipping malformed message: " << raw << std::endl;
                continue;
            }

            std::string html = convertMarkdown(parsed->markdown);
            std::string result = buildResultMessage(parsed->tenantId, parsed->documentId, html);

            producer->produce(
                "converted-documents",
                RdKafka::Topic::PARTITION_UA,
                RdKafka::Producer::RK_MSG_COPY,
                const_cast<char *>(result.data()), result.size(),
                nullptr, 0,
                0, nullptr, nullptr
            );
            producer->poll(0);

            std::cout << "Processed document " << parsed->documentId
                       << " for tenant " << parsed->tenantId << std::endl;
        } else if (msg->err() != RdKafka::ERR__TIMED_OUT) {
            std::cerr << "Consume error: " << msg->errstr() << std::endl;
        }
    }

    producer->flush(5000);
    consumer->close();
}

int main() {
    const char *brokersEnv = std::getenv("KAFKA_BROKERS");
    std::string brokers = brokersEnv ? brokersEnv : "my-kafka-cluster-kafka-bootstrap.kafka.svc.cluster.local:9092";

    std::thread kafkaThread(kafkaConsumerLoop, brokers);

    httplib::Server svr;

    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("{\"status\":\"ok\",\"service\":\"cpp-markdown-conversion\",\"version\":\"1.0.0\"}", "application/json");
    });

    svr.Post("/convert", [](const httplib::Request &req, httplib::Response &res) {
        if (req.body.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"empty request body\"}", "application/json");
            return;
        }

        try {
            std::string html = convertMarkdown(req.body);
            res.set_content(html, "text/html");
        } catch (const std::exception &e) {
            res.status = 500;
            res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
        }
    });

    int port = 8080;
    std::cout << "Conversion service listening on port " << port << std::endl;
    svr.listen("0.0.0.0", port);

    running = false;
    kafkaThread.join();

    return 0;
}