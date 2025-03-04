#include <signal.h>

#include <chrono>
#include <fstream>
#include <vector>

#include "net_func.h"
#include "node.h"

int my_id = 0;

int main(int argc, char **argv) {
    if (argc != 3) {
        return -1;
    }

    Node me(atoi(argv[1]), atoi(argv[2]));
    my_id = me.id;
    std::string prog_path = "./worker";
    while (1) {
        if (getppid() == 1) {  // Если родительский процесс мертв, завершаем узел
            std::cout << "[WORKER] Parent process died. Exiting...\n";
            break;
        }
        std::string message;
        std::string command = " ";
        message = my_net::reseave(&(me.parent));
        std::istringstream request(message);
        request >> command;

        if (command == "create") {
            int id_child;
            request >> id_child;
            std::string ans = me.Create_child(id_child, prog_path);
            my_net::send_message(&me.parent, ans);
        } 
        else if (command == "pid") {
            std::string ans = me.Pid();
            my_net::send_message(&me.parent, ans);
        } 
        else if (command == "ping") {
            int id_child;
            request >> id_child;
            std::string ans = me.Ping_child(id_child);
            my_net::send_message(&me.parent, ans);
        } 
        else if (command == "send") {
            int id;
            request >> id;
            std::string str;
            getline(request, str);
            str.erase(0, 1);
            std::string ans;
            ans = me.Send(str, id);
            my_net::send_message(&me.parent, ans);
        } 
        else if (command == "exec") {
            int n;
            request >> n;
            int sum = 0;

            // Считываем и суммируем n чисел
            for (int i = 0; i < n; i++) {
                int num;
                request >> num;
                sum += num;
            }

            std::string ans =
                "Ok:" + std::to_string(me.id) + ":" + std::to_string(sum);
            my_net::send_message(&me.parent, ans);
        }
    }

    sleep(1);
    return 0;
}