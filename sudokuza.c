/* ****************************************************************************

    Sudokuza. The solver of Sudoku puzzles
    Written by Ilya O. Levin, http://www.literatecode.com 
    Oct 2005

   ****************************************************************************
*/
#ifdef _MSVC
#pragma warning(push, 1)
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef _MSVC
#pragma warning (pop)
#endif

#define uint8   unsigned char
#define G_COLS  9
#define G_ROWS  9 
#define G_NUMS  10 /* 9+1 */

typedef struct {
  uint8 n[G_NUMS]; 
} sudoku_cell;

typedef struct {
  sudoku_cell sq[G_COLS][G_ROWS];
} sudoku_grid;

#define ALLOC_GRID(x)     (x = (sudoku_grid *) malloc(sizeof(sudoku_grid)) )

int  read_puzzle(sudoku_grid *, char *);
void output_grid(sudoku_grid *);
int  solve_grid(sudoku_grid *);
int  verify_grid(sudoku_grid *);

void _analyzerow(sudoku_grid *, uint8, uint8 *);
void _analyzecol(sudoku_grid *, uint8, uint8 *);
void _analyze3x3(sudoku_grid *, uint8, uint8, uint8 *);
void _revisecells(sudoku_grid *, uint8 *);
void _chkrow4lone(sudoku_grid *, uint8, uint8 *);
void _chkcol4lone(sudoku_grid *, uint8, uint8 *);
void output_trial_grid (sudoku_grid *);


static char *default_puzzle_filename = "puzzle.txt";


int main (int argc, char *argv[])
{
  char *filename = default_puzzle_filename;
  sudoku_grid *grd; 
  int rv;

  printf("Sudokuza. The solver of Sudoku puzzles.\n" \
         "Written by Ilya O. Levin, http://www.literatecode.com\n\n");

  if ( ALLOC_GRID(grd) == NULL ) { printf("out of memory\n"); return 1;}
  memset(grd, 0, sizeof(sudoku_grid));

  if (argc > 1) filename = argv[1];

  if ( read_puzzle(grd, filename) != 0 )
  {
     printf("*** Puzzle:\n"); output_grid(grd);
     rv = solve_grid(grd);
     printf("\n*** %s:\n", ((rv > 0) ? "Solution" : "Gave up at"));
     output_grid(grd); 
  }
  else printf("Unable read a puzzle from %s\n", filename);
  
  free(grd);

  return 0;
} /* main */


/* ------------------------------------------------------------------------ 
   read_puzzle() - read a puzzle from the specified file and initialize 
   given sudoku_grid structure. Return 0 in case of an error or non-zero
   otherwise. 
 ----------------------------------------------------------------------- */
int read_puzzle (sudoku_grid *p, char *filename)
{
  uint8 buf[32];
  FILE *f;
  int rv = 0; 
  register uint8 i, j;

  if ( (f = fopen(filename, "r")) != NULL )
  {
    j = 0;
    while ( !feof(f) ) if ( fgets((char*) buf, sizeof(buf)-2, f) != NULL )
    {
       for ( i = 0; ( (i < G_COLS) && (buf[i] != 0) ); i++) 
         if ( isdigit(buf[i]) ) p->sq[i][j].n[0] = buf[i]-'0';
       rv++;
       if ( ++j > G_ROWS) break;
    }
    fclose(f);
  } 

  return rv; 
} /* read_puzzle */


/* ------------------------------------------------------------------------ 
   output_grid() - displays the Sudoku grid
 ----------------------------------------------------------------------- */
void output_grid (sudoku_grid *p)
{
  register uint8 i, j;

  for (j = 0; j < G_ROWS; j++) 
  {
     for (i = 0; i < G_COLS; i++) 
     if ( p->sq[i][j].n[0] > 0 ) printf("%d ", p->sq[i][j].n[0]); 
         else printf(". ");
     printf("\n");
  }
} /* output_grid */


/* ------------------------------------------------------------------------ 
   solve_grid() - solve the puzzle from the given Sudoku grid. Return 0
   if the puzzle was not solved or 1 if it was solved.
 ----------------------------------------------------------------------- */
int solve_grid (sudoku_grid *p)
{
  sudoku_grid *p0; 
  int rv = 1; 
  uint8 stt; 
  register uint8 i, j, l, x, y;

  /* solve */
  do 
  {
    stt = 0;
    for (i = 0; i < G_ROWS; i++) _analyzerow(p, i, &stt);
    for (i = 0; i < G_COLS; i++) _analyzecol(p, i, &stt);
    for (j = 0; j < G_ROWS; j+=3)
        for (i = 0; i < G_COLS; i+=3) _analyze3x3(p, i, j, &stt);
    for (j = 0; j < G_ROWS; j++) _chkrow4lone(p, j, &stt);
    for (i = 0; i < G_COLS; i++) _chkcol4lone(p, i, &stt);
    _revisecells(p, &stt);
  } while ( stt > 0 );

  /* check for any non-solved cell and set rv=0 if such cell found */
  for (j = 0; ((j < G_ROWS) && (rv > 0)); j++) for (i = 0; i < G_COLS; i++) 
     if ( p->sq[i][j].n[0] == 0 ) {rv = 0;  break;}
  j--;

  /* retry to solve the puzzle for each candidate left in the cell */
  for (l = 1; ((l < G_NUMS) && (rv == 0)); l++) if (p->sq[i][j].n[l] == 0) 
  {
     if ( ALLOC_GRID(p0) != NULL )
     {
        memset(p0, 0, sizeof(sudoku_grid));
        p0->sq[i][j].n[0] = l;
        for (y = 0; y < G_ROWS; y++) for (x = 0; x < G_COLS; x++) 
          if (p->sq[x][y].n[0] != 0 ) p0->sq[x][y].n[0] = p->sq[x][y].n[0];

        rv = solve_grid(p0);
        if ( rv != 0 ) memcpy(p, p0, sizeof(sudoku_grid));
        free(p0);
     }
  }

  if ( rv == 1 ) rv = verify_grid(p);

  return rv;
} /* solve_grid */


/* ------------------------------------------------------------------------
 verify_grid() - check if the grid is a properly solved puzzle assuming 
  grid is a 9x9 one with G_COLS == G_ROWS. 
 ----------------------------------------------------------------------- */
int  verify_grid(sudoku_grid *p)
{
  int rv = 1, s0, s1;
  register uint8 i, j;
  
  for(i = 0; ((i < G_COLS) && (rv >0)); i++)
  {
    s0 = s1 = 0;
    for (j = 0; j < G_ROWS; j++) 
    { 
       s0 += p->sq[i][j].n[0]; s1 += p->sq[j][i].n[0];
    }
    rv = (int) (s0 == 45) && (s1 == 45); 
  }

  return rv;
} /* verify_grid */


/* ------------------------------------------------------------------------ 
   _analyzerow() - eliminate solved values from the rest of cells in the 
   given row #j. Set g_state to 1 if the row has been changed. 
 ----------------------------------------------------------------------- */
void _analyzerow (sudoku_grid *p, uint8 j, uint8 *g_state)
{
  register uint8 i, k, x;

  for (i = 0; i< G_COLS; i++) if ( p->sq[i][j].n[0] > 0 )
  {
    x = p->sq[i][j].n[0]; 
    for (k = 0; k < G_COLS; k++) 
    { 
      if ( p->sq[k][j].n[x] == 0 ) *g_state = 1;
      p->sq[k][j].n[x]++;
    }
  }

} /* _analyzerow */


/* ------------------------------------------------------------------------ 
   _analyzecol() - same as _analyzerow() above but for a column #i
 ----------------------------------------------------------------------- */
void _analyzecol (sudoku_grid *p, uint8 i, uint8 *g_state)
{
  register uint8 j, k, x;

  for (j = 0; j< G_ROWS; j++) if ( p->sq[i][j].n[0] > 0 )
  {
    x = p->sq[i][j].n[0];  
    for (k = 0; k < G_ROWS; k++) 
    {
      if ( p->sq[i][k].n[x] == 0 ) *g_state = 1;
      p->sq[i][k].n[x]++;
    }
  }

} /* _analyzecol */


/* ------------------------------------------------------------------------ 
  _analyze3x3() - same as _analyzerow()/_analyzecol() above but for a 3x3
  block of cells. Parameters x and y are the column and the row of a top 
  left cell in the block.
 ----------------------------------------------------------------------- */
void _analyze3x3 (sudoku_grid *p, uint8 x, uint8 y, uint8 *g_state)
{
  register uint8 i, j, k;

  if ( (x % 3) || (y % 3) || ((x+3) > G_COLS ) || ((y+3) > G_ROWS )
     ) return; // invalid block boundaries

  #define FIXCELLS for (i = x; i < (x+3); i++)  \
                   for (j = y; j < (y+3); j++)  \
                     {  \
                        if ( p->sq[i][j].n[k] == 0 ) *g_state = 1; \
                        p->sq[i][j].n[k]++; \
                     }

  k = p->sq[x  ][y].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+1][y].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+2][y].n[0];  if ( k ) FIXCELLS;

  k = p->sq[x  ][y+1].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+1][y+1].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+2][y+1].n[0];  if ( k ) FIXCELLS;

  k = p->sq[x  ][y+2].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+1][y+2].n[0];  if ( k ) FIXCELLS;
  k = p->sq[x+2][y+2].n[0];  if ( k ) FIXCELLS;

  #undef FIXCELLS

} /* _analyze3x3 */


/* ------------------------------------------------------------------------ 
  _revisecells() - check the Sudoku grid and mark all single-candidate cells
  as solved
 ----------------------------------------------------------------------- */
void _revisecells (sudoku_grid *p, uint8 *g_state)
{
  register uint8 i, j, k, l, m;

  for (j = 0; j < G_ROWS; j++) for (i = 0; i < G_COLS; i++) 
    if (p->sq[i][j].n[0] == 0 ) 
    {
      l = 0; m = 0;
      for (k = 1; k < G_NUMS; k++) if ( p->sq[i][j].n[k] == 0) { l++; m = k;}
      if ( l == 1 ) 
      {
        p->sq[i][j].n[0] = m;
        for (k = 1; k < G_NUMS; k++) p->sq[i][j].n[k] = 0;
      }
      else if ( l == 0 ) *g_state = 0; /* force end of solution */
    }

} /* _revisecells */


/* ------------------------------------------------------------------------ 
   _chkrow4lone() - check cells in the specified row #j for an obviously 
   lone candidate.
 ----------------------------------------------------------------------- */
void _chkrow4lone (sudoku_grid *p, uint8 j, uint8 *g_state)
{
  register uint8 i, k, l, n;

  for (i = 0; i< G_COLS; i++) if ( p->sq[i][j].n[0] == 0 )
  for ( k = 1; k < G_NUMS; k++) if ( p->sq[i][j].n[k] == 0 )
  {
     n = 0; 
     for (l = 0; l < G_COLS;l++) 
        if (  (p->sq[l][j].n[0] == 0) && (p->sq[l][j].n[k] == 0) ) n++;
     if ( n == 1 ) /* a standalone candidate found */
     {
         for (l = 1; l < G_NUMS; l++) p->sq[i][j].n[l]++;
         p->sq[i][j].n[k] = 0; *g_state = 1; break;
     }
  }

} /* _chkrow4lone */


/* ------------------------------------------------------------------------ 
  _chkcol4lone() - same as _chkrow4lone() above but for the column #i
 ----------------------------------------------------------------------- */
void _chkcol4lone (sudoku_grid *p, uint8 i, uint8 *g_state)
{
  register uint8 j, k, l, n;

  for (j = 0; j< G_COLS; j++) if ( p->sq[i][j].n[0] == 0 )
  for ( k = 1; k < G_NUMS; k++) if ( p->sq[i][j].n[k] == 0 )
  {
     n = 0; 
     for (l = 0; l < G_ROWS;l++) 
        if (  (p->sq[i][l].n[0] == 0) && (p->sq[i][l].n[k] == 0) ) n++;
     if ( n == 1 ) /* a standalone candidate found */
     {
         for (l = 1; l < G_NUMS; l++) p->sq[i][j].n[l]++;
         p->sq[i][j].n[k] = 0; *g_state = 1; break;
     }
  }

} /* _chkcol4lone */


/* ------------------------------------------------------------------------ 
   output_trial_grid() - display the current state of the given Sudoku 
   grid in a form of a trial matrix. 
 ----------------------------------------------------------------------- */
void output_trial_grid (sudoku_grid *p)
{
  register uint8 i, j;

  for (j = 0; j < G_ROWS; j++) 
  {
	  for (i = 0; i < G_COLS; i++) 
             if (p->sq[i][j].n[0]) printf("###  "); 
	     else printf("%c%c%c  ", 
	                 (p->sq[i][j].n[1])?'.':'1', 
	                 (p->sq[i][j].n[2])?'.':'2', 
	                 (p->sq[i][j].n[3])?'.':'3'
	                );
	     printf("\n");

	  for (i = 0; i < G_COLS; i++) 
	     if (p->sq[i][j].n[0]) printf(" %c   ", p->sq[i][j].n[0]+'0'); 
  	     else printf("%c%c%c  ", 
	                 (p->sq[i][j].n[4])?'.':'4', 
	                 (p->sq[i][j].n[5])?'.':'5', 
	                 (p->sq[i][j].n[6])?'.':'6'
	                );
	     printf("\n");

	  for (i = 0; i < G_COLS; i++) 
             if (p->sq[i][j].n[0]) printf("###  "); 
	     else printf("%c%c%c  ", 
	                 (p->sq[i][j].n[7])?'.':'7', 
	                 (p->sq[i][j].n[8])?'.':'8', 
	                 (p->sq[i][j].n[9])?'.':'9'
	                );
	  printf("\n\n");
   }

} /* output_trial_grid */
