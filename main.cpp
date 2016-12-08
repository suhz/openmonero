#include <iostream>

#include <memory>
#include <cstdlib>

#include "ext/restbed/source/restbed"
#include "ext/date/tz.h"
#include "ext/json.hpp"

using namespace std;
using namespace restbed;
using namespace nlohmann;

string
get_current_time(const char* format = "%a, %d %b %Y %H:%M:%S %Z")
{

   auto current_time = date::make_zoned(
           date::current_zone(),
           date::floor<chrono::seconds>(std::chrono::system_clock::now())
   );

   return date::format(format, current_time);
}

multimap<string, string>
make_headers(const multimap<string, string>& extra_headers = multimap<string, string>())
{
    multimap<string, string> headers {
        {"Date", get_current_time()},
        {"Access-Control-Allow-Origin",      "https://localhost"},
        {"access-control-allow-headers",     "*, DNT,X-CustomHeader,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Set-Cookie"},
        {"access-control-max-age",           "86400, 1728000"},
        {"access-control-allow-methods",     "GET, POST, OPTIONS"},
        {"access-control-allow-credentials", "true"},
        {"Content-Type",                     "application/json"}
    };

    headers.insert(extra_headers.begin(), extra_headers.end());

    return headers;
};



void
generic_options_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0);

    cout << "options_method_handler" << endl;

    session->fetch(content_length, [ ]( const shared_ptr< Session > session, const Bytes & body )
    {
        string _body(reinterpret_cast<const char *>(body.data()), body.size());

        cout << "requriest options_method_handler" << endl;

        session->close( OK, string{}, make_headers());
    } );
}

struct handel_
{
    using fetch_func = function< void ( const shared_ptr< Session >, const Bytes& ) >;

    fetch_func request_callback;

    handel_(const fetch_func& callback):
            request_callback {callback}
    {}


    void operator()(const shared_ptr< Session > session)
    {
        const auto request = session->get_request( );

        size_t content_length = request->get_header( "Content-Length", 0);

        cout << "post_get_address_info_handler" << endl;

        session->fetch(content_length, this->request_callback);
    }
};

shared_ptr<Resource>
make_resource(const string& path, const handel_& handler)
{
    shared_ptr<Resource> resource_ptr = make_shared<Resource>();

    resource_ptr->set_path(path);
    resource_ptr->set_method_handler( "OPTIONS", generic_options_handler);
    resource_ptr->set_method_handler( "POST", handler );

    return resource_ptr;
}

int
main()
{

    auto request_login = [](const shared_ptr< Session > session, const Bytes & body)
    {
        string _body(reinterpret_cast<const char *>(body.data()), body.size());

        cout << _body << endl;

        json j_response {{"new_address", true}};

        string response_body = j_response.dump();

        auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

        session->close( OK, response_body, response_headers);
    };

    auto request_get_address_txs = [](const shared_ptr< Session > session, const Bytes & body)
    {
        string _body(reinterpret_cast<const char *>(body.data()), body.size());

        cout << _body << endl;

        json j_response {
                { "total_received", "0"},
                { "scanned_height", 2012455},
                { "scanned_block_height", 1195848},
                { "start_height", 2012455},
                { "transaction_height", 2012455},
                { "blockchain_height", 1195848}
        };

        string response_body = j_response.dump();

        auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

        session->close( OK, response_body, response_headers);
    };

    auto request_get_address_info = [](const shared_ptr< Session > session, const Bytes & body)
    {
        string _body(reinterpret_cast<const char *>(body.data()), body.size());

        cout << _body << endl;

        json j_response  {
                {"locked_funds", "0"},
                {"total_received", "0"},
                {"total_sent", "0"},
                {"scanned_height", 2012470},
                {"scanned_block_height", 1195850},
                {"start_height", 2012470},
                {"transaction_height", 2012470},
                {"blockchain_height", 1195850},
                {"spent_outputs", nullptr}
        };

        string response_body = j_response.dump();

        auto response_headers = make_headers({{ "Content-Length", to_string(response_body.size())}});

        session->close( OK, response_body, response_headers);
    };


    auto login            = make_resource("/login"           , handel_(request_login));
    auto get_address_txs  = make_resource("/get_address_txs" , handel_(request_get_address_txs));
    auto get_address_info = make_resource("/get_address_info", handel_(request_get_address_txs));

    bool use_ssl {true};

    auto settings = make_shared< Settings >( );

    if (use_ssl)
    {
        auto ssl_settings = make_shared< SSLSettings >( );

        ssl_settings->set_http_disabled( true );
        ssl_settings->set_port(1984);
        ssl_settings->set_private_key( Uri( "file:///tmp/mwo.key" ) );
        ssl_settings->set_certificate( Uri( "file:///tmp/mwo.crt" ) );
        ssl_settings->set_temporary_diffie_hellman( Uri( "file:///tmp/dh2048.pem" ) );

        settings->set_ssl_settings( ssl_settings );
    }

    //settings->set_default_header("Connection", "close" );
    //settings->set_port(1984);


    cout << "Start the service at https://localhost:1984" << endl;

    Service service;
    service.publish(login);
    service.publish(get_address_txs);
    service.publish(get_address_info);
    service.start( settings );

    return EXIT_SUCCESS;
}