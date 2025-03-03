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
    using namespace std::chrono_literals;

    if (argc < 2)
    {
        std::cerr << "Usage: ./worker <port> [parent_id] [own_id]" << std::endl;
        return 1;
    }

    int own_port = std::stoi(argv[1]);
    int own_id = (argc > 3) ? std::stoi(argv[3]) : 0;
    
    // ID и порты для левого и правого дочерних узлов
    int left_id = -1, right_id = -1;
    int left_port = -1, right_port = -1;
    
    // Инициализация ZMQ контекста
    zmq::context_t ctx;
    
    // Сокет для приема команд от родителя или клиента
    zmq::socket_t main_socket{ctx, zmq::socket_type::rep};
    main_socket.bind("tcp://localhost:" + std::string(argv[1]));

    // Сокеты для связи с дочерними узлами
    std::map<int, zmq::socket_t> child_sockets;
    
    for (;;)
    {
        zmq::message_t request;

        // Принимаем запрос
        zmq::recv_result_t res = main_socket.recv(request, zmq::recv_flags::none);
        if (!res.has_value())
        {
            std::cerr << "Error receiving message" << std::endl;
            continue;
        }

        std::string request_str = request.to_string();
        std::string response;
        std::istringstream iss(request_str);
        std::string cmd;
        iss >> cmd;

        if (cmd == "ping")
        {
            int target_id;
            iss >> target_id;
            
            // Если запрос к текущему узлу
            if (target_id == own_id || target_id == 0)
            {
                response = "Ok";
            }
            // Иначе перенаправляем запрос к нужному дочернему узлу
            else 
            {
                bool child_found = false;
                
                // Определяем путь: если ID меньше текущего, идем влево, иначе вправо
                int child_id = (target_id < own_id) ? left_id : right_id;
                int child_port = (target_id < own_id) ? left_port : right_port;
                
                if (child_id != -1 && child_port != -1)
                {
                    // Создаем сокет для связи с дочерним узлом, если он еще не существует
                    if (child_sockets.find(child_id) == child_sockets.end())
                    {
                        child_sockets[child_id] = zmq::socket_t(ctx, zmq::socket_type::req);
                        child_sockets[child_id].connect("tcp://localhost:" + std::to_string(child_port));
                        child_sockets[child_id].set(zmq::sockopt::rcvtimeo, 1000);
                    }
                    
                    // Пересылаем запрос
                    child_sockets[child_id].send(zmq::buffer(request_str), zmq::send_flags::none);
                    
                    // Ждем ответа
                    zmq::message_t child_reply;
                    auto child_res = child_sockets[child_id].recv(child_reply, zmq::recv_flags::none);
                    
                    if (child_res.has_value())
                    {
                        response = child_reply.to_string();
                        child_found = true;
                    }
                }
                
                if (!child_found)
                {
                    response = "Error: Unable to reach node";
                }
            }
        }
        else if (cmd == "exec")
        {
            int target_id;
            iss >> target_id;
            
            // Если команда для текущего узла
            if (target_id == own_id || target_id == 0)
            {
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
            else
            {
                // Перенаправляем запрос, аналогично команде ping
                bool child_found = false;
                int child_id = (target_id < own_id) ? left_id : right_id;
                int child_port = (target_id < own_id) ? left_port : right_port;
                
                if (child_id != -1 && child_port != -1)
                {
                    if (child_sockets.find(child_id) == child_sockets.end())
                    {
                        child_sockets[child_id] = zmq::socket_t(ctx, zmq::socket_type::req);
                        child_sockets[child_id].connect("tcp://localhost:" + std::to_string(child_port));
                        child_sockets[child_id].set(zmq::sockopt::rcvtimeo, 2000);
                    }
                    
                    child_sockets[child_id].send(zmq::buffer(request_str), zmq::send_flags::none);
                    
                    zmq::message_t child_reply;
                    auto child_res = child_sockets[child_id].recv(child_reply, zmq::recv_flags::none);
                    
                    if (child_res.has_value())
                    {
                        response = child_reply.to_string();
                        child_found = true;
                    }
                }
                
                if (!child_found)
                {
                    response = "Error: Unable to reach node";
                }
            }
        }
        else if (cmd == "register_child")
        {
            int child_id, child_port;
            iss >> child_id >> child_port;
            
            if (child_id < own_id)
            {
                left_id = child_id;
                left_port = child_port;
            }
            else
            {
                right_id = child_id;
                right_port = child_port;
            }
            response = "ChildRegistered";
        }
        else
        {
            response = "Error: Unknown command";
        }

        // Отправляем ответ
        main_socket.send(zmq::buffer(response), zmq::send_flags::none);
    }

    return 0;
}