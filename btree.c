#include "btree.h"
#include <stdlib.h>
#include <stdio.h>

//Создание и освобождение
BTree* btree_create(int t) {
	if (t < 2) return NULL;
	BTree* tree = (BTree*)malloc(sizeof(BTree));
	if (!tree) return NULL;
	tree->t = t;
	tree->root = NULL;
	return tree;

}
//создание узла

static BTreeNode* btree_new_node(int t, bool leaf) {
	BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
	if (!node) return NULL;
	node->keys = (int*)malloc(sizeof(int) * (2 * t - 1));
	node->children = (BTreeNode**)malloc(sizeof(BTreeNode*) * (2 * t));
	if (!node->keys || !node->children) {
		free(node->keys);
		free(node->children);
		free(node);
		return NULL;
	}
	node->n = 0;
	node->leaf = leaf;
	return node;
}

void btree_free_node(BTreeNode* node) {
	if (!node) return;
	if (!node->leaf) {
		for (int i = 0; i <= node->n; i++) {
			btree_free_node(node->children[i]);
		}
	}
	free(node->keys);
	free(node->children);
	free(node);
}

void btree_destroy(BTree* tree) {
	if (!tree) return;
	btree_free_node(tree->root);
	free(tree);
}

//Поиск

BTreeNode* btree_search(BTreeNode* node, int key) {
	if (!node) return NULL;
	int i = 0;
	//Ищем первый ключ, больший или равный key
	while (i<node->n && key>node->keys[i])i++;
	if (i < node->n && key == node->keys[i]) return node;
	if (node->leaf) return NULL;
	return btree_search(node->children[i], key);
}

//Вставка
//Разделение полного дочернего узла y (child[i] узла x)
static void btree_split_child(BTree* tree, BTreeNode* x, int i) {
	int t = tree->t;
	BTreeNode* y = x->children[i];
	BTreeNode* z = btree_new_node(t, y->leaf);
	z->n = t - 1;
	//Копируем вторую половину ключей y в z
	for (int j = 0;j < t - 1;j++)
		z->keys[j] = y->keys[j + t];
	//Если y не лист, копируем соответствующих потомков
	if (!y->leaf) {
		for (int j = 0;j < t;j++)
			z->children[j] = y->children[j + t];
	}
	y->n = t - 1;
	//Освобождаем место для нового дочернего указателя в x
	for (int j = x->n; j >= i + 1; j--)
		x->children[j + 1] = x->children[j];
	x->children[i + 1] = z;
	//Перемещаем ключи в x, чтобы вставить медианный ключ из y
	for (int j = x->n - 1; j >= i; j--)
		x->keys[j + 1] = x->keys[j];
	x->keys[i] = y->keys[t - 1];
	x->n++;
}

//Вставка в незаполненный узел (n < 2t-1)
static void btree_insert_nonfull(BTree* tree, BTreeNode* x, int key) {
	int t = tree->t;
	int i = x->n - 1;

	if (x->leaf) {
		//Вставка в лист 
		while (i >= 0 && key < x->keys[i]) {
			x->keys[i + 1] = x->keys[i];
			i--;
		}
		x->keys[i + 1] = key;
		x->n++;
	}
	else {
		//Внутренний узел: ищем подходящего потомка 
		while (i >= 0 && key < x->keys[i]) i--;
		i++;
		//Если потомок полон, разделяем его
		if (x->children[i]->n == 2 * t - 1) {
			btree_split_child(tree, x, i);
			if (key > x->keys[i]) i++;
		}
		btree_insert_nonfull(tree, x->children[i], key);
	}
}

void btree_insert(BTree* tree, int key) {
	if (!tree) return;
	BTreeNode* root = tree->root;
	if (!root) {
		root = btree_new_node(tree->t, true);
		root->keys[0] = key;
		root->n = 1;
		tree->root = root;
		return;
	}
	//Если корень полон, создаём новый корень и разделяем старый 
	if (root->n == 2 * tree->t - 1) {
		BTreeNode* new_root = btree_new_node(tree->t, false);
		new_root->children[0] = root;
		btree_split_child(tree, new_root, 0);
		//Вставляем в подходящего потомка
		int i = 0;
		if (key > new_root->keys[0]) i++;
		btree_insert_nonfull(tree, new_root->children[i], key);
		tree->root = new_root;
	}
	else {
		btree_insert_nonfull(tree, root, key);
	}
}
//Удаление
//Получить индекс первого ключа, >= key 
static int find_key(BTreeNode* node, int key) {
    int idx = 0;
    while (idx < node->n && node->keys[idx] < key) idx++;
    return idx;
}

//Удаление ключа из листа
static void remove_from_leaf(BTreeNode* node, int idx) {
    for (int i = idx + 1; i < node->n; i++)
        node->keys[i - 1] = node->keys[i];
    node->n--;
}

//Получить предшественника (максимальный ключ в поддереве)
static int get_predecessor(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx];
    while (!cur->leaf)
        cur = cur->children[cur->n];
    return cur->keys[cur->n - 1];
}

//Получить преемника (минимальный ключ в поддереве)
static int get_successor(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx + 1];
    while (!cur->leaf)
        cur = cur->children[0];
    return cur->keys[0];
}

//Заполнить потомка node->children[idx], в котором менее t-1 ключей
static void fill(BTree* tree, BTreeNode* node, int idx) {
    int t = tree->t;
    if (idx != 0 && node->children[idx - 1]->n >= t) {
        //Заимствование у левого брата 
        BTreeNode* child = node->children[idx];
        BTreeNode* left = node->children[idx - 1];
        //Сдвиг ключей в child вправо
        for (int i = child->n - 1; i >= 0; i--)
            child->keys[i + 1] = child->keys[i];
        if (!child->leaf) {
            for (int i = child->n; i >= 0; i--)
                child->children[i + 1] = child->children[i];
        }
        child->keys[0] = node->keys[idx - 1];
        if (!child->leaf)
            child->children[0] = left->children[left->n];
        node->keys[idx - 1] = left->keys[left->n - 1];
        child->n++;
        left->n--;
    }
    else if (idx != node->n && node->children[idx + 1]->n >= t) {
        //Заимствование у правого брата
        BTreeNode* child = node->children[idx];
        BTreeNode* right = node->children[idx + 1];
        child->keys[child->n] = node->keys[idx];
        if (!child->leaf)
            child->children[child->n + 1] = right->children[0];
        node->keys[idx] = right->keys[0];
        for (int i = 1; i < right->n; i++)
            right->keys[i - 1] = right->keys[i];
        if (!right->leaf) {
            for (int i = 1; i <= right->n; i++)
                right->children[i - 1] = right->children[i];
        }
        child->n++;
        right->n--;
    }
    else {
        //Слияние с братом 
        if (idx != node->n) {
            //Слияние с правым братом 
            BTreeNode* child = node->children[idx];
            BTreeNode* right = node->children[idx + 1];
            child->keys[t - 1] = node->keys[idx];
            for (int i = 0; i < right->n; i++)
                child->keys[t + i] = right->keys[i];
            if (!child->leaf) {
                for (int i = 0; i <= right->n; i++)
                    child->children[t + i] = right->children[i];
            }
            child->n = 2 * t - 1;
            //Удаляем right и ключ из node 
            for (int i = idx + 1; i < node->n; i++)
                node->keys[i - 1] = node->keys[i];
            for (int i = idx + 2; i <= node->n; i++)
                node->children[i - 1] = node->children[i];
            node->n--;
            free(right->keys);
            free(right->children);
            free(right);
        }
        else {
            //Слияние с левым братом (idx == node->n) 
            BTreeNode* child = node->children[idx];
            BTreeNode* left = node->children[idx - 1];
            left->keys[t - 1] = node->keys[idx - 1];
            for (int i = 0; i < child->n; i++)
                left->keys[t + i] = child->keys[i];
            if (!left->leaf) {
                for (int i = 0; i <= child->n; i++)
                    left->children[t + i] = child->children[i];
            }
            left->n = 2 * t - 1;
            node->n--;
            free(child->keys);
            free(child->children);
            free(child);
        }
    }
}

//Рекурсивное удаление ключа из поддерева
static void btree_delete_key(BTree* tree, BTreeNode* node, int key) {
    int t = tree->t;
    int idx = find_key(node, key);

    if (idx < node->n && node->keys[idx] == key) {
        //Ключ найден в этом узле
        if (node->leaf) {
            remove_from_leaf(node, idx);
        }
        else {
            //Внутренний узел
            if (node->children[idx]->n >= t) {
                int pred = get_predecessor(node, idx);
                node->keys[idx] = pred;
                btree_delete_key(tree, node->children[idx], pred);
            }
            else if (node->children[idx + 1]->n >= t) {
                int succ = get_successor(node, idx);
                node->keys[idx] = succ;
                btree_delete_key(tree, node->children[idx + 1], succ);
            }
            else {
                //Оба потомка имеют по t-1 ключей – сливаем
                BTreeNode* child = node->children[idx];
                BTreeNode* right = node->children[idx + 1];
                child->keys[t - 1] = key;
                for (int i = 0; i < right->n; i++)
                    child->keys[t + i] = right->keys[i];
                if (!child->leaf) {
                    for (int i = 0; i <= right->n; i++)
                        child->children[t + i] = right->children[i];
                }
                child->n = 2 * t - 1;
                //Удаляем right и ключ из node
                for (int i = idx + 1; i < node->n; i++)
                    node->keys[i - 1] = node->keys[i];
                for (int i = idx + 2; i <= node->n; i++)
                    node->children[i - 1] = node->children[i];
                node->n--;
                free(right->keys);
                free(right->children);
                free(right);
                btree_delete_key(tree, child, key);
            }
        }
    }
    else {
        //Ключ не в этом узле, идём к потомку
        if (node->leaf) return; // ключ отсутствует
        bool is_last = (idx == node->n);
        //Если потомок, куда должен попасть ключ, имеет t-1 ключей, заполним его
        if (node->children[idx]->n == t - 1)
            fill(tree, node, idx);
        //Если последний потомок был слит с предыдущим, idx уменьшается
        if (is_last && idx > node->n)
            btree_delete_key(tree, node->children[idx - 1], key);
        else
            btree_delete_key(tree, node->children[idx], key);
    }
}

void btree_delete(BTree* tree, int key) {
    if (!tree || !tree->root) return;
    btree_delete_key(tree, tree->root, key);
    //Если корень стал пустым, заменяем его первым потомком
    if (tree->root->n == 0) {
        BTreeNode* old_root = tree->root;
        if (tree->root->leaf)
            tree->root = NULL;
        else
            tree->root = tree->root->children[0];
        free(old_root->keys);
        free(old_root->children);
        free(old_root);
    }
}

// Вывод дерева
void btree_print(BTreeNode* node, int level) {
    if (!node) return;
    for (int i = 0; i < level; i++) printf("  ");
    printf("[");
    for (int i = 0; i < node->n; i++) {
        printf("%d", node->keys[i]);
        if (i < node->n - 1) printf(" ");
    }
    printf("]\n");
    if (!node->leaf) {
        for (int i = 0; i <= node->n; i++) {
            btree_print(node->children[i], level + 1);
        }
    }
}