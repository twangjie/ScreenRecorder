//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "server.hpp"

#include <map>
#include <vector>

using namespace std;
using namespace http::server2;

class Worker : public ITcpCallbackSink
{
public:
	Worker()
	{
		
	}
	
	int OnConnect(connection_ptr conn) override
	{
		std::cout << "OnConnect\n";

		if (true)
		{
			request *req = new request;
			request_parser *req_parser = new request_parser;

			boost::recursive_mutex::scoped_lock lock(request_mutex_);
			request_[conn] = req;
			request_parser_[conn] = req_parser;
		}
		
		return 0;
	}

	int OnDisconnected(connection_ptr conn) override
	{

		boost::recursive_mutex::scoped_lock lock(request_mutex_);
		request_.erase(conn);
		request_parser_.erase(conn);

		std::cout << "OnDisconnected\n";
		return 0;
	}

	int OnRecvData(http::server2::connection_ptr conn, char* data, int len) override
	{
		if (true)
		{
			boost::recursive_mutex::scoped_lock conlock(conn_mutex_);

			auto iter = url_producer_map_.find(conn);
			if(iter != url_producer_map_.end())
			{
				auto iterCon = url_consumer_map_.find(iter->second);
				if(iterCon != url_consumer_map_.end())
				{
					for(connection_ptr cli : url_consumer_map_[iter->second])
					{
						if(cli.get())
						{
							cli->sendData(data, len);
						}
					}
				}

				return 1;
			}
		}

		if(true)
		{
			boost::recursive_mutex::scoped_lock lock(request_mutex_);

			auto iter = request_parser_.find(conn);
			if (iter != request_parser_.end())
			{
				auto req = request_[conn];

				boost::tribool result;
				boost::tie(result, boost::tuples::ignore) = iter->second->parse(*req, data, data + len);

				if (result)
				{
					std::cout << req->method << std::endl;

					if (req->method == "POST")
					{
						url_producer_map_[conn] = req->uri;
					}
					else if (req->method == "GET")
					{
						url_consumer_map_[req->uri].push_back(conn);
						url_consumer_conn_map_[conn] = conn;
					}
					return 0;
				}
			}
		}
	

		return 0;
	}

private:

	boost::recursive_mutex								conn_mutex_;
	std::map<connection_ptr, std::string>				url_producer_map_;
	std::map<connection_ptr, connection_ptr>			url_consumer_conn_map_;
	std::map<std::string, std::vector<connection_ptr>>	url_consumer_map_;

	boost::recursive_mutex						request_mutex_;
	std::map<connection_ptr, request *>			request_;
	std::map<connection_ptr, request_parser *>	request_parser_;
};

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 5)
    {
      std::cerr << "Usage: http_server <address> <port> <threads> <doc_root>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    receiver 0.0.0.0 80 1 .\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    receiver 0::0 80 1 .\n";
      return 1;
    }

	Worker worker;

    // Initialise the server.
    std::size_t num_threads = boost::lexical_cast<std::size_t>(argv[3]);
    http::server2::server s(argv[1], argv[2], argv[4], num_threads);
	s.setCallbackSink(&worker);

    // Run the server until stopped.
    s.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}
