#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <vector>
#include <map>
#include <zmq.hpp>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./worker <port> <parent_port>" << std::endl;
        return 1;
    }

    int my_port = std::stoi(argv[1]);
    int parent_port = std::stoi(argv[2]);

    // Инициализация ZMQ
    zmq::context_t ctx;

    // Сокет REP для получения команд от главного узла и других узлов
    zmq::socket_t socket{ctx, zmq::socket_type::rep};
    socket.bind("tcp://localhost:" + std::string(argv[1]));

    // Хранилище для известных узлов (ID -> порт)
    std::map<int, int> known_nodes;

    for (;;)
    {
        zmq::message_t request;

        // Получаем запрос
        zmq::recv_result_t res = socket.recv(request, zmq::recv_flags::none);
        if (!res.has_value())
        {
            std::cerr << "Error receiving message" << std::endl;
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
            // Текущая обработка exec
            std::istringstream iss(request_str.substr(5));
            int n;
            iss >> n;

            long long sum = 0;
            int num;
            for (int i = 0; i < n && iss >> num; i++)
            {
                sum += num;
            }

            response = std::to_string(sum);
        }
        else if (request_str.substr(0, 9) == "node_info")
        {
            // Регистрация информации о другом узле
            std::istringstream iss(request_str.substr(10));
            int node_id, node_port;
            if (iss >> node_id >> node_port)
            {
                known_nodes[node_id] = node_port;
                response = "Node " + std::to_string(node_id) + " registered";
            }
            else
            {
                response = "Invalid node info";
            }
        }
        else if (request_str.substr(0, 4) == "send")
        {
            // Обработка команды отправки сообщения другому узлу
            std::istringstream iss(request_str.substr(5));
            int target_id;
            std::string message;
            
            if (iss >> target_id && known_nodes.count(target_id) > 0)
            {
                std::getline(iss, message);
                
                // Создаем временный REQ сокет для отправки сообщения
                zmq::socket_t req_socket{ctx, zmq::socket_type::req};
                req_socket.connect("tcp://localhost:" + std::to_string(known_nodes[target_id]));
                
                // Устанавливаем таймаут
                int timeout = 1000;
                req_socket.set(zmq::sockopt::rcvtimeo, timeout);
                
                // Формируем сообщение
                std::string msg = "message" + message;
                
                // Отправляем и ждем ответа
                req_socket.send(zmq::buffer(msg), zmq::send_flags::none);
                
                zmq::message_t reply;
                auto reply_res = req_socket.recv(reply, zmq::recv_flags::none);
                
                if (reply_res.has_value())
                {
                    response = "Message delivered to node " + std::to_string(target_id) + 
                              ", response: " + reply.to_string();
                }
                else
                {
                    response = "Failed to deliver message to node " + std::to_string(target_id);
                }
            }
            else
            {
                response = "Unknown target node or invalid command";
            }
        }
        else if (request_str.substr(0, 7) == "message")
        {
            // Обработка входящего сообщения от другого узла
            std::string message_content = request_str.substr(7);
            std::cout << "Received message: " << message_content << std::endl;
            response = "Message received";
        }
        else
        {
            response = "Error: Unknown command";
        }

        // Отправляем ответ
        socket.send(zmq::buffer(response), zmq::send_flags::none);
    }

    return 0;
}