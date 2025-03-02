#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <vector>
#include <zmq.hpp>

int main(int argc, char *argv[])
{
    using namespace std::chrono_literals;

    if (argc != 2)
    {
        std::cerr << "Usage: ./worker <endpoint>" << std::endl;
        return 1;
    }

    // initialize the zmq context with a single IO thread
    zmq::context_t ctx;

    // construct a REP (reply) socket and bind to interface
    zmq::socket_t socket{ctx, zmq::socket_type::rep};

    // Добавим опцию для повторного использования порта
    socket.bind("tcp://localhost:" + std::string(argv[1]));

    // prepare some static data for responses
    const std::string data{"World"};

    for (;;)
    {
        zmq::message_t request;

        // receive a request from client
        zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
        if (!res.has_value())
        {
            std::cerr << "Error receiving message: " << zmq_strerror(zmq_errno()) << std::endl;
            continue;
        }

        std::string request_str = request.to_string();
        std::string response;

        if (request_str == "ping")
        {
            response = "Ok";
        }
        else if (request_str.substr(0, 4) == "exec")
        {
            // Обработка команды exec
            std::istringstream iss(request_str.substr(5)); // Пропускаем "exec "
            int n;
            iss >> n; // Количество чисел

            long long sum = 0;
            int num;
            for (int i = 0; i < n && iss >> num; i++)
            {
                sum += num;
            }

            response = std::to_string(sum);
        }
        else
        {
            response = "Error: Unknown command";
        }

        // send the reply to the client
        socket.send(zmq::buffer(response), zmq::send_flags::none);
    }

    return 0;
}