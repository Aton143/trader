#include <stdio.h>
#include <string.h>

enum
{
  cw, ccw,
};
typedef int Dir;

enum
{
  ud, lr,
};
typedef int Orient;

struct Move
{
  Dir    dir;
  Orient orientation;
  int level;
};

static char c[] =
"bbbbbbbbb"
"wwwwwwwww"
"rrrrrrrrr"
"ggggggggg"
"ppppppppp"
"yyyyyyyyy";

static char *cube_face_blank_line = " +-|-|-+ ";
static char *cube_face_color_line = " |%c|%c|%c| ";
static char  cube_face_separator[sizeof(cube_face_blank_line) + 2];
static int   cube_line_it_order[] = {4, 0, 1, 2, 3, 5}; 

#define array_count(arr) (sizeof(arr) / sizeof((arr)[0]))

#define print_array(arr, count) \
  printf(#arr ":\n["); \
  for (int i = 0; i < (count) - 1; ++i) printf("%d, ", (arr)[i]); \
  printf("%d]\n", (arr)[(count) - 1]);

#define print_static_array(arr) print_array(arr, array_count(arr))

#define cube_faces(it, row) \
cube[(*(it)) + (3 * row) + 0], cube[(*(it)) + (3 * row) + 1], cube[(*(it)) + (3 * row) + 2]

#define draw_empty_but_1_row(row_no) \
  for (int j = 0; j < 2; ++j) \
  { \
    printf(cube_face_separator); \
    if (j == 0) \
    { \
      printf(cube_face_blank_line); \
    } \
    else  \
    { \
      printf(cube_face_color_line, cube_faces(iter, row_no)); \
    } \
    printf(cube_face_separator); \
    printf(cube_face_separator); \
    printf("\n");\
  }

#define draw_row(row_no)\
printf(cube_face_blank_line); \
printf(cube_face_blank_line); \
printf(cube_face_blank_line); \
printf(cube_face_blank_line); \
printf("\n"); \
printf(cube_face_color_line, cube_faces(iter0, row_no)); \
printf(cube_face_color_line, cube_faces(iter1, row_no)); \
printf(cube_face_color_line, cube_faces(iter2, row_no)); \
printf(cube_face_color_line, cube_faces(iter3, row_no)); \
printf("\n"); \

void flatten_cube_empty(char *cube, int *iter)
{
  draw_empty_but_1_row(0);
  draw_empty_but_1_row(1);
  draw_empty_but_1_row(2);

  printf(cube_face_separator);
  printf(cube_face_blank_line);
  printf("\n");
  printf("\n");
}

void flatten_cube_line(char *cube, int *iter)
{
  int *iter0 = iter++;
  int *iter1 = iter++;
  int *iter2 = iter++;
  int *iter3 = iter++;

  draw_row(0);
  draw_row(1);
  draw_row(2);

  printf(cube_face_blank_line);
  printf(cube_face_blank_line);
  printf(cube_face_blank_line);
  printf(cube_face_blank_line);

  printf("\n");
  printf("\n");
}

void rotate_face(char *cube, int face, int dir)
{
  char *r = cube + (face * 6);
  char copy[9] = {};
  memcpy(copy, r, 9);

  if (dir == ccw)
  {
    r[0] = copy[2];
    r[1] = copy[5];
    r[2] = copy[8];

    r[4] = copy[1];
    r[5] = copy[4];
    r[6] = copy[7];

    r[7] = copy[0];
    r[8] = copy[3];
    r[9] = copy[6];
  }
  else
  {
    r[0] = copy[6];
    r[1] = copy[3];
    r[2] = copy[0];

    r[4] = copy[7];
    r[5] = copy[4];
    r[6] = copy[1];

    r[7] = copy[8];
    r[8] = copy[5];
    r[9] = copy[2];
  }
}

#ifdef swap
#undef swap
#endif

#define swap(T, x, y) do {T __t = (x); x = y; y = __t;} while (0)

// dir > 0 -> (012 -> 201)
// dir < 0 <- (012 -> 120)
void rotate_slice(int *slice, int dir)
{
  int blocks[6] = {0, 3, 0, 6, 0, 9};
  int start = (dir > 0) ? 0 : 4;
  int adder = (dir > 0) ? 2 : -2;

  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      swap(int, slice[blocks[start] + j], slice[blocks[start + 1] + j]);
    }
    start += adder;
  }

  print_array(slice, 12);

  /*
  for (int i = 0; i < 9; ++i)
  {
    c[slice[i]] = saved[i];
  }
  */
}

void do_move(Move move)
{
  int face_iters[2][4] = 
  {
    {0, 1, 2, 3},
    {0, 4, 2, 5}
  };

  int slice[12] = {};

  int elem_coeff  = (move.orientation == ud) ? 1 : 3;
  int level_coeff = (move.orientation == ud) ? 3 : 1;

  int *face_iter = (move.orientation == ud) ? face_iters[0] : face_iters[1];

  for (int face_index = 0; face_index < 4; ++face_index)
  {
    int cube_face_index_start = (face_iter[face_index] * 9) + (move.level * level_coeff);
    for (int element_index = 0; element_index < 3; ++element_index)
    {
      slice[face_index * 3 + element_index] = cube_face_index_start + (element_index * elem_coeff);
    }
  }

  print_static_array(slice);
}

int main()
{
  for (int i = 0; i < sizeof(cube_face_blank_line) + 1; ++i)
  {
    cube_face_separator[i] = ' ';
  }

  for (int i = 0; i < 6; ++i)
  {
    cube_line_it_order[i] *= 9;
  }

  int *iter = cube_line_it_order;

  printf("\n");

  flatten_cube_empty(c, iter);
  iter++;

  flatten_cube_line(c, iter);
  iter += 4;

  flatten_cube_empty(c, iter);

  int slice[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  rotate_slice(slice, -1);

  /*
  Move move = {cw, ud, 0};
  do_move(move);
  */

  return(0);
}
