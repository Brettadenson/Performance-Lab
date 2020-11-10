#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int
main(int argc, char **argv)
{

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

class Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(class Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  short inputWidth = input -> width-1;
  output -> width = inputWidth;
  short inputHeight = input -> height-1;
  output -> height = inputHeight;

  short filterDivisor = filter -> getDivisor(); //code motion from for loop
  short pixelOutput;
  int pixelGet[9] = {
    filter -> get(0,0),filter -> get(0,1),filter -> get(0,2),
    filter -> get(1,0),filter -> get(1,1),filter -> get(1,2),
    filter -> get(2,0),filter -> get(2,1),filter -> get(2,2)
  };

      for(int plane = 0; plane < 3; plane++) {
        for(int row = 1; row < (inputHeight); row = row + 1) {
            for(int col = 1; col < (inputWidth); col = col + 1) {
              pixelOutput = 0;
              short firstRow = row-1;
              short secondRow = row;
              short thirdRow = row+1;
              short firstColumn = col-1;
              short secondColumn = col;
              short thirdColumn = col+1;
              int inputGrid[9] = {
                input -> color[plane][firstRow][firstColumn] * pixelGet[0],
                input -> color[plane][firstRow][secondColumn] * pixelGet[1],
                input -> color[plane][firstRow][thirdColumn] * pixelGet[2],
                input -> color[plane][secondRow][firstColumn] * pixelGet[3],
                input -> color[plane][secondRow][secondColumn] * pixelGet[4],
                input -> color[plane][secondRow][thirdColumn] * pixelGet[5],
                input -> color[plane][thirdRow][firstColumn] * pixelGet[6],
                input -> color[plane][thirdRow][secondColumn] * pixelGet[7],
                input -> color[plane][thirdRow][thirdColumn] * pixelGet[8],
              };
              pixelOutput += inputGrid[0];
              pixelOutput += inputGrid[1];
              pixelOutput += inputGrid[2];
              pixelOutput += inputGrid[3];
              pixelOutput += inputGrid[4];
              pixelOutput += inputGrid[5];
              pixelOutput += inputGrid[6];
              pixelOutput += inputGrid[7];
              pixelOutput += inputGrid[8];

              pixelOutput = pixelOutput / filterDivisor;
              pixelOutput = (pixelOutput < 0) ? 0 : pixelOutput;
              pixelOutput = (pixelOutput > 255) ? 255 : pixelOutput;
              output -> color[plane][row][col] = pixelOutput;
            }
          }
        }

   


    
  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
