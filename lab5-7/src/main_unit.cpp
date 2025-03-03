#include <iostream>
#include <string>
#include <zmq.hpp>
#include <mutex>
#include "bintree.hpp"
#include <thread>
#include <vector>
#include <sstream>

class HyperUnit
{
public:
    HyperUnit() : ctx(1) {};

    void createNode(int id)
    {
        std::lock_guard<std::mutex> lock(node_mutex);
        
        // Проверяем существование узла с таким id
        if (tree.find(id))
        {
            std::cout << "Error: Already exists\n";
            return;
        }

        int port = 55010 + id;
        pid_t pid = fork();
        
        if (pid == -1)
        {
            std::cout << "Error: Could not fork\n";
            return;
        }
        
        if (pid == 0) // Процесс-потомок
        {
            const char *args[] = {"./subunit", std::to_string(port).c_str(), "0", std::to_string(id).c_str(), nullptr};
            execv(args[0], const_cast<char *const *>(args));
            exit(1);
        }

        // Родительский процесс
        tree.insert(id, port);
        std::cout << "Ok: " << pid << '\n';
        
        // Находим родителя для нового узла
        Node* root = tree.findRoot();
        
        // Если это первый узел, он становится корнем дерева
        if (root->id == id)
            return;

        // Находим родительский узел в дереве
        Node* parent = findParentNode(root, id);
        
        if (parent == nullptr) {
            std::cout << "Error: Could not find parent node\n";
            return;
        }

        // Регистрируем новый узел у родителя
        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(parent->port));
        
        std::string cmd = "register_child " + std::to_string(id) + " " + std::to_string(port);
        socket.send(zmq::buffer(cmd), zmq::send_flags::none);
        
        // Ждем подтверждения
        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
        
        // Здесь можно было бы обработать ошибку, но для простоты опустим
    }

    Node* findParentNode(Node* current, int id) {
        if (current == nullptr) 
            return nullptr;
        
        // Проверяем, является ли текущий узел родителем для id
        if ((current->left != nullptr && current->left->id == id) || 
            (current->right != nullptr && current->right->id == id)) {
            return current;
        }
        
        // Ищем в дереве рекурсивно
        Node* result = nullptr;
        
        if (id < current->id) 
            result = findParentNode(current->left, id);
        else 
            result = findParentNode(current->right, id);
        
        return result;
    }

    void pingNode(int id)
    {
        std::lock_guard<std::mutex> lock(node_mutex);

        Node* node = tree.find(id);
        if (node == nullptr)
        {
            std::cout << "Error: Node was not found\n";
            return;
        }

        // Находим корневой узел для отправки команды
        Node* root = tree.findRoot();
        if (root == nullptr) {
            std::cout << "Error: No root node\n";
            return;
        }

        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(root->port));
        int timeout = 2000;
        socket.set(zmq::sockopt::rcvtimeo, timeout);

        // Отправляем команду ping с указанием целевого id
        std::string cmd = "ping " + std::to_string(id);
        socket.send(zmq::buffer(cmd), zmq::send_flags::none);

        // Ждем ответ
        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
        
        if (res.has_value() && reply.to_string() == "Ok")
        {
            std::cout << "Ok: 1\n";
            tree.setNodeStatus(id, true);
        }
        else
        {
            std::cout << "Ok: 0\n";
            tree.setNodeStatus(id, false);
        }
    }

    void executeCommand(int id, std::string params)
    {
        std::lock_guard<std::mutex> lock(node_mutex);

        Node* node = tree.find(id);
        if (node == nullptr)
        {
            std::cout << "Error: Node was not found\n";
            return;
        }

        // Проверяем статус узла
        if (!tree.getNodeStatus(id))
        {
            std::cout << "Error: Node is not responding\n";
            return;
        }

        // Находим корневой узел
        Node* root = tree.findRoot();
        if (root == nullptr) {
            std::cout << "Error: No root node\n";
            return;
        }

        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(root->port));
        int timeout = 3000;
        socket.set(zmq::sockopt::rcvtimeo, timeout);

        // Формируем команду exec с указанием целевого id
        std::istringstream iss(params);
        int n;
        iss >> n;

        std::string command = "exec " + std::to_string(id) + " " + std::to_string(n);
        for (int i = 0; i < n; i++)
        {
            int num;
            if (iss >> num)
            {
                command += " " + std::to_string(num);
            }
        }

        // Отправляем команду корневому узлу для маршрутизации
        socket.send(zmq::buffer(command), zmq::send_flags::none);

        // Ждем ответа
        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);

        if (res.has_value())
        {
            std::string reply_str = reply.to_string();
            if (reply_str.find("Error") != std::string::npos) {
                std::cout << reply_str << std::endl;
                tree.setNodeStatus(id, false);
            } else {
                std::cout << "Ok:" << id << ": " << reply_str << std::endl;
            }
        }
        else
        {
            std::cout << "Error: Communication failed\n";
            tree.setNodeStatus(id, false);
        }
    }

private:
    zmq::context_t ctx{1};
    BinaryTree tree;
    std::mutex node_mutex;
};

int main()
{
    HyperUnit manager;
    std::string command;

    while (std::cout << "> " && std::getline(std::cin, command))
    {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "create")
        {
            int id;
            iss >> id;
            manager.createNode(id);
        }
        else if (cmd == "exec")
        {
            int id;
            iss >> id;
            std::string params;
            std::getline(iss, params);
            manager.executeCommand(id, params);
        }
        else if (cmd == "ping")
        {
            int id;
            iss >> id;
            manager.pingNode(id);
        }
        else if (cmd == "exit")
        {
            break;
        }
        else
        {
            std::cout << "Error: Unknown command\n";
        }
    }

    return 0;
}