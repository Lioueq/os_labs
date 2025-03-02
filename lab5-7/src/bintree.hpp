#ifndef BINTREE_HPP
#define BINTREE_HPP

#include <iostream>
#include <string>

struct Node
{
    int id;
    Node *left;
    Node *right;
    int port;
    bool is_alive;
};

class BinaryTree
{
private:
    Node *root;

    // Вспомогательные рекурсивные методы
    Node *insertRecursive(Node *current, int id, int port);
    Node *findRecursive(Node *current, int id);
    void traverseInOrderRecursive(Node *current);
    void deleteTreeRecursive(Node *current);
    Node *removeNodeRecursive(Node *current, int id);
    Node *findMinValueNode(Node *node);

public:
    BinaryTree();
    ~BinaryTree();

    // Основные методы для работы с деревом
    void insert(int id, int port);
    Node *find(int id);
    void traverseInOrder();
    void remove(int id);
    bool isEmpty();

    // Дополнительные методы для управления статусом узлов
    void setNodeStatus(int id, bool status);
    bool getNodeStatus(int id);
};

#endif // BINTREE_HPP