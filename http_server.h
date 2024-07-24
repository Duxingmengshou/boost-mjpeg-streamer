#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <cstdlib>

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/system.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>

class http_server {
public:
	typedef boost::asio::ip::tcp::acceptor acceptor;
	typedef std::shared_ptr<acceptor>  acceptor_shared_ptr;

	typedef boost::asio::ip::tcp::socket socket;
	typedef std::shared_ptr<socket> socket_shared_ptr;

	typedef boost::asio::io_service io_service;
	typedef std::shared_ptr<io_service> io_service_shared_ptr;

	typedef boost::asio::ip::tcp::iostream iostream_socket;
	typedef boost::asio::ip::tcp::endpoint endpoint;

	typedef socket* socket_ptr;

public:
	io_service_shared_ptr io_service_sp;
	acceptor_shared_ptr acceptor_sp;
	boost::asio::thread_pool thread_pool;
	std::map<std::string, std::vector<char>> frames;

public:
	http_server();
	void run();

	void run_service_io();

	void handle_client(socket_shared_ptr socket);

	void handle_device_request(socket_shared_ptr socket,
		const boost::beast::http::request<boost::beast::http::string_body>& req);

	void handle_not_found(socket_shared_ptr socket,
		const boost::beast::http::request<boost::beast::http::string_body>& req);

	void handle_sn_request(socket_shared_ptr socket,
		const boost::beast::http::request<boost::beast::http::string_body>& req, std::string sn);

	void accept();
	void accept_handler(const boost::system::error_code& ec,
		socket_shared_ptr sock);

	void publish(const std::string& sn, std::shared_ptr<std::vector<char>> frame_shared_ptr);
};