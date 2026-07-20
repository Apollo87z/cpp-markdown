#include "httplib.h"
#include "markdown.h"

#include <sstream>
#include <iostream>

std::string convertMarkdown(const std::string &input) {
    markdown::Document doc;
    doc.read(input);
    std::ostringstream out;
    doc.write(out);
    return out.str();
}

int main() {
    httplib::Server svr;

    svr.Get("/health", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
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

    return 0;
}