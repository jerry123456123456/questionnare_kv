/*
nty_tree.h 文件主要功能及 API 接口
主要功能
nty_tree.h 文件实现了伸展树（Splay Tree）的数据结构。伸展树是一种自调整形式的二叉搜索树，它会在每次访问之后对树进行重构，使得被访问的节点移动到树的根节点，从而在后续的访问中能够更快地找到该节点。
API 接口
树的初始化
SPLAY_INITIALIZER(root)：静态初始化伸展树的根节点为 NULL。
SPLAY_INIT(root)：动态初始化伸展树的根节点为 NULL。
树的操作
SPLAY_LEFT(elm, field)：获取节点 elm 的左子节点。
SPLAY_RIGHT(elm, field)：获取节点 elm 的右子节点。
SPLAY_ROOT(head)：获取伸展树的根节点。
SPLAY_EMPTY(head)：判断伸展树是否为空。
SPLAY_ROTATE_RIGHT(head, tmp, field)：对伸展树进行右旋操作。
SPLAY_ROTATE_LEFT(head, tmp, field)：对伸展树进行左旋操作。
SPLAY_LINKLEFT(head, tmp, field)：将当前根节点的左子节点链接到临时节点 tmp 上，并更新根节点。
SPLAY_LINKRIGHT(head, tmp, field)：将当前根节点的右子节点链接到临时节点 tmp 上，并更新根节点。
SPLAY_ASSEMBLE(head, node, left, right, field)：将左右子树重新组装到根节点上。
原型声明
SPLAY_PROTOTYPE(name, type, field, cmp)：声明伸展树的操作函数，包括 SPLAY、SPLAY_MINMAX、SPLAY_INSERT、SPLAY_REMOVE、SPLAY_FIND、SPLAY_NEXT、SPLAY_MIN_MAX 等函数。
函数实现
SPLAY_GENERATE(name, type, field, cmp)：实现伸展树的操作函数，包括插入、删除、伸展等操作。
*/

#ifndef __NTY_TREE_H__
#define __NTY_TREE_H__

#define SPLAY_HEAD(name, type)                        \
    struct name                                       \
    {                                                 \
        struct type *sph_root; /* root of the tree */ \
    }

#define SPLAY_INITIALIZER(root) \
    {NULL}

#define SPLAY_INIT(root)         \
    do                           \
    {                            \
        (root)->sph_root = NULL; \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ENTRY(type)                           \
    struct                                          \
    {                                               \
        struct type *spe_left;  /* left element */  \
        struct type *spe_right; /* right element */ \
    }

#define SPLAY_LEFT(elm, field) (elm)->field.spe_left
#define SPLAY_RIGHT(elm, field) (elm)->field.spe_right
#define SPLAY_ROOT(head) (head)->sph_root
#define SPLAY_EMPTY(head) (SPLAY_ROOT(head) == NULL)

/* SPLAY_ROTATE_{LEFT,RIGHT} expect that tmp hold SPLAY_{RIGHT,LEFT} */
#define SPLAY_ROTATE_RIGHT(head, tmp, field)                           \
    do                                                                 \
    {                                                                  \
        SPLAY_LEFT((head)->sph_root, field) = SPLAY_RIGHT(tmp, field); \
        SPLAY_RIGHT(tmp, field) = (head)->sph_root;                    \
        (head)->sph_root = tmp;                                        \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ROTATE_LEFT(head, tmp, field)                            \
    do                                                                 \
    {                                                                  \
        SPLAY_RIGHT((head)->sph_root, field) = SPLAY_LEFT(tmp, field); \
        SPLAY_LEFT(tmp, field) = (head)->sph_root;                     \
        (head)->sph_root = tmp;                                        \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_LINKLEFT(head, tmp, field)                        \
    do                                                          \
    {                                                           \
        SPLAY_LEFT(tmp, field) = (head)->sph_root;              \
        tmp = (head)->sph_root;                                 \
        (head)->sph_root = SPLAY_LEFT((head)->sph_root, field); \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_LINKRIGHT(head, tmp, field)                        \
    do                                                           \
    {                                                            \
        SPLAY_RIGHT(tmp, field) = (head)->sph_root;              \
        tmp = (head)->sph_root;                                  \
        (head)->sph_root = SPLAY_RIGHT((head)->sph_root, field); \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_ASSEMBLE(head, node, left, right, field)                   \
    do                                                                   \
    {                                                                    \
        SPLAY_RIGHT(left, field) = SPLAY_LEFT((head)->sph_root, field);  \
        SPLAY_LEFT(right, field) = SPLAY_RIGHT((head)->sph_root, field); \
        SPLAY_LEFT((head)->sph_root, field) = SPLAY_RIGHT(node, field);  \
        SPLAY_RIGHT((head)->sph_root, field) = SPLAY_LEFT(node, field);  \
    } while (/*CONSTCOND*/ 0)

#define SPLAY_PROTOTYPE(name, type, field, cmp)                     \
    void name##_SPLAY(struct name *, struct type *);                \
    void name##_SPLAY_MINMAX(struct name *, int);                   \
    struct type *name##_SPLAY_INSERT(struct name *, struct type *); \
    struct type *name##_SPLAY_REMOVE(struct name *, struct type *); \
                                                                    \
    /* Finds the node with the same key as elm */                   \
    static __inline struct type *                                   \
        name##_SPLAY_FIND(struct name *head, struct type *elm)      \
    {                                                               \
        \ if (SPLAY_EMPTY(head)) return (NULL);                     \
        name##_SPLAY(head, elm);                                    \
        if ((cmp)(elm, (head)->sph_root) == 0)                      \
            return (head->sph_root);                                \
        return (NULL);                                              \
    }                                                               \
                                                                    \
    static __inline struct type *                                   \
        name##_SPLAY_NEXT(struct name *head, struct type *elm)      \
    {                                                               \
        name##_SPLAY(head, elm);                                    \
        if (SPLAY_RIGHT(elm, field) != NULL)                        \
        {                                                           \
            elm = SPLAY_RIGHT(elm, field);                          \
            while (SPLAY_LEFT(elm, field) != NULL)                  \
            {                                                       \
                elm = SPLAY_LEFT(elm, field);                       \
            }                                                       \
        }                                                           \
        else                                                        \
            elm = NULL;                                             \
        return (elm);                                               \
    }                                                               \
                                                                    \
    static __inline struct type *                                   \
        name##_SPLAY_MIN_MAX(struct name *head, int val)            \
    {                                                               \
        name##_SPLAY_MINMAX(head, val);                             \
        return (SPLAY_ROOT(head));                                  \
    }

/* Main splay operation.
 * Moves node close to the key of elm to top
 */
#define SPLAY_GENERATE(name, type, field, cmp)                                  \
    struct type *                                                               \
        name##_SPLAY_INSERT(struct name *head, struct type *elm)                \
    {                                                                           \
        if (SPLAY_EMPTY(head))                                                  \
        {                                                                       \
            SPLAY_LEFT(elm, field) = SPLAY_RIGHT(elm, field) = NULL;            \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            int __comp;                                                         \
            name##_SPLAY(head, elm);                                            \
            __comp = (cmp)(elm, (head)->sph_root);                              \
            if (__comp < 0)                                                     \
            {                                                                   \
                SPLAY_LEFT(elm, field) = SPLAY_LEFT((head)->sph_root, field);   \
                SPLAY_RIGHT(elm, field) = (head)->sph_root;                     \
                SPLAY_LEFT((head)->sph_root, field) = NULL;                     \
            }                                                                   \
            else if (__comp > 0)                                                \
            {                                                                   \
                SPLAY_RIGHT(elm, field) = SPLAY_RIGHT((head)->sph_root, field); \
                SPLAY_LEFT(elm, field) = (head)->sph_root;                      \
                SPLAY_RIGHT((head)->sph_root, field) = NULL;                    \
            }                                                                   \
            else                                                                \
                return ((head)->sph_root);                                      \
        }                                                                       \
        (head)->sph_root = (elm);                                               \
        return (NULL);                                                          \
    }                                                                           \
                                                                                \
    struct type *                                                               \
        name##_SPLAY_REMOVE(struct name *head, struct type *elm)                \
    {                                                                           \
        struct type *__tmp;                                                     \
        if (SPLAY_EMPTY(head))                                                  \
            return (NULL);                                                      \
        name##_SPLAY(head, elm);                                                \
        if ((cmp)(elm, (head)->sph_root) == 0)                                  \
        {                                                                       \
            if (SPLAY_LEFT((head)->sph_root, field) == NULL)                    \
            {                                                                   \
                (head)->sph_root = SPLAY_RIGHT((head)->sph_root, field);        \
            }                                                                   \
            else                                                                \
            {                                                                   \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                (head)->sph_root = SPLAY_LEFT((head)->sph_root, field);         \
                name##_SPLAY(head, elm);                                        \
                SPLAY_RIGHT((head)->sph_root, field) = __tmp;                   \
            }                                                                   \
            return (elm);                                                       \
        }                                                                       \
        return (NULL);                                                          \
    }                                                                           \
                                                                                \
    void                                                                        \
        name##_SPLAY(struct name *head, struct type *elm)                       \
    {                                                                           \
        struct type __node, *__left, *__right, *__tmp;                          \
        int __comp;                                                             \
                                                                                \
        SPLAY_LEFT(&__node, field) = SPLAY_RIGHT(&__node, field) = NULL;        \
        __left = __right = &__node;                                             \
                                                                                \
        while ((__comp = (cmp)(elm, (head)->sph_root)) != 0)                    \
        {                                                                       \
            if (__comp < 0)                                                     \
            {                                                                   \
                __tmp = SPLAY_LEFT((head)->sph_root, field);                    \
                if (__tmp == NULL)                                              \
                    break;                                                      \
                if ((cmp)(elm, __tmp) < 0)                                      \
                {                                                               \
                    SPLAY_ROTATE_RIGHT(head, __tmp, field);                     \
                    if (SPLAY_LEFT((head)->sph_root, field) == NULL)            \
                        break;                                                  \
                }                                                               \
                SPLAY_LINKLEFT(head, __right, field);                           \
            }                                                                   \
            else if (__comp > 0)                                                \
            {                                                                   \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                if (__tmp == NULL)                                              \
                    break;                                                      \
                if ((cmp)(elm, __tmp) > 0)                                      \
                {                                                               \
                    SPLAY_ROTATE_LEFT(head, __tmp, field);                      \
                    if (SPLAY_RIGHT((head)->sph_root, field) == NULL)           \
                        break;                                                  \
                }                                                               \
                SPLAY_LINKRIGHT(head, __left, field);                           \
            }                                                                   \
        }                                                                       \
        SPLAY_ASSEMBLE(head, &__node, __left, __right, field);                  \
    }                                                                           \
                                                                                \
    void name##_SPLAY_MINMAX(struct name *head, int __comp)                     \
    {                                                                           \
        struct type __node, *__left, *__right, *__tmp;                          \
                                                                                \
        SPLAY_LEFT(&__node, field) = SPLAY_RIGHT(&__node, field) = NULL;        \
        __left = __right = &__node;                                             \
                                                                                \
        while (1)                                                               \
        {                                                                       \
            if (__comp < 0)                                                     \
            {                                                                   \
                __tmp = SPLAY_LEFT((head)->sph_root, field);                    \
                if (__tmp == NULL)                                              \
                    break;                                                      \
                if (__comp < 0)                                                 \
                {                                                               \
                    SPLAY_ROTATE_RIGHT(head, __tmp, field);                     \
                    if (SPLAY_LEFT((head)->sph_root, field) == NULL)            \
                        break;                                                  \
                }                                                               \
                SPLAY_LINKLEFT(head, __right, field);                           \
            }                                                                   \
            else if (__comp > 0)                                                \
            {                                                                   \
                __tmp = SPLAY_RIGHT((head)->sph_root, field);                   \
                if (__tmp == NULL)                                              \
                    break;                                                      \
                if (__comp > 0)                                                 \
                {                                                               \
                    SPLAY_ROTATE_LEFT(head, __tmp, field);                      \
                    if (SPLAY_RIGHT((head)->sph_root, field) == NULL)           \
                        break;                                                  \
                }                                                               \
                SPLAY_LINKRIGHT(head, __left, field);                           \
            }                                                                   \
        }                                                                       \
        SPLAY_ASSEMBLE(head, &__node, __left, __right, field);                  \
    }

#define SPLAY_NEGINF -1
#define SPLAY_INF 1

#define SPLAY_INSERT(name, x, y) name##_SPLAY_INSERT(x, y)
#define SPLAY_REMOVE(name, x, y) name##_SPLAY_REMOVE(x, y)
#define SPLAY_FIND(name, x, y) name##_SPLAY_FIND(x, y)
#define SPLAY_NEXT(name, x, y) name##_SPLAY_NEXT(x, y)
#define SPLAY_MIN(name, x) (SPLAY_EMPTY(x) ? NULL \
                                           : name##_SPLAY_MIN_MAX(x, SPLAY_NEGINF))
#define SPLAY_MAX(name, x) (SPLAY_EMPTY(x) ? NULL \
                                           : name##_SPLAY_MIN_MAX(x, SPLAY_INF))

#define SPLAY_FOREACH(x, name, head)  \
    for ((x) = SPLAY_MIN(name, head); \
         (x) != NULL;                 \
         (x) = SPLAY_NEXT(name, head, x))

/* Macros that define a red-black tree */
#define RB_HEAD(name, type)                           \
    struct name                                       \
    {                                                 \
        struct type *rbh_root; /* root of the tree */ \
    }

#define RB_INITIALIZER(root) \
    {NULL}

#define RB_INIT(root)            \
    do                           \
    {                            \
        (root)->rbh_root = NULL; \
    } while (0)

#define RB_BLACK 0
#define RB_RED 1

#define RB_ENTRY(type)                                \
    struct                                            \
    {                                                 \
        struct type *rbe_left;   /* left element */   \
        struct type *rbe_right;  /* right element */  \
        struct type *rbe_parent; /* parent element */ \
        int rbe_color;           /* node color */     \
    }

#define RB_LEFT(elm, field) (elm)->field.rbe_left
#define RB_RIGHT(elm, field) (elm)->field.rbe_right
#define RB_PARENT(elm, field) (elm)->field.rbe_parent
#define RB_COLOR(elm, field) (elm)->field.rbe_color
#define RB_ROOT(head) (head)->rbh_root
#define RB_EMPTY(head) (RB_ROOT(head) == NULL)

#define RB_SET(elm, parent, field)                         \
    do                                                     \
    {                                                      \
        RB_PARENT(elm, field) = parent;                    \
        RB_LEFT(elm, field) = RB_RIGHT(elm, field) = NULL; \
        RB_COLOR(elm, field) = RB_RED;                     \
    } while (0)

#define RB_SET_BLACKRED(black, red, field) \
    do                                     \
    {                                      \
        RB_COLOR(black, field) = RB_BLACK; \
        RB_COLOR(red, field) = RB_RED;     \
    } while (0)

#ifndef RB_AUGMENT
#define RB_AUGMENT(x) \
    do                \
    {                 \
    } while (0)
#endif

#define RB_ROTATE_LEFT(head, elm, tmp, field)                        \
    do                                                               \
    {                                                                \
        (tmp) = RB_RIGHT(elm, field);                                \
        if ((RB_RIGHT(elm, field) = RB_LEFT(tmp, field)) != NULL)    \
        {                                                            \
            RB_PARENT(RB_LEFT(tmp, field), field) = (elm);           \
        }                                                            \
        RB_AUGMENT(elm);                                             \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) \
        {                                                            \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))      \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);       \
            else                                                     \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);      \
        }                                                            \
        else                                                         \
            (head)->rbh_root = (tmp);                                \
        RB_LEFT(tmp, field) = (elm);                                 \
        RB_PARENT(elm, field) = (tmp);                               \
        RB_AUGMENT(tmp);                                             \
        if ((RB_PARENT(tmp, field)))                                 \
            RB_AUGMENT(RB_PARENT(tmp, field));                       \
    } while (0)

#define RB_ROTATE_RIGHT(head, elm, tmp, field)                       \
    do                                                               \
    {                                                                \
        (tmp) = RB_LEFT(elm, field);                                 \
        if ((RB_LEFT(elm, field) = RB_RIGHT(tmp, field)) != NULL)    \
        {                                                            \
            RB_PARENT(RB_RIGHT(tmp, field), field) = (elm);          \
        }                                                            \
        RB_AUGMENT(elm);                                             \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) \
        {                                                            \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))      \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);       \
            else                                                     \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);      \
        }                                                            \
        else                                                         \
            (head)->rbh_root = (tmp);                                \
        RB_RIGHT(tmp, field) = (elm);                                \
        RB_PARENT(elm, field) = (tmp);                               \
        RB_AUGMENT(tmp);                                             \
        if ((RB_PARENT(tmp, field)))                                 \
            RB_AUGMENT(RB_PARENT(tmp, field));                       \
    } while (0)

#define RB_PROTOTYPE(name, type, field, cmp) \
    RB_PROTOTYPE_INTERNAL(name, type, field, cmp, )
#define RB_PROTOTYPE_STATIC(name, type, field, cmp) \
    RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __unused static)
#define RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)                        \
    attr void name##_RB_INSERT_COLOR(struct name *, struct type *);                \
    attr void name##_RB_REMOVE_COLOR(struct name *, struct type *, struct type *); \
    attr struct type *name##_RB_REMOVE(struct name *, struct type *);              \
    attr struct type *name##_RB_INSERT(struct name *, struct type *);              \
    attr struct type *name##_RB_FIND(struct name *, struct type *);                \
    attr struct type *name##_RB_NFIND(struct name *, struct type *);               \
    attr struct type *name##_RB_NEXT(struct type *);                               \
    attr struct type *name##_RB_PREV(struct type *);                               \
    attr struct type *name##_RB_MINMAX(struct name *, int);

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
/*
这个宏会生成一系列红黑树操作函数，常见的有：
RB_INIT：初始化红黑树。
RB_INSERT：向红黑树中插入一个节点。
RB_REMOVE：从红黑树中移除一个节点。
RB_FIND：在红黑树中查找一个节点。
RB_MIN：获取红黑树中最小的节点。
RB_MAX：获取红黑树中最大的节点。
RB_EMPTY：检查红黑树是否为空。

name：红黑树的类型名，用于生成的红黑树操作函数的命名空间。
type：红黑树节点的数据类型，也就是存储在红黑树中的元素类型。
field：红黑树节点中用于连接树结构的字段名，该字段类型通常是红黑树节点结构体。
cmp：用于比较两个节点大小的比较函数，该函数决定了红黑树中节点的排序顺序。
*/
#define RB_GENERATE(name, type, field, cmp) \
    RB_GENERATE_INTERNAL(name, type, field, cmp, )
#define RB_GENERATE_STATIC(name, type, field, cmp) \
    RB_GENERATE_INTERNAL(name, type, field, cmp, static)
#define RB_GENERATE_INTERNAL(name, type, field, cmp, attr)                               \
    attr void                                                                            \
        name##_RB_INSERT_COLOR(struct name *head, struct type *elm)                      \
    {                                                                                    \
        struct type *parent, *gparent, *tmp;                                             \
        while ((parent = RB_PARENT(elm, field)) != NULL &&                               \
               RB_COLOR(parent, field) == RB_RED)                                        \
        {                                                                                \
            gparent = RB_PARENT(parent, field);                                          \
            if (parent == RB_LEFT(gparent, field))                                       \
            {                                                                            \
                tmp = RB_RIGHT(gparent, field);                                          \
                if (tmp && RB_COLOR(tmp, field) == RB_RED)                               \
                {                                                                        \
                    RB_COLOR(tmp, field) = RB_BLACK;                                     \
                    RB_SET_BLACKRED(parent, gparent, field);                             \
                    elm = gparent;                                                       \
                    continue;                                                            \
                }                                                                        \
                if (RB_RIGHT(parent, field) == elm)                                      \
                {                                                                        \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                            \
                    tmp = parent;                                                        \
                    parent = elm;                                                        \
                    elm = tmp;                                                           \
                }                                                                        \
                RB_SET_BLACKRED(parent, gparent, field);                                 \
                RB_ROTATE_RIGHT(head, gparent, tmp, field);                              \
            }                                                                            \
            else                                                                         \
            {                                                                            \
                tmp = RB_LEFT(gparent, field);                                           \
                if (tmp && RB_COLOR(tmp, field) == RB_RED)                               \
                {                                                                        \
                    RB_COLOR(tmp, field) = RB_BLACK;                                     \
                    RB_SET_BLACKRED(parent, gparent, field);                             \
                    elm = gparent;                                                       \
                    continue;                                                            \
                }                                                                        \
                if (RB_LEFT(parent, field) == elm)                                       \
                {                                                                        \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                           \
                    tmp = parent;                                                        \
                    parent = elm;                                                        \
                    elm = tmp;                                                           \
                }                                                                        \
                RB_SET_BLACKRED(parent, gparent, field);                                 \
                RB_ROTATE_LEFT(head, gparent, tmp, field);                               \
            }                                                                            \
        }                                                                                \
        RB_COLOR(head->rbh_root, field) = RB_BLACK;                                      \
    }                                                                                    \
                                                                                         \
    attr void                                                                            \
        name##_RB_REMOVE_COLOR(struct name *head, struct type *parent, struct type *elm) \
    {                                                                                    \
        struct type *tmp;                                                                \
        while ((elm == NULL || RB_COLOR(elm, field) == RB_BLACK) &&                      \
               elm != RB_ROOT(head))                                                     \
        {                                                                                \
            if (RB_LEFT(parent, field) == elm)                                           \
            {                                                                            \
                tmp = RB_RIGHT(parent, field);                                           \
                if (RB_COLOR(tmp, field) == RB_RED)                                      \
                {                                                                        \
                    RB_SET_BLACKRED(tmp, parent, field);                                 \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                            \
                    tmp = RB_RIGHT(parent, field);                                       \
                }                                                                        \
                if ((RB_LEFT(tmp, field) == NULL ||                                      \
                     RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) &&                \
                    (RB_RIGHT(tmp, field) == NULL ||                                     \
                     RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK))                 \
                {                                                                        \
                    RB_COLOR(tmp, field) = RB_RED;                                       \
                    elm = parent;                                                        \
                    parent = RB_PARENT(elm, field);                                      \
                }                                                                        \
                else                                                                     \
                {                                                                        \
                    if (RB_RIGHT(tmp, field) == NULL ||                                  \
                        RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK)               \
                    {                                                                    \
                        struct type *oleft;                                              \
                        if ((oleft = RB_LEFT(tmp, field)) != NULL)                       \
                            RB_COLOR(oleft, field) = RB_BLACK;                           \
                        RB_COLOR(tmp, field) = RB_RED;                                   \
                        RB_ROTATE_RIGHT(head, tmp, oleft, field);                        \
                        tmp = RB_RIGHT(parent, field);                                   \
                    }                                                                    \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);                      \
                    RB_COLOR(parent, field) = RB_BLACK;                                  \
                    if (RB_RIGHT(tmp, field))                                            \
                        RB_COLOR(RB_RIGHT(tmp, field), field) = RB_BLACK;                \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                            \
                    elm = RB_ROOT(head);                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
            else                                                                         \
            {                                                                            \
                tmp = RB_LEFT(parent, field);                                            \
                if (RB_COLOR(tmp, field) == RB_RED)                                      \
                {                                                                        \
                    RB_SET_BLACKRED(tmp, parent, field);                                 \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                           \
                    tmp = RB_LEFT(parent, field);                                        \
                }                                                                        \
                if ((RB_LEFT(tmp, field) == NULL ||                                      \
                     RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) &&                \
                    (RB_RIGHT(tmp, field) == NULL ||                                     \
                     RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK))                 \
                {                                                                        \
                    RB_COLOR(tmp, field) = RB_RED;                                       \
                    elm = parent;                                                        \
                    parent = RB_PARENT(elm, field);                                      \
                }                                                                        \
                else                                                                     \
                {                                                                        \
                    if (RB_LEFT(tmp, field) == NULL ||                                   \
                        RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK)                \
                    {                                                                    \
                        struct type *oright;                                             \
                        if ((oright = RB_RIGHT(tmp, field)) != NULL)                     \
                            RB_COLOR(oright, field) = RB_BLACK;                          \
                        RB_COLOR(tmp, field) = RB_RED;                                   \
                        RB_ROTATE_LEFT(head, tmp, oright, field);                        \
                        tmp = RB_LEFT(parent, field);                                    \
                    }                                                                    \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);                      \
                    RB_COLOR(parent, field) = RB_BLACK;                                  \
                    if (RB_LEFT(tmp, field))                                             \
                        RB_COLOR(RB_LEFT(tmp, field), field) = RB_BLACK;                 \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                           \
                    elm = RB_ROOT(head);                                                 \
                    break;                                                               \
                }                                                                        \
            }                                                                            \
        }                                                                                \
        if (elm)                                                                         \
            RB_COLOR(elm, field) = RB_BLACK;                                             \
    }                                                                                    \
                                                                                         \
    attr struct type *                                                                   \
        name##_RB_REMOVE(struct name *head, struct type *elm)                            \
    {                                                                                    \
        struct type *child, *parent, *old = elm;                                         \
        int color;                                                                       \
        if (RB_LEFT(elm, field) == NULL)                                                 \
            child = RB_RIGHT(elm, field);                                                \
        else if (RB_RIGHT(elm, field) == NULL)                                           \
            child = RB_LEFT(elm, field);                                                 \
        else                                                                             \
        {                                                                                \
            struct type *left;                                                           \
            elm = RB_RIGHT(elm, field);                                                  \
            while ((left = RB_LEFT(elm, field)) != NULL)                                 \
                elm = left;                                                              \
            child = RB_RIGHT(elm, field);                                                \
            parent = RB_PARENT(elm, field);                                              \
            color = RB_COLOR(elm, field);                                                \
            if (child)                                                                   \
                RB_PARENT(child, field) = parent;                                        \
            if (parent)                                                                  \
            {                                                                            \
                if (RB_LEFT(parent, field) == elm)                                       \
                    RB_LEFT(parent, field) = child;                                      \
                else                                                                     \
                    RB_RIGHT(parent, field) = child;                                     \
                RB_AUGMENT(parent);                                                      \
            }                                                                            \
            else                                                                         \
                RB_ROOT(head) = child;                                                   \
            if (RB_PARENT(elm, field) == old)                                            \
                parent = elm;                                                            \
            (elm)->field = (old)->field;                                                 \
            if (RB_PARENT(old, field))                                                   \
            {                                                                            \
                if (RB_LEFT(RB_PARENT(old, field), field) == old)                        \
                    RB_LEFT(RB_PARENT(old, field), field) = elm;                         \
                else                                                                     \
                    RB_RIGHT(RB_PARENT(old, field), field) = elm;                        \
                RB_AUGMENT(RB_PARENT(old, field));                                       \
            }                                                                            \
            else                                                                         \
                RB_ROOT(head) = elm;                                                     \
            RB_PARENT(RB_LEFT(old, field), field) = elm;                                 \
            if (RB_RIGHT(old, field))                                                    \
                RB_PARENT(RB_RIGHT(old, field), field) = elm;                            \
            if (parent)                                                                  \
            {                                                                            \
                left = parent;                                                           \
                do                                                                       \
                {                                                                        \
                    RB_AUGMENT(left);                                                    \
                } while ((left = RB_PARENT(left, field)) != NULL);                       \
            }                                                                            \
            goto color;                                                                  \
        }                                                                                \
        parent = RB_PARENT(elm, field);                                                  \
        color = RB_COLOR(elm, field);                                                    \
        if (child)                                                                       \
            RB_PARENT(child, field) = parent;                                            \
        if (parent)                                                                      \
        {                                                                                \
            if (RB_LEFT(parent, field) == elm)                                           \
                RB_LEFT(parent, field) = child;                                          \
            else                                                                         \
                RB_RIGHT(parent, field) = child;                                         \
            RB_AUGMENT(parent);                                                          \
        }                                                                                \
        else                                                                             \
            RB_ROOT(head) = child;                                                       \
    color:                                                                               \
        if (color == RB_BLACK)                                                           \
            name##_RB_REMOVE_COLOR(head, parent, child);                                 \
        return (old);                                                                    \
    }                                                                                    \
                                                                                         \
    /* Inserts a node into the RB tree */                                                \
    attr struct type *                                                                   \
        name##_RB_INSERT(struct name *head, struct type *elm)                            \
    {                                                                                    \
        struct type *tmp;                                                                \
        struct type *parent = NULL;                                                      \
        int comp = 0;                                                                    \
        tmp = RB_ROOT(head);                                                             \
        while (tmp)                                                                      \
        {                                                                                \
            parent = tmp;                                                                \
            comp = (cmp)(elm, parent);                                                   \
            if (comp < 0)                                                                \
                tmp = RB_LEFT(tmp, field);                                               \
            else if (comp > 0)                                                           \
                tmp = RB_RIGHT(tmp, field);                                              \
            else                                                                         \
                return (tmp);                                                            \
        }                                                                                \
        RB_SET(elm, parent, field);                                                      \
        if (parent != NULL)                                                              \
        {                                                                                \
            if (comp < 0)                                                                \
                RB_LEFT(parent, field) = elm;                                            \
            else                                                                         \
                RB_RIGHT(parent, field) = elm;                                           \
            RB_AUGMENT(parent);                                                          \
        }                                                                                \
        else                                                                             \
            RB_ROOT(head) = elm;                                                         \
        name##_RB_INSERT_COLOR(head, elm);                                               \
        return (NULL);                                                                   \
    }                                                                                    \
                                                                                         \
    /* Finds the node with the same key as elm */                                        \
    attr struct type *                                                                   \
        name##_RB_FIND(struct name *head, struct type *elm)                              \
    {                                                                                    \
        struct type *tmp = RB_ROOT(head);                                                \
        int comp;                                                                        \
        while (tmp)                                                                      \
        {                                                                                \
            comp = cmp(elm, tmp);                                                        \
            if (comp < 0)                                                                \
                tmp = RB_LEFT(tmp, field);                                               \
            else if (comp > 0)                                                           \
                tmp = RB_RIGHT(tmp, field);                                              \
            else                                                                         \
                return (tmp);                                                            \
        }                                                                                \
        return (NULL);                                                                   \
    }                                                                                    \
                                                                                         \
    /* Finds the first node greater than or equal to the search key */                   \
    attr struct type *                                                                   \
        name##_RB_NFIND(struct name *head, struct type *elm)                             \
    {                                                                                    \
        struct type *tmp = RB_ROOT(head);                                                \
        struct type *res = NULL;                                                         \
        int comp;                                                                        \
        while (tmp)                                                                      \
        {                                                                                \
            comp = cmp(elm, tmp);                                                        \
            if (comp < 0)                                                                \
            {                                                                            \
                res = tmp;                                                               \
                tmp = RB_LEFT(tmp, field);                                               \
            }                                                                            \
            else if (comp > 0)                                                           \
                tmp = RB_RIGHT(tmp, field);                                              \
            else                                                                         \
                return (tmp);                                                            \
        }                                                                                \
        return (res);                                                                    \
    }                                                                                    \
                                                                                         \
    /* ARGSUSED */                                                                       \
    attr struct type *                                                                   \
        name##_RB_NEXT(struct type *elm)                                                 \
    {                                                                                    \
        if (RB_RIGHT(elm, field))                                                        \
        {                                                                                \
            elm = RB_RIGHT(elm, field);                                                  \
            while (RB_LEFT(elm, field))                                                  \
                elm = RB_LEFT(elm, field);                                               \
        }                                                                                \
        else                                                                             \
        {                                                                                \
            if (RB_PARENT(elm, field) &&                                                 \
                (elm == RB_LEFT(RB_PARENT(elm, field), field)))                          \
                elm = RB_PARENT(elm, field);                                             \
            else                                                                         \
            {                                                                            \
                while (RB_PARENT(elm, field) &&                                          \
                       (elm == RB_RIGHT(RB_PARENT(elm, field), field)))                  \
                    elm = RB_PARENT(elm, field);                                         \
                elm = RB_PARENT(elm, field);                                             \
            }                                                                            \
        }                                                                                \
        return (elm);                                                                    \
    }                                                                                    \
                                                                                         \
    /* ARGSUSED */                                                                       \
    attr struct type *                                                                   \
        name##_RB_PREV(struct type *elm)                                                 \
    {                                                                                    \
        if (RB_LEFT(elm, field))                                                         \
        {                                                                                \
            elm = RB_LEFT(elm, field);                                                   \
            while (RB_RIGHT(elm, field))                                                 \
                elm = RB_RIGHT(elm, field);                                              \
        }                                                                                \
        else                                                                             \
        {                                                                                \
            if (RB_PARENT(elm, field) &&                                                 \
                (elm == RB_RIGHT(RB_PARENT(elm, field), field)))                         \
                elm = RB_PARENT(elm, field);                                             \
            else                                                                         \
            {                                                                            \
                while (RB_PARENT(elm, field) &&                                          \
                       (elm == RB_LEFT(RB_PARENT(elm, field), field)))                   \
                    elm = RB_PARENT(elm, field);                                         \
                elm = RB_PARENT(elm, field);                                             \
            }                                                                            \
        }                                                                                \
        return (elm);                                                                    \
    }                                                                                    \
                                                                                         \
    attr struct type *                                                                   \
        name##_RB_MINMAX(struct name *head, int val)                                     \
    {                                                                                    \
        struct type *tmp = RB_ROOT(head);                                                \
        struct type *parent = NULL;                                                      \
        while (tmp)                                                                      \
        {                                                                                \
            parent = tmp;                                                                \
            if (val < 0)                                                                 \
                tmp = RB_LEFT(tmp, field);                                               \
            else                                                                         \
                tmp = RB_RIGHT(tmp, field);                                              \
        }                                                                                \
        return (parent);                                                                 \
    }

#define RB_NEGINF -1
#define RB_INF 1

#define RB_INSERT(name, x, y) name##_RB_INSERT(x, y)
#define RB_REMOVE(name, x, y) name##_RB_REMOVE(x, y)
#define RB_FIND(name, x, y) name##_RB_FIND(x, y)
#define RB_NFIND(name, x, y) name##_RB_NFIND(x, y)
#define RB_NEXT(name, x, y) name##_RB_NEXT(y)
#define RB_PREV(name, x, y) name##_RB_PREV(y)
#define RB_MIN(name, x) name##_RB_MINMAX(x, RB_NEGINF)
#define RB_MAX(name, x) name##_RB_MINMAX(x, RB_INF)

#define RB_FOREACH(x, name, head)  \
    for ((x) = RB_MIN(name, head); \
         (x) != NULL;              \
         (x) = name##_RB_NEXT(x))

#define RB_FOREACH_FROM(x, name, y)                               \
    for ((x) = (y);                                               \
         ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL); \
         (x) = (y))

#define RB_FOREACH_SAFE(x, name, head, y)                         \
    for ((x) = RB_MIN(name, head);                                \
         ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL); \
         (x) = (y))

#define RB_FOREACH_REVERSE(x, name, head) \
    for ((x) = RB_MAX(name, head);        \
         (x) != NULL;                     \
         (x) = name##_RB_PREV(x))

#define RB_FOREACH_REVERSE_FROM(x, name, y)                       \
    for ((x) = (y);                                               \
         ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL); \
         (x) = (y))

#define RB_FOREACH_REVERSE_SAFE(x, name, head, y)                 \
    for ((x) = RB_MAX(name, head);                                \
         ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL); \
         (x) = (y))

#endif