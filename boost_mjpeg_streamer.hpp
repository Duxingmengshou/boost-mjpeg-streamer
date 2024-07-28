#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <map>
#include <boost/beast/version.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/system.hpp>
#include <boost/bind.hpp>


class http_server {
public:
    typedef boost::asio::ip::tcp::acceptor acceptor;
    typedef std::shared_ptr<acceptor> acceptor_shared_ptr;

    typedef boost::asio::ip::tcp::socket socket;
    typedef std::shared_ptr<socket> socket_shared_ptr;

    typedef boost::asio::io_service io_service;
    typedef std::shared_ptr<io_service> io_service_shared_ptr;

    typedef boost::asio::ip::tcp::endpoint endpoint;
public:
    io_service_shared_ptr io_service_sp;
    acceptor_shared_ptr acceptor_sp;
    boost::asio::thread_pool thread_pool;
    std::map<std::string, std::shared_ptr<std::vector<char>>> frames;

public:
    http_server();

    void run();

    void run_service_io() const;

    void handle_client(socket_shared_ptr socket);

    void handle_device_request(socket_shared_ptr socket,
                               const boost::beast::http::request<boost::beast::http::string_body> &req);

    void handle_not_found(socket_shared_ptr socket,
                          const boost::beast::http::request<boost::beast::http::string_body> &req);

    void handle_sn_request(socket_shared_ptr socket,
                           const boost::beast::http::request<boost::beast::http::string_body> &req, std::string sn);

    void accept();

    void accept_handler(const boost::system::error_code &ec,
                        socket_shared_ptr sock);

    void publish(const std::string &sn, std::shared_ptr<std::vector<char>> frame_shared_ptr);

    void close(const std::string &sn);
};

#include <utility>

http_server::http_server() : thread_pool(7) {
    try {
        int port = 8080;
        io_service_sp = std::make_shared<io_service>();
        acceptor_sp = std::make_shared<acceptor>(
                *io_service_sp,
                endpoint(boost::asio::ip::tcp::v4(), port)
        );
    }
    catch (const boost::system::system_error &e) {
        if (e.code() == boost::asio::error::address_in_use) {
            std::cerr << "端口被占用" << std::endl;
            exit(EXIT_FAILURE);
        } else {
            std::cerr << "其他错误：" << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }

    }
}

void http_server::run() {
    accept();
    //io_service_sp->run();
    boost::asio::post(thread_pool, [this]() { run_service_io(); });
}

void http_server::run_service_io() const {
    io_service_sp->run();
}

void http_server::handle_client(socket_shared_ptr socket) {
    try {
        boost::beast::flat_buffer request_buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;
        boost::system::error_code err;
        boost::beast::http::read(*socket, request_buffer, req, err);
        if (err) {
            std::cerr << "read: " << err.message() << "\n";
            return;
        }

        // 解析请求路径
        std::string target = std::string(req.target());
        std::cout << "request path: " << target << std::endl;

        if (target == "/device") {
            handle_device_request(socket, req);
        } else if (target == "/") {
            handle_device_request(socket, req);
        } else {
            for (const auto &c: frames) {
                std::cout << c.first << std::endl;
                if (std::string(target.begin() + 1, target.end()) == c.first) {
                    handle_sn_request(socket, req, std::string(target.begin() + 1, target.end()));
                }
            }
            handle_not_found(socket, req);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void http_server::handle_device_request(socket_shared_ptr socket,
                                        const boost::beast::http::request<boost::beast::http::string_body> &req) {
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "<html><body><h1>Boost MJPEG Streamer</h1></body>";

    for (const auto &c: frames) {
        res.body() += "<body><h2><a href=\"http://127.0.0.1:8080/" + c.first + "\">" + c.first + "</a></h2></body>";
    }

    res.body() += "</html>";
    res.prepare_payload();

    boost::system::error_code err;
    boost::beast::http::write(*socket, res, err);
    if (err) {
        std::cerr << "write: " << err.message() << "\n";
    }
}

void http_server::handle_not_found(socket_shared_ptr socket,
                                   const boost::beast::http::request<boost::beast::http::string_body> &req) {
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found,
                                                                      req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "<html><body><h1>404 Not Found</h1></body></html>";
    res.prepare_payload();

    boost::system::error_code err;
    boost::beast::http::write(*socket, res, err);
    if (err) {
        std::cerr << "write: " << err.message() << "\n";
    }
}

void http_server::handle_sn_request(socket_shared_ptr socket,
                                    const boost::beast::http::request<boost::beast::http::string_body> &req,
                                    std::string sn) {
    boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "multipart/x-mixed-replace; boundary=frame");
    res.keep_alive();
    boost::beast::http::response_serializer<boost::beast::http::empty_body> sr{res};
    boost::system::error_code err;
    boost::beast::http::write_header(*socket, sr, err);
    if (err) {
        std::cerr << "write: " << err.message() << "\n";
        return;
    }

    while (true) {
        auto const size = frames[sn]->size();
        boost::beast::http::response<boost::beast::http::vector_body<char>> res{std::piecewise_construct,
                                                                                std::make_tuple(
                                                                                        *frames[sn]),
                                                                                std::make_tuple(
                                                                                        boost::beast::http::status::ok,
                                                                                        req.version())};
        res.set(boost::beast::http::field::body, "--frame");
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "image/jpeg");
        res.content_length(size);
        res.keep_alive(true);
        boost::beast::http::write(*socket, res, err);
        if (err) {
            std::cerr << "write: " << err.message() << "\n";
            break;
        }
    }
}

void http_server::accept() {
    socket_shared_ptr sock = std::make_shared<socket>(*io_service_sp);
    acceptor_sp->async_accept(*sock,
                              boost::bind(
                                      &http_server::accept_handler,
                                      this,
                                      boost::asio::placeholders::error,
                                      sock
                              )
    );
}

void http_server::accept_handler(const boost::system::error_code &ec, socket_shared_ptr sock) {
    if (ec) {
        this->accept();
        std::cout << ec.what() << std::endl;
        return;
    }
    boost::asio::post(thread_pool, [this, sock]() { handle_client(sock); });
    this->accept();
}

void http_server::publish(const std::string &sn, std::shared_ptr<std::vector<char>> frame_shared_ptr) {
    frames[sn] = frame_shared_ptr;
}

void http_server::close(const std::string &sn) {
    frames.erase(sn);
}
