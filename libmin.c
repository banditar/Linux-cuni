#include "libmin.h"

int
min (int a[], size_t length) {
        int i;
        int min = a[0];
        for (i = 0; i < length; i++) {
                if (a[i] < min) {
                        min = a[i];
                }
        }
				return min;
} 
