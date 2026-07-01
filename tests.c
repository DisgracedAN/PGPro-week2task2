#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "btree.h"

// Проверка свойств B-дерева
static bool check_properties(BTreeNode* node, int t, int* leaf_depth, int current_depth) {
    if (!node) return true;
    // Количество ключей в корне и внутренних узлах
    if (node->n < 1 || node->n > 2 * t - 1) return false;
    if (!node->leaf) {
        // Все ключи должны быть в порядке возрастания
        for (int i = 0; i < node->n; i++) {
            if (i > 0 && node->keys[i] <= node->keys[i - 1]) return false;
        }
        // Все потомки должны быть непусты и иметь правильное количество ключей
        for (int i = 0; i <= node->n; i++) {
            if (!node->children[i]) return false;
            if (node->children[i]->n < t - 1) return false;
        }
        // Ключи разделяют поддеревья
        for (int i = 0; i < node->n; i++) {
            // Все ключи в левом поддереве < node->keys[i]
            BTreeNode* left = node->children[i];
            if (left->keys[left->n - 1] >= node->keys[i]) return false;
            // Все ключи в правом поддереве > node->keys[i]
            BTreeNode* right = node->children[i + 1];
            if (right->keys[0] <= node->keys[i]) return false;
        }
        // Проверяем глубину листьев
        for (int i = 0; i <= node->n; i++) {
            if (!check_properties(node->children[i], t, leaf_depth, current_depth + 1))
                return false;
        }
    }
    else {
        // Лист: все ключи упорядочены
        for (int i = 1; i < node->n; i++) {
            if (node->keys[i] <= node->keys[i - 1]) return false;
        }
        if (*leaf_depth == -1) {
            *leaf_depth = current_depth;
        }
        else if (*leaf_depth != current_depth) {
            return false; // все листья на одной глубине
        }
    }
    return true;
}

static bool verify_btree(BTree* tree) {
    if (!tree->root) return true;
    int leaf_depth = -1;
    return check_properties(tree->root, tree->t, &leaf_depth, 0);
}

//Вспомогательная печать
static void print_keys_inorder(BTreeNode* node) {
    if (!node) return;
    for (int i = 0; i < node->n; i++) {
        if (!node->leaf) print_keys_inorder(node->children[i]);
        printf("%d ", node->keys[i]);
    }
    if (!node->leaf) print_keys_inorder(node->children[node->n]);
}


//Тест с большим количеством ключей (1000)
static void run_large_test(void) {
    printf("Тест с большим количеством ключей\n");
    BTree* tree = btree_create(3); // t=3 (диапазон ключей в узле 2..5)
    const int N = 1000;
    int* keys = malloc(N * sizeof(int));
    // Заполняем числами 1..N, затем перемешиваем для случайного порядка вставки
    for (int i = 0; i < N; i++) keys[i] = i + 1;
    srand(42);
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = keys[i];
        keys[i] = keys[j];
        keys[j] = tmp;
    }

    // Вставка
    for (int i = 0; i < N; i++) {
        btree_insert(tree, keys[i]);
        assert(verify_btree(tree));   // проверяем свойства после каждой вставки
    }
    printf("  Вставка %d ключей: OK\n", N);

    // Поиск всех вставленных ключей
    for (int i = 0; i < N; i++) {
        assert(btree_search(tree->root, keys[i]) != NULL);
    }
    printf("  Поиск всех ключей: OK\n");

    // Перемешиваем для случайного порядка удаления
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = keys[i];
        keys[i] = keys[j];
        keys[j] = tmp;
    }

    // Удаление
    for (int i = 0; i < N; i++) {
        btree_delete(tree, keys[i]);
        assert(verify_btree(tree));
    }
    assert(tree->root == NULL);
    printf("  Удаление всех ключей: OK\n");

    free(keys);
    btree_destroy(tree);
    printf("  Большой тест пройден.\n\n");
}
// Автоматические тесты
void run_automatic_tests(void) {
    printf("Автоматические тесты B-дерева\n");

    // Создание с недопустимым t
    assert(btree_create(1) == NULL);

    BTree* tree = btree_create(2); // t=2 (2-3-4 дерево)
    assert(tree != NULL);

    // Вставка
    int keys[] = { 10, 20, 5, 6, 12, 30, 7, 17 };
    for (int i = 0; i < 8; i++) {
        btree_insert(tree, keys[i]);
        assert(verify_btree(tree));
    }

    // Поиск
    assert(btree_search(tree->root, 6) != NULL);
    assert(btree_search(tree->root, 30) != NULL);
    assert(btree_search(tree->root, 99) == NULL);

    // Удаление из листа (6)
    btree_delete(tree, 6);
    assert(verify_btree(tree));
    assert(btree_search(tree->root, 6) == NULL);

    // Удаление внутреннего ключа (10)
    btree_delete(tree, 10);
    assert(verify_btree(tree));
    assert(btree_search(tree->root, 10) == NULL);

    // Удаление несуществующего ключа
    btree_delete(tree, 99);
    assert(verify_btree(tree));

    // Удаление всех ключей
    int rem[] = { 20, 5, 12, 30, 7, 17 };
    for (int i = 0; i < 6; i++) {
        btree_delete(tree, rem[i]);
        assert(verify_btree(tree));
    }
    assert(tree->root == NULL);

    // Вставка после полного удаления
    btree_insert(tree, 42);
    assert(btree_search(tree->root, 42) != NULL);

    
    btree_destroy(tree);
    printf("  [OK] Все тесты пройдены.\n\n");

    run_large_test();

}

//Демонстрация
void run_demo(void) {
    printf("Демонстрация B-дерева со случайными ключами\n\n");

    // t=2 
    printf("Пример с t=2\n");
    BTree* tree = btree_create(2);
    const int N1 = 15;                     // количество вставляемых ключей
    int keys1[15];
    srand(12345);                          // фиксируем seed для воспроизводимости

    // Генерируем уникальные случайные числа 1..100
    for (int i = 0; i < N1; i++) {
        int candidate;
        int is_unique;
        do {
            is_unique = 1;
            candidate = rand() % 100 + 1;
            for (int j = 0; j < i; j++)
                if (keys1[j] == candidate) { is_unique = 0; break; }
        } while (!is_unique);
        keys1[i] = candidate;
    }

    printf("Случайные ключи для вставки: ");
    for (int i = 0; i < N1; i++) printf("%d ", keys1[i]);
    printf("\n\n");

    // Вставка
    for (int i = 0; i < N1; i++) {
        btree_insert(tree, keys1[i]);
        printf("После вставки %d:\n", keys1[i]);
        btree_print(tree->root, 1);
        printf("-------------------\n");
    }

    // Удаляем несколько ключей (первые 5 из сгенерированного списка)
    printf("\nУдаляем первые 5 ключей: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", keys1[i]);
    }
    printf("\n");
    for (int i = 0; i < 5; i++) {
        btree_delete(tree, keys1[i]);
        printf("После удаления %d:\n", keys1[i]);
        btree_print(tree->root, 1);
        printf("-------------------\n");
    }

    // Вставляем ещё 5 новых случайных чисел
    printf("\nДобавляем ещё 5 новых ключей:\n");
    srand(54321);
    for (int i = 0; i < 5; i++) {
        int new_key = rand() % 100 + 1;
        printf("Вставка %d\n", new_key);
        btree_insert(tree, new_key);
        btree_print(tree->root, 1);
        printf("-------------------\n");
    }

    printf("Итоговое дерево t=2 (обход inorder): ");
    print_keys_inorder(tree->root);
    printf("\n\n");

    btree_destroy(tree);

    // t=3 (более широкие узлы)
    printf(" Пример с t=3 (больше ключей) \n");
    tree = btree_create(3);
    const int N2 = 30;
    int keys2[30];
    srand(9999);
    for (int i = 0; i < N2; i++) {
        int candidate;
        int is_unique;
        do {
            is_unique = 1;
            candidate = rand() % 200 + 1;
            for (int j = 0; j < i; j++)
                if (keys2[j] == candidate) { is_unique = 0; break; }
        } while (!is_unique);
        keys2[i] = candidate;
    }

    printf("Вставляем 30 случайных неповторяющихся чисел (1..200): ");
    for (int i = 0; i < N2; i++) {
        btree_insert(tree, keys2[i]);
    }
    printf("готово.\n");
    printf("Структура дерева:\n");
    btree_print(tree->root, 1);
    printf("\n");

    // Удалим 10 случайных ключей
    printf("Удаляем 10 случайных ключей из вставленных: ");
    for (int i = 0; i < 10; i++) {
        int idx = rand() % N2;
        printf("%d ", keys2[idx]);
        btree_delete(tree, keys2[idx]);
    }
    printf("\nСтруктура после удалений:\n");
    btree_print(tree->root, 1);
    printf("\n");

    printf("Итоговый обход inorder: ");
    print_keys_inorder(tree->root);
    printf("\n\n");

    btree_destroy(tree);
}