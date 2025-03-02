#include <iostream>
#include <string>
#include <zmq.hpp>
#include <mutex>
#include "bintree.hpp"
#include <thread>

class HyperUnit
{
public:
	HyperUnit() : ctx(1) {};

	void createNode(int id)
	{
		std::lock_guard<std::mutex> lock(node_mutex);
		if (root.find(id))
		{
			std::cout << "Error: Already exists\n";
			return;
		}

		pid_t pid = fork();
		if (pid == -1)
		{
			std::cout << "Error: Could not fork\n";
			return;
		}
		int port = 55010 + id;
		if (pid == 0)
		{
			const char *args[] = {"./subunit", std::to_string(port).c_str(), nullptr};
			execv(args[0], const_cast<char *const *>(args));
			exit(1);
		}

		root.insert(id, port);
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

		// set up some static data to send
		const std::string data{"ping"};

		// send the request message
		socket.send(zmq::buffer(data), zmq::send_flags::none);

		// wait for reply from server
		zmq::message_t reply{};
		zmq::recv_result_t res = socket.recv(reply, zmq::recv_flags::none);
		if (res.has_value())
		{
			std::cout << "Ok: 1\n";
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

		// Проверяем, жив ли узел
		if (!node->is_alive)
		{
			std::cout << "Error: Node is not responding\n";
			return;
		}

		zmq::socket_t socket(ctx, zmq::socket_type::req);
		socket.connect("tcp://localhost:" + std::to_string(node->port));

		// Устанавливаем таймаут для получения ответа
		int timeout = 2000; // 2 секунды
		socket.set(zmq::sockopt::rcvtimeo, timeout);

		// Формируем команду
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

		// Отправляем команду
		socket.send(zmq::buffer(command), zmq::send_flags::none);

		// Ждем ответа
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

private:
	zmq::context_t ctx{1};
	BinaryTree root;
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