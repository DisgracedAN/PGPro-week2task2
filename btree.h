#ifndef BTREE_H
#define BTREE_H
#include <stdbool.h>

typedef struct BTreeNode {
	int* keys;//массив ключей
	struct BTreeNode** children;//массив дочерних указателей
	int n;//текущее количество ключей в дереве
	bool leaf;//является ли листом
}BTreeNode;

typedef struct  {
	int t;//минимальная степень (t >= 2)
	BTreeNode* root;//корень дерева 
}BTree;

//создаёт пустое б-дерево с заданной минимальной степенью
//возвращает указатель на дерево или NULL при ошибке
BTree* btree_create(int t);

//Поиск ключа в поддереве с корнем node
//возвращает указатель на узел к ключом или NULL при ошибке/отсутствии ключа
BTreeNode* btree_search(BTreeNode* node, int key);

//вставка ключа в б-дерево
//если ключ уже есть-ничего не делает, т.к. дупликаты не допускаются
void btree_insert(BTree* tree, int key);


//удаление ключа из б-дерева
//если ключ не найден-ничего не делает
void btree_delete(BTree* tree, int key);


//Выводит структуру дерева в консоль (по уровням с отступами)
void btree_print(BTree* tree, int level);

//Рекурсивно освобождает память, занятую узлом и его потомками
void btree_free_node(BTreeNode* node);

//Уничтожает дерево и освобождает всю память
void btree_destroy(BTree* tree);

#endif