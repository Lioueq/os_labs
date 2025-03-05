#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Job {
    std::string id;
    std::vector<std::string> dependencies; // IDs of jobs that this job depends on
    std::vector<std::string> next_jobs;    // IDs of jobs that depend on this job
    bool completed = false;
    bool failed = false;

    bool execute() {
        std::cout << "Executing job " << id << std::endl;
        if (failed) {
            return false;
        }
        return true;
    }
};

class DAGScheduler {
public:
    // Load DAG from a JSON file
    bool loadFromJson(const std::string& filename) {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error opening file: " << filename << std::endl;
                return false;
            }
            
            json config;
            file >> config;
            
            // Create jobs
            for (const auto& job_json : config["jobs"]) {
                std::string id = job_json["id"];
                Job* job = new Job();
                job->id = id;
                job->failed = job_json["failed"];
                
                if (job_json.contains("dependencies")) {
                    for (const auto& dep : job_json["dependencies"]) {
                        job->dependencies.push_back(dep);
                    }
                }
                
                jobs[id] = job;
            }
            
            // Set up job relationships
            for (auto& pair : jobs) {
                auto job = pair.second;
                for (const auto& dep_id : job->dependencies) {
                    if (jobs.find(dep_id) == jobs.end()) {
                        std::cerr << "Error: Dependency " << dep_id << " for job " << job->id << " does not exist" << std::endl;
                        return false;
                    }
                    jobs[dep_id]->next_jobs.push_back(job->id);
                }
            }
            
            // Identify start and end jobs
            for (const auto& pair : jobs) {
                if (pair.second->dependencies.empty()) {
                    start_jobs.push_back(pair.first);
                }
                if (pair.second->next_jobs.empty()) {
                    end_jobs.push_back(pair.first);
                }
            }
            
            return true;
        } 
        catch (const std::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Check for cycles using DFS
    bool checkNoCycles() {
        std::unordered_map<std::string, int> visited;
        
        for (const auto& pair : jobs) {
            visited[pair.first] = 0;
        }
        
        for (const auto& pair : jobs) {
            if (visited[pair.first] == 0) {
                if (hasCycleDFS(pair.first, visited)) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // Check for a single connected component
    bool checkSingleComponent() {
        if (jobs.empty()) return true;
        
        // Create an undirected graph (ignore edge direction)
        std::unordered_map<std::string, std::vector<std::string>> undirected_graph;
        for (const auto& pair : jobs) {
            undirected_graph[pair.first] = {};
            for (const auto& dep : pair.second->dependencies) {
                undirected_graph[pair.first].push_back(dep);
                undirected_graph[dep].push_back(pair.first);
            }
        }
        
        // BFS to check connectivity
        std::unordered_set<std::string> visited;
        std::queue<std::string> q;
        
        // Start with the first job
        auto first_job = jobs.begin()->first;
        q.push(first_job);
        visited.insert(first_job);
        
        while (!q.empty()) {
            auto current = q.front();
            q.pop();
            
            for (const auto& neighbor : undirected_graph[current]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }
        
        return visited.size() == jobs.size();
    }
    
    bool checkStartAndEndJobs() {
        return !start_jobs.empty() && !end_jobs.empty();
    }
    
    bool validateDAG() {
        if (!checkNoCycles()) {
            std::cerr << "Error: DAG contains cycles" << std::endl;
            return false;
        }
        
        if (!checkSingleComponent()) {
            std::cerr << "Error: DAG contains multiple connected components" << std::endl;
            return false;
        }
        
        if (!checkStartAndEndJobs()) {
            std::cerr << "Error: DAG does not contain start or end jobs" << std::endl;
            return false;
        }
        
        std::cout << "DAG is valid" << std::endl;
        return true;
    }
    
    bool executeDAG() {
        std::unordered_map<std::string, int> in_degree;
        std::queue<std::string> q;
        
        // Initialize in-degrees
        for (const auto& pair : jobs) {
            in_degree[pair.first] = pair.second->dependencies.size();
            if (in_degree[pair.first] == 0) {
                q.push(pair.first);
            }
        }
        
        while (!q.empty()) {
            std::string current_id = q.front();
            q.pop();
            
            Job* current_job = jobs[current_id];
            bool success = current_job->execute();
            current_job->completed = true;
            
            if (!success) {
                std::cerr << "Job " << current_id << " failed, aborting DAG execution" << std::endl;
                return false;
            }
            
            // Process next jobs
            for (const auto& next_id : current_job->next_jobs) {
                in_degree[next_id]--;
                if (in_degree[next_id] == 0) {
                    q.push(next_id);
                }
            }
        }
        
        // Check if all jobs were executed
        for (const auto& pair : jobs) {
            if (!pair.second->completed) {
                std::cerr << "Not all jobs were executed, possible cycle detected" << std::endl;
                return false;
            }
        }
        
        std::cout << "DAG executed successfully" << std::endl;
        return true;
    }

    ~DAGScheduler() {
        // Clean up memory
        for (auto& pair : jobs) {
            delete pair.second;
        }
    }

private:
    std::unordered_map<std::string, Job*> jobs;
    std::vector<std::string> start_jobs;   // IDs of start jobs
    std::vector<std::string> end_jobs;     // IDs of end jobs

    // Helper function for cycle detection
    bool hasCycleDFS(const std::string& id, std::unordered_map<std::string, int>& visited) {
        visited[id] = 1; // In progress
        
        for (const auto& next_id : jobs[id]->next_jobs) {
            if (visited[next_id] == 1) {
                return true; // Cycle detected
            }
            if (visited[next_id] == 0 && hasCycleDFS(next_id, visited)) {
                return true;
            }
        }
        
        visited[id] = 2; // Processed
        return false;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_json_file>" << std::endl;
        return 1;
    }
    
    std::string json_file = argv[1];
    DAGScheduler scheduler;
    
    if (!scheduler.loadFromJson(json_file)) {
        std::cerr << "Error loading DAG configuration" << std::endl;
        return 1;
    }
    
    if (!scheduler.validateDAG()) {
        std::cerr << "DAG is invalid" << std::endl;
        return 1;
    }
    
    if (!scheduler.executeDAG()) {
        std::cerr << "DAG execution aborted due to an error" << std::endl;
        return 1;
    }
    
    return 0;
}
