> Originally published on Jan 23, 2007 by Ilya O. Levin
> at http://www.literatecode.com

# Sudokuza: Sudoku puzzles solver.

Today I have found Sudokuza while revising my archives. This
program solves Sudoku puzzles. I sketched it more than a year
ago for fun to avoid Sudoku mania :)

My main interest in making Sudokuza was to create a program
that would solve a puzzle the similar way as a human would do,
without using hardcore math.

The algorithm is very simple and straightforward.

All cells on a grid hold variants from 0  to 9. A cell with
a single variant assumed as solved.

Sudokuza simply exclude solved cells’s number from variants
of remaining cells within a corresponding row, a column, and
a 3x3 block.

If there is nothing to exclude and the grid is not solved then
Sudokuza tries to proceed by assuming a remaining variant from
the first unsolved cell as correct answer. And so on, until it
solves the puzzle or gives up.

Sudokuza gives up on a cell with no variants left or on an
impossibly solved number.

Sudokuza is written in C. The source code is attached, together
with few sample puzzles.

I wrote Sudokuza primarily for personal entertainment, so it
does not have a user-friendly interface. It is only a
command-line utility that inputs puzzle from a text file
specified in parameter:

        sudokuza yourpuzzle.txt

The format of a puzzle file is simple: first nine characters
of the first nine lines represent a Sudoku grid. Take a look
at the provided samples, these are easy to understand.

Have fun :)
