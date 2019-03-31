#include <stdio.h>			// stdio: printf(), scanf()
#include <stdlib.h>                     // stdlib: malloc()

typedef struct _List {			// structure _List..
    int value;				//   with value [int]
    struct _List *next;			//   and next [pointer]
} List;					// ..as type List

int main() {
  List zeroitem;			// Declare static memory for first item
  zeroitem.value = 0;                   // Give it a default value
  List *lastitem = &zeroitem;           // Declare a pointer to the last item
  lastitem->next = NULL;                // Last item is firt item, no next item
  while (1) {
    int number = -1;
    printf("Number: ");			// Display a prompt
    scanf("%d", &number);		// Read a number
    if (number < 0) {
      break;				// Exit loop if negative
    } else {
      List *nextitem = (List *)malloc(sizeof(List));	// Dynamic memory for new item
      nextitem->value = number;				// - Value is the number read
      nextitem->next = NULL;				// - Is the last item (no next)
      lastitem->next = nextitem;	// Next item after previous lastitem is new item 
      lastitem = nextitem;		// lastitem is now the new item
    }
  }
  List *li = zeroitem.next;		// List iterator points to second item (first is 0)
  while (li != NULL) {			// While this item exists
    printf(" %d", li->value);		// - Display its value
    li = li->next;			// - Point iterator to next item
  }
  printf("\n");				// Linebreak at end of list
  List *lf = zeroitem.next;		// Point to first dynamic item (second item in list)
  while (lf != NULL) {			// While this item exists
    List *thislf = lf;			// - Copy memory address (since lf changes)
    lf = lf->next;			// - Point iterator to next item (mem still declared)
    free(thislf);			// - Free dynamically declared memory
  }
  return 0;
}
