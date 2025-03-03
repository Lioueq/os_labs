#include "bintree.hpp"

// Конструктор
BinaryTree::BinaryTree() : root(nullptr) {}

// Деструктор
BinaryTree::~BinaryTree()
{
    deleteTreeRecursive(root);
}

// Рекурсивное удаление всего дерева
void BinaryTree::deleteTreeRecursive(Node *current)
{
    if (current)
    {
        deleteTreeRecursive(current->left);
        deleteTreeRecursive(current->right);
        delete current;
    }
}

// Вставка нового узла
void BinaryTree::insert(int id, int port)
{
    root = insertRecursive(root, id, port, 0);
}

// Рекурсивная вставка узла
Node *BinaryTree::insertRecursive(Node *current, int id, int port, int parent_port)
{
    if (current == nullptr)
    {
        Node *newNode = new Node;
        newNode->id = id;
        newNode->port = port;
        newNode->parent_port = parent_port;
        newNode->left = nullptr;
        newNode->right = nullptr;
        newNode->is_alive = true;
        return newNode;
    }

    if (id < current->id)
    {
        current->left = insertRecursive(current->left, id, port, current->port);
    }
    else if (id > current->id)
    {
        current->right = insertRecursive(current->right, id, port, current->port);
    }
    // Если id равен текущему, можно обновить информацию или оставить как есть

    return current;
}

// Поиск узла по id
Node *BinaryTree::find(int id)
{
    return findRecursive(root, id);
}

// Рекурсивный поиск узла
Node *BinaryTree::findRecursive(Node *current, int id)
{
    if (current == nullptr || current->id == id)
    {
        return current;
    }

    if (id < current->id)
    {
        return findRecursive(current->left, id);
    }

    return findRecursive(current->right, id);
}

// Обход дерева в порядке возрастания ключей
void BinaryTree::traverseInOrder()
{
    traverseInOrderRecursive(root);
    std::cout << std::endl;
}

// Рекурсивный обход дерева
void BinaryTree::traverseInOrderRecursive(Node *current)
{
    if (current != nullptr)
    {
        traverseInOrderRecursive(current->left);
        std::cout << "ID: " << current->id << ", Port: " << current->port
                  << ", Status: " << (current->is_alive ? "Alive" : "Dead") << " | ";
        traverseInOrderRecursive(current->right);
    }
}

// Проверка, пустое ли дерево
bool BinaryTree::isEmpty()
{
    return root == nullptr;
}

// Удаление узла по id
void BinaryTree::remove(int id)
{
    root = removeNodeRecursive(root, id);
}

// Рекурсивное удаление узла
Node *BinaryTree::removeNodeRecursive(Node *current, int id)
{
    // Базовый случай: дерево пустое
    if (current == nullptr)
    {
        return nullptr;
    }

    // Рекурсивный поиск узла для удаления
    if (id < current->id)
    {
        current->left = removeNodeRecursive(current->left, id);
    }
    else if (id > current->id)
    {
        current->right = removeNodeRecursive(current->right, id);
    }
    else
    {
        // Узел с одним ребёнком или без детей
        if (current->left == nullptr)
        {
            Node *temp = current->right;
            delete current;
            return temp;
        }
        else if (current->right == nullptr)
        {
            Node *temp = current->left;
            delete current;
            return temp;
        }

        // Узел с двумя детьми: получаем наименьший узел в правом поддереве
        Node *temp = findMinValueNode(current->right);

        // Копируем данные преемника в текущий узел
        current->id = temp->id;
        current->port = temp->port;
        current->is_alive = temp->is_alive;

        // Удаляем преемника
        current->right = removeNodeRecursive(current->right, temp->id);
    }

    return current;
}

// Поиск узла с минимальным значением
Node *BinaryTree::findMinValueNode(Node *node)
{
    Node *current = node;

    // Спускаемся влево до конца
    while (current && current->left != nullptr)
    {
        current = current->left;
    }

    return current;
}

// Установка статуса узла
void BinaryTree::setNodeStatus(int id, bool status)
{
    Node *node = find(id);
    if (node)
    {
        node->is_alive = status;
    }
}

// Получение статуса узла
bool BinaryTree::getNodeStatus(int id)
{
    Node *node = find(id);
    if (node)
    {
        return node->is_alive;
    }
    return false; // Узел не найден
}

Node* BinaryTree::findRoot() {
    return root;
}