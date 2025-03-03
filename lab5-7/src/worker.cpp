#include <signal.h>

#include <chrono>
#include <fstream>
#include <vector>

#include "net_func.h"
#include "node.h"

int my_id = 0;

static bool timer_running = false;
static std::chrono::time_point<std::chrono::steady_clock> start_time;
static std::chrono::duration<double> elapsed(0.0);

void Log(std::string str) {
    std::string f = std::to_string(my_id) + ".txt";
    std::ofstream fout(f, std::ios_base::app);
    fout << str;
    fout.close();
}

int main(int argc, char **argv) {
    if (argc != 3) {
        return -1;
    }

    Node me(atoi(argv[1]), atoi(argv[2]));
    my_id = me.id;
    std::string prog_path = "./worker";
    while (1) {
        if (getppid() ==
            1) {  // Если родительский процесс мертв, завершаем узел
            std::cout << "[WORKER] Parent process died. Exiting..."
                      << std::endl;
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
        } else if (command == "pid") {
            std::string ans = me.Pid();
            my_net::send_message(&me.parent, ans);
        } else if (command == "ping") {
            int id_child;
            request >> id_child;
            std::string ans = me.Ping_child(id_child);
            my_net::send_message(&me.parent, ans);
        } else if (command == "send") {
            int id;
            request >> id;
            std::string str;
            getline(request, str);
            str.erase(0, 1);
            std::string ans;
            ans = me.Send(str, id);
            my_net::send_message(&me.parent, ans);
        } else if (command == "exec") {
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
        } else if (command == "remove") {
            std::cout << "[WORKER] Removing node " << my_id << "..."
                      << std::endl;
            std::string ans = me.Remove();
            ans = std::to_string(me.id) + " " + ans;
            my_net::send_message(&me.parent, ans);
            my_net::disconnect(&me.parent, me.parent_port);
            me.parent.close();
            std::cout << "[WORKER] Node " << my_id << " removed successfully."
                      << std::endl;
            break;
        }
    }
    sleep(1);
    return 0;
}