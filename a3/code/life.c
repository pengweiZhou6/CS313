#include <stdio.h>
#include "life.h"

// You are only allowed to change the contents of this file with respect 
// to improving the performance of this program



// If you need to initialize some data structures for your implementation
// of life then make changes to the following function, otherwise 
// do not change this function.

void initLife() {
}



// When completed this file will contain several versions of the life() function and 
// conditional compilation will be used to determine which version runs.  Each 
// version will be surrounded with  an ifdef, undef, and endif as illustrated 
// immediately below this comment block. To select the version of the life function 
// to use, put the #define USE USE immediately infront of the version of the life() 
// to compile.  Your version of life() must produce a correct result. 

// You are required to document the changes you make in the README.txt file. For each entry
// in the README.txt file there is to be a version of the matching  life() function here such
// that the markers can see, and run if needed,  the actual code that you used 
// to produce your results for the logged change to the life function.  


#ifdef USE
int life(long oldWorld[N][N], long newWorld[N][N]) {
  return 0;
}
#undef USE
#endif






// For each version of life you are testing duplicate the code between the 
// ifdef and endif and make your changes. To use the new version, move the the #define 
// just about this comment to be just in front of the ifdef for the new version.

#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  
  //clear the new world
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] =  0;
    }

  int col, row;
  int q = -1;


  // Count the cells to the top left
  for (i = 0; i < N; i++) {
    col = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
      row = (j - 1 + N ) % N;
      newWorld[j][i] += oldWorld[row][col];
    }
  }
  
  // Count the cells immediately above
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      row = (j - 1 + N ) % N;
      newWorld[j][i] += oldWorld[row][i];
    }
  }

  // Count the cells to the top right
  for (i = 0; i < N; i++) {
    col = (i + 1 + N) % N;
    for (j = 0; j < N; j++) {
      row = (j - 1  + N ) % N;
      newWorld[j][i] += oldWorld[row][col];
    }
  }

 // Count the cells to the immediate left
  for (i = 0; i < N; i++) {
    col = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
       newWorld[j][i] += oldWorld[j][col];
    }
  }
  
  // Count the cells to the immediate right
  for (i = 0; i < N; i++) {
    col = (i + 1 + N) % N;
    for (j = 0; j < N; j++) {
      newWorld[j][i] += oldWorld[j][col];
    }
  }

// Count the cells to the bottom left
  for (i = 0; i < N; i++) {
    col = (i - 1 + N) % N;
    for (j = 0; j < N; j++) {
      row = (j + 1 + N ) % N;
      newWorld[j][i] += oldWorld[row][col];
    }
  }

  // Count the cells immediately below
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      row = (j + 1  + N ) % N;
      newWorld[j][i] += oldWorld[row][i];
    }
  }

  // Count the cells to the bottom right
  for (i = 0; i < N; i++) {
    col = (i + 1 + N) % N;
    for (j = 0; j < N; j++) {
      row = (j + 1  + N ) % N;
      newWorld[j][i] += oldWorld[row][col];
    }
  }

  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif




// log2 change to row-major access by switching col and row, switch i and j 
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  
  //clear the new world
  //change here
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[i][j] =  0;
    }

  int col, row;
  int q = -1;


  // Count the cells to the top left
  // change here
  for (i = 0; i < N; i++) {
    row = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N ) % N;
      newWorld[i][j] += oldWorld[row][col];
    }
  }
  
  // Count the cells immediately above
    // change here
  for (i = 0; i < N; i++) {
      row = (i - 1 + N ) % N;
    for (j = 0; j < N; j++) {
      newWorld[i][j] += oldWorld[row][j];
    }
  }

  // Count the cells to the top right
    // change here
  for (i = 0; i < N; i++) {
    row = (i - 1 + N) % N;
    for (j = 0; j < N; j++) {
      col = (j + 1  + N ) % N;
      newWorld[i][j] += oldWorld[row][col];
    }
  }

 // Count the cells to the immediate left
 // change here
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
          col = (j -1 + N) % N;
       newWorld[i][j] += oldWorld[i][col];
    }
  }
  
  // Count the cells to the immediate right
   // change here
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
          col = (j + 1 + N) % N;
      newWorld[i][j] += oldWorld[i][col];
    }
  }

// Count the cells to the bottom left
// change here
  for (i = 0; i < N; i++) {
    row = (i + 1 + N ) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N) % N;
      newWorld[i][j] += oldWorld[row][col];
    }
  }

  // Count the cells immediately below
  // change here
  for (i = 0; i < N; i++) {
    row = (i + 1  + N ) % N;
    for (j = 0; j < N; j++) {
      newWorld[i][j] += oldWorld[row][j];
    }
  }

  // Count the cells to the bottom right
  // change here
  for (i = 0; i < N; i++) {
    row = (i + 1  + N ) % N;
    for (j = 0; j < N; j++) {
        col = (j + 1 + N) % N;  
      newWorld[i][j] += oldWorld[row][col];
    }
  }

  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif



// log3 merge 9 loops into 3 loops to saving repeating read
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  
  //clear the new world
  //change here
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[i][j] =  0;
    }

  int col, row, colR, rowR;
  int q = -1;


  // Count the cells to the top left
  // change here
  for (i = 0; i < N; i++) {
    row = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N ) % N;
      colR = (j + 1  + N ) % N;
      newWorld[i][j] += oldWorld[row][col];
      newWorld[i][j] += oldWorld[row][j];
      newWorld[i][j] += oldWorld[row][colR];
    }
  }
  


 // Count the cells to the immediate left
 // change here
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
          col = (j -1 + N) % N;
          colR = (j + 1 + N) % N;
      newWorld[i][j] += oldWorld[i][col];
       newWorld[i][j] += oldWorld[i][colR];
    }
  }
  


// Count the cells to the bottom left
// change here
  for (i = 0; i < N; i++) {
    row = (i + 1 + N ) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N) % N;
              colR = (j + 1 + N) % N;  
      newWorld[i][j] += oldWorld[row][colR];
      newWorld[i][j] += oldWorld[row][col];
            newWorld[i][j] += oldWorld[row][j];
    }
  }



  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif




// log4 merge write into one and save write time
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  
  //clear the new world
  //change here
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[i][j] =  0;
    }

  int col, row, colR, rowR;
  int q = -1;


  // Count the cells to the top left
  // change here
  for (i = 0; i < N; i++) {
    row = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N ) % N;
      colR = (j + 1  + N ) % N;
      newWorld[i][j] += oldWorld[row][col] +oldWorld[row][j] +oldWorld[row][colR];
    }
  }
  


 // Count the cells to the immediate left
 // change here
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
          col = (j -1 + N) % N;
          colR = (j + 1 + N) % N;
      newWorld[i][j] += oldWorld[i][col] + oldWorld[i][colR];
    }
  }
  


  // Count the cells to the bottom left
  // change here
    for (i = 0; i < N; i++) {
      row = (i + 1 + N ) % N;
      for (j = 0; j < N; j++) {
        col = (j - 1 + N) % N;
                colR = (j + 1 + N) % N;  
        newWorld[i][j] += oldWorld[row][colR] + oldWorld[row][col] + oldWorld[row][j];
      }
  }



  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif



// log5 remove initializing new life by just save to it instead of add to it
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  

  int col, row, colR, rowR;
  int q = -1;


  // Count the cells to the top left
  // change here
  for (i = 0; i < N; i++) {
    row = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
      col = (j - 1 + N ) % N;
      colR = (j + 1  + N ) % N;
      newWorld[i][j] = oldWorld[row][col] +oldWorld[row][j] +oldWorld[row][colR];
    }
  }
  


 // Count the cells to the immediate left

  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
          col = (j -1 + N) % N;
          colR = (j + 1 + N) % N;
      newWorld[i][j] += oldWorld[i][col] + oldWorld[i][colR];
    }
  }
  


  // Count the cells to the bottom left

    for (i = 0; i < N; i++) {
      row = (i + 1 + N ) % N;
      for (j = 0; j < N; j++) {
        col = (j - 1 + N) % N;
        colR = (j + 1 + N) % N;  
        newWorld[i][j] += oldWorld[row][colR] + oldWorld[row][col] + oldWorld[row][j];
      }
  }



  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif



// log6 merge 3 loops into 1
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  

  int col, row, colR, rowR;
  int q = -1;



  // Count the cells to the bottom left

    for (i = 0; i < N; i++) {
      row = (i + 1 + N ) % N;
      rowR = (i -1 + N) % N;
      for (j = 0; j < N; j++) {
        col = (j - 1 + N) % N;
        colR = (j + 1 + N) % N;  
        newWorld[i][j] = oldWorld[rowR][col] +oldWorld[rowR][j] +oldWorld[rowR][colR] +oldWorld[i][col] + oldWorld[i][colR] + oldWorld[row][colR] + oldWorld[row][col] + oldWorld[row][j];
      }
  }



  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++) {
      newWorld[j][i] = checkHealth(newWorld[j][i], oldWorld[j][i]);
      alive += newWorld[j][i] ? 1:0;
    }

  return alive;
}
#undef USE
#endif

#define USE
// log7 merge into last loop
#ifdef USE

int life(long oldWorld[N][N], long newWorld[N][N]) {

  int i, j, k, l;
  

  int col, row, colR, rowR;
  int q = -1;


  // Check each cell to see if it should come to life, continue to live, or die
  int alive = 0;
  
  for (i = 0; i < N; i++){
      row = (i + 1 + N ) % N;
      rowR = (i -1 + N) % N;
    for (j = 0; j < N; j++) {
        col = (j - 1 + N) % N;
        colR = (j + 1 + N) % N;  
        
      alive += checkHealth((oldWorld[rowR][col] +oldWorld[rowR][j] +oldWorld[rowR][colR] +oldWorld[i][col] + oldWorld[i][colR] + oldWorld[row][colR] + oldWorld[row][col] + oldWorld[row][j]), oldWorld[i][j]) ? 1:0;
      }
  }
  return alive;
}
#undef USE
#endif