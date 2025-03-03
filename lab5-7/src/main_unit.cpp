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
        Node *node = root.find(id);
        if (node != nullptr)
        {
            std::cout << "Error: Already exists\n";
            return;
        }

        // Определяем порт для нового узла
        int port = 55040 + id;
        int parent_port = 0;

        // Если это не первый узел, находим родителя для этого узла
        if (!root.isEmpty()) {
            // Добавим узел в дерево, чтобы определить его родительский порт
            root.insert(id, port);
            node = root.find(id);
            parent_port = node->parent_port;
        } else {
            // Если это первый узел, просто добавляем его в дерево
            root.insert(id, port);
            node = root.find(id);
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            std::cout << "Error: Could not fork\n";
            root.remove(id); // Удаляем узел из дерева, так как не смогли создать процесс
            return;
        }

        if (pid == 0)
        {
            // Дочерний процесс
            const char *args[] = {"./subunit", std::to_string(port).c_str(), 
                                std::to_string(parent_port).c_str(), nullptr};
            execv(args[0], const_cast<char *const *>(args));
            exit(1);
        }

        // Даем время узлу на инициализацию
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Регистрируем связи между родителем и новым узлом
        if (parent_port != 0) {
            // Найдем родительский узел
            Node* parent = findNodeByPort(parent_port);
            if (parent != nullptr) {
                // Регистрируем родителя у нового узла
                registerNodeWith(id, parent->id, parent->port);
                
                // Регистрируем новый узел у родителя
                registerNodeWith(parent->id, id, port);
            }
        }

        std::cout << "Ok: " << pid << '\n';
    }

    void pingNode(int id)
    {
        std::lock_guard<std::mutex> lock(node_mutex);

        Node *node = root.find(id);
        if (node == nullptr)
        {
            std::cout << "Error: Node was not found\n";
            return;
        }

        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(node->port));
        int timeout = 1000;

        socket.set(zmq::sockopt::rcvtimeo, timeout);

        const std::string data{"ping"};

        socket.send(zmq::buffer(data), zmq::send_flags::none);

        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
        if (res.has_value())
        {
            std::cout << "Ok: 1\n";
            node->is_alive = true;
        }
        else
        {
            std::cout << "Ok: 0\n";
            node->is_alive = false;
        }
    }

    void executeCommand(int id, std::string params)
    {
        std::lock_guard<std::mutex> lock(node_mutex);

        Node *node = root.find(id);
        if (node == nullptr)
        {
            std::cout << "Error: Node was not found\n";
            return;
        }

        if (!node->is_alive)
        {
            std::cout << "Error: Node is not responding\n";
            return;
        }

        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(node->port));

        int timeout = 2000;
        socket.set(zmq::sockopt::rcvtimeo, timeout);

        std::istringstream iss(params);
        int n;
        iss >> n;

        std::string command = "exec " + std::to_string(n);
        for (int i = 0; i < n; i++)
        {
            int num;
            if (iss >> num)
            {
                command += " " + std::to_string(num);
            }
        }

        socket.send(zmq::buffer(command), zmq::send_flags::none);

        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);

        if (res.has_value())
        {
            std::cout << "Ok:" << id << ": " << reply.to_string() << std::endl;
        }
        else
        {
            std::cout << "Error: Node is not responding\n";
            node->is_alive = false;
        }
    }

    // Метод отправки сообщения через дерево
    void sendMessage(int from_id, int to_id, const std::string& message)
    {
        std::lock_guard<std::mutex> lock(node_mutex);

        Node *from_node = root.find(from_id);
        Node *to_node = root.find(to_id);

        if (from_node == nullptr)
        {
            std::cout << "Error: Source node not found\n";
            return;
        }
        
        if (to_node == nullptr)
        {
            std::cout << "Error: Target node not found\n";
            return;
        }

        if (!from_node->is_alive)
        {
            std::cout << "Error: Source node is not responding\n";
            return;
        }

        // Получаем путь в дереве от from_node к to_node
        std::vector<int> path = findPathInTree(from_id, to_id);
        
        if (path.empty()) {
            std::cout << "Error: No path between nodes\n";
            return;
        }

        // Отправка сообщения по найденному пути
        std::string response;
        if (routeMessage(from_id, to_id, path, message, response)) {
            std::cout << "Ok: " << response << std::endl;
        } else {
            std::cout << "Error: Failed to deliver message\n";
        }
    }

    // Вывод текущей топологии дерева
    void printTopology() {
        std::cout << "Текущая топология дерева:" << std::endl;
        printTopologyRecursive(root.getRoot(), 0);
    }

private:
    zmq::context_t ctx{1};
    BinaryTree root;
    std::mutex node_mutex;

    // Находит узел по его порту
    Node* findNodeByPort(int port) {
        std::vector<Node*> all_nodes;
        collectNodes(root.getRoot(), all_nodes);
        
        for (auto node : all_nodes) {
            if (node->port == port) {
                return node;
            }
        }
        return nullptr;
    }

    // Собирает все узлы из дерева
    void collectNodes(Node* node, std::vector<Node*>& nodes) {
        if (node == nullptr)
            return;
            
        nodes.push_back(node);
        
        if (node->left) 
            collectNodes(node->left, nodes);
        if (node->right) 
            collectNodes(node->right, nodes);
    }

    // Регистрирует узел node_id с портом node_port у узла host_id
    void registerNodeWith(int host_id, int node_id, int node_port)
    {
        Node *host = root.find(host_id);
        if (host == nullptr || !host->is_alive)
            return;

        zmq::socket_t socket(ctx, zmq::socket_type::req);
        socket.connect("tcp://localhost:" + std::to_string(host->port));
        
        int timeout = 1000;
        socket.set(zmq::sockopt::rcvtimeo, timeout);
        
        std::string command = "node_info " + std::to_string(node_id) + " " + std::to_string(node_port);
        
        socket.send(zmq::buffer(command), zmq::send_flags::none);
        
        zmq::message_t reply{};
        zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
        
        if (!res.has_value())
            host->is_alive = false;
    }

    // Находит путь между двумя узлами в дереве
    std::vector<int> findPathInTree(int from_id, int to_id) {
        // Сначала находим путь от корня к источнику
        std::vector<int> path_to_source;
        findPathFromRoot(root.getRoot(), from_id, path_to_source);
        
        // Затем находим путь от корня к получателю
        std::vector<int> path_to_target;
        findPathFromRoot(root.getRoot(), to_id, path_to_target);
        
        // Если не нашли путь к одному из узлов, возвращаем пустой путь
        if (path_to_source.empty() || path_to_target.empty()) {
            return std::vector<int>();
        }
        
        // Находим общего предка
        int common_ancestor_index = 0;
        size_t min_len = std::min(path_to_source.size(), path_to_target.size());
        
        for (; common_ancestor_index < min_len; ++common_ancestor_index) {
            if (path_to_source[common_ancestor_index] != path_to_target[common_ancestor_index]) {
                break;
            }
        }
        
        // Строим полный путь: идём от источника до общего предка, затем к получателю
        std::vector<int> full_path;
        
        // Добавляем узлы от источника до общего предка (в обратном порядке)
        for (int i = path_to_source.size() - 1; i >= common_ancestor_index - 1; --i) {
            full_path.push_back(path_to_source[i]);
        }
        
        // Добавляем узлы от общего предка к получателю
        for (size_t i = common_ancestor_index; i < path_to_target.size(); ++i) {
            full_path.push_back(path_to_target[i]);
        }
        
        return full_path;
    }

    // Находит путь от корня дерева к указанному узлу
    bool findPathFromRoot(Node* root_node, int target_id, std::vector<int>& path) {
        if (root_node == nullptr)
            return false;
        
        // Добавляем текущий узел в путь
        path.push_back(root_node->id);
        
        // Если нашли целевой узел, возвращаем true
        if (root_node->id == target_id)
            return true;
        
        // Ищем в левом и правом поддереве
        if ((root_node->left && findPathFromRoot(root_node->left, target_id, path)) ||
            (root_node->right && findPathFromRoot(root_node->right, target_id, path)))
            return true;
        
        // Если не нашли узел в этом пути, удаляем текущий узел из пути и возвращаем false
        path.pop_back();
        return false;
    }

    // Маршрутизирует сообщение по указанному пути
    bool routeMessage(int from_id, int to_id, const std::vector<int>& path, 
                     const std::string& message, std::string& response) {
        if (path.size() < 2) { // Требуется как минимум источник и получатель
            return false;
        }
        
        // Отправляем сообщение от узла к узлу по пути
        int current_sender = path[0]; // Начинаем с исходного узла
        
        for (size_t i = 1; i < path.size(); ++i) {
            int current_receiver = path[i];
            
            // Проверяем доступность текущего отправителя
            Node* sender_node = root.find(current_sender);
            if (sender_node == nullptr || !sender_node->is_alive) {
                return false;
            }
            
            // Формируем команду для отправки
            std::string route_command;
            
            if (current_receiver == to_id) {
                // Если это последний узел в пути, отправляем оригинальное сообщение
                route_command = "send " + std::to_string(current_receiver) + " " + message;
            } else {
                // Иначе, отправляем команду маршрутизации
                route_command = "send " + std::to_string(current_receiver) + " [ROUTE:" + 
                               std::to_string(from_id) + "->" + std::to_string(to_id) + "] " + message;
            }
            
            // Отправляем команду текущему отправителю
            zmq::socket_t socket(ctx, zmq::socket_type::req);
            socket.connect("tcp://localhost:" + std::to_string(sender_node->port));
            
            int timeout = 2000;
            socket.set(zmq::sockopt::rcvtimeo, timeout);
            
            socket.send(zmq::buffer(route_command), zmq::send_flags::none);
            
            zmq::message_t reply{};
            zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
            
            if (!res.has_value()) {
                sender_node->is_alive = false;
                return false;
            }
            
            // Если это был последний шаг, сохраняем ответ
            if (i == path.size() - 1) {
                response = reply.to_string();
            }
            
            // Для следующей итерации получатель становится отправителем
            current_sender = current_receiver;
        }
        
        return true;
    }

    // Рекурсивный вывод топологии дерева
    void printTopologyRecursive(Node* node, int level) {
        if (node == nullptr) return;
        
        // Выводим отступ соответственно уровню узла
        for (int i = 0; i < level; ++i) {
            std::cout << "  ";
        }
        
        // Выводим информацию о текущем узле
        std::cout << "ID: " << node->id << " (Port: " << node->port 
                  << ", Parent: " << node->parent_port 
                  << ", Status: " << (node->is_alive ? "Alive" : "Dead") << ")" << std::endl;
        
        // Рекурсивно выводим левое и правое поддерево
        printTopologyRecursive(node->left, level + 1);
        printTopologyRecursive(node->right, level + 1);
    }
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
        else if (cmd == "send")
        {
            int from_id, to_id;
            iss >> from_id >> to_id;
            std::string message;
            std::getline(iss, message);
            if (message.empty() || message[0] == ' ')
                message = message.substr(1);  // Удаляем начальный пробел
            manager.sendMessage(from_id, to_id, message);
        }
        else if (cmd == "tree")
        {
            manager.printTopology();
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