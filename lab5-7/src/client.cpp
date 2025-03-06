#include <signal.h>

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#include "net_func.h"
#include "node.h"
#include "set"

static std::mutex nodes_mutex;

int main() {
    std::set<int> all_nodes;
    std::string prog_path = "./worker";
    Node me(-1);
    all_nodes.insert(-1);

    std::string command;
    while (std::cin >> command) {
        if (command == "create") {
            int id_child, id_parent;
            std::cin >> id_child >> id_parent;

            if (all_nodes.find(id_child) != all_nodes.end()) {
                std::cout << "Error: Already exists" << std::endl;
            }
            else if (all_nodes.find(id_parent) == all_nodes.end()) {
                std::cout << "Error: Parent not found" << std::endl;
            } 
            else if (id_parent == me.id) {
                std::string ans = me.Create_child(id_child, prog_path);
                std::cout << ans << std::endl;
                all_nodes.insert(id_child);
            } 
            else {
                std::string str = "create " + std::to_string(id_child);
                std::string ans = me.Send(str, id_parent);
                std::cout << ans << std::endl;
                all_nodes.insert(id_child);
            }
        } 
        else if (command == "ping") {
            int id_child;
            std::cin >> id_child;
            if (all_nodes.find(id_child) == all_nodes.end()) {
                std::cout << "Error: Not found" << std::endl;
            } 
            else if (me.children.find(id_child) != me.children.end()) {
                std::string ans = me.Ping_child(id_child);
                std::cout << ans << std::endl;
            } 
            else {
                std::string str = "ping " + std::to_string(id_child);
                std::string ans = me.Send(str, id_child);
                if (ans == "Error: not find") {
                    ans = "Ok: 0";
                }
                std::cout << ans << std::endl;
            }
        }
        else if (command == "exec") {
            int id, n;
            std::cin >> id >> n;
            
            std::string msg = "exec " + std::to_string(n);
            for (int i = 0; i < n; i++) {
                int num;
                std::cin >> num;
                msg += " " + std::to_string(num);
            }

            if (all_nodes.find(id) == all_nodes.end()) {
                std::cout << "Error: Not found" << std::endl;
            } 
            else {
                std::string ping_result =
                    me.Send("ping " + std::to_string(id), id);
                if (ping_result == "Ok: 0" ||
                    ping_result == "Error: not find") {
                    std::cout << "Error: Node is unavailable" << std::endl;
                    all_nodes.erase(id);
                } 
                else {
                    std::string ans = me.Send(msg, id);
                    std::cout << ans << std::endl;
                }
            }
        } 
    }

    me.Remove();
    return 0;
}