#ifndef ARRAY_H_
#define ARRAY_H_

#define INITIAL_CAPACITY 256

#define ARRAY(T)           \
    struct {               \
        T *items;          \
        uint32_t count;    \
        uint32_t capacity; \
    }

// NOTE: New Array
#define array_new(array)                                                            \
    do {                                                                            \
        (array)->items = calloc(INITIAL_CAPACITY, sizeof(*(array)->items));         \
        assert((array)->items != NULL && "Initial malloc() for new Array Failed");  \
        (array)->count = 0;                                                         \
        (array)->capacity = INITIAL_CAPACITY;                                       \
    } while(0)

// NOTE: Remove An Element of Specified Index and Shift the Array
#define array_delete_item(array, index)                             \
    do {                                                            \
        assert(index < (array)->count);                             \
        assert((array)->items != NULL);                             \
        if (index < (array)->count - 1) {                           \
            for (uint32_t i = index; i < (array)->count - 1; i++) { \
                (array)->items[i] = (array)->items[i + 1];          \
            }                                                       \
        }                                                           \
        (array)->count--;                                           \
    } while (0)

// NOTE: Remove Last Element From Array
#define array_pop(array)                                                           \
    do {                                                                           \
        if ((array)->count == 0) {                                                 \
            fprintf(stderr, "Warning: Attempting to Pop From An empty Array.\n");  \
            break;                                                                 \
        } else {                                                                   \
            array_delete_item((array), (array)->count - 1);                        \
        }                                                                          \
    } while (0)

// NOTE: Push An Element To A Specified Index in An Array
#define array_push(array, item, index)                                                                 \
    do {                                                                                               \
        if ((array)->count >= (array)->capacity) {                                                     \
            (array)->capacity = (array)->capacity == 0 ? INITIAL_CAPACITY : (array)->capacity * 2 + 1; \
            (array)->items = realloc((array)->items, sizeof(*(array)->items)*(array)->capacity);       \
            assert((array)->items != NULL && "Memory Reallocation For Array Failed.");                 \
        }                                                                                              \
        if (index != (array)->count) {                                                                 \
            for (uint32_t i = (array)->count; i > index; --i) {                                        \
                (array)->items[i] = (array)->items[i - 1];                                             \
            }                                                                                          \
        }                                                                                              \
        (array)->items[index] = item;                                                                  \
        (array)->count++;                                                                              \
    } while (0)

// NOTE: Append An Item To The Array
#define array_append(array, item) array_push((array), item, (array)->count)

// NOTE: Destroy the Array
#define array_delete(array)    \
    do {                       \
        free((array)->items);  \
        (array)->count = 0;    \
        (array)->capacity = 0; \
    } while(0)

// NOTE: Output the Array x-tics
#define array_analysis(array)\
    printf("Array Count: %u\nArray Capacity: %u\n\n", (array)->count, (array)->capacity);\

#define array_clear(array)  \
    do {                    \
        (array)->count = 0; \
    } while (0)


#endif // ARRAY_H_
