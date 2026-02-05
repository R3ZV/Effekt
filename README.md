# Info

The impulse response used was from a Marshall 1960 cabinet with G12M-25 speakers,
recorded with an SM57 positioned at the Cap Edge at a distance of 0.5 inches.

# Build locally

To run the formatter: `cmake --build build --target format`

## Windows

- `cmake -B build; cmake --build build; ./build/AudioProcessor.exe`
  
# Project Structure

- `docs`: Contains the paper about this project (both .tex and .pdf)  

- `output`: Contains the different results from the filters. The subdirectory structure is usually like: `output/{filter_name}/{audio_name}/{params}/`. And in the final subdirectory the resulted audio and it's spectograms will be found. `output/combination/{audio_name}/` contains the result of applying all of the filters.  

- `samples`: The audio used as input to test the filters  

- `showcase`: Similar to output but contains combinations of filter.  

- `src`: Where the source and header files of the project reside.

- `visualize.py`: Script used to generate spectograms.

# Reference

- https://redwirez.com/pages/the-marshall-1960a-ir-pack
- https://people.montefiore.uliege.be/josmalskyj/files/Gardner1995Efficient.pdf
- https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.1.0.pdf
- Spatial Audio by Francis Rumsey
- https://ccrma.stanford.edu/~dtyeh/papers/yeh10_taslp.pdf  
- https://www.musicdsp.org/en/latest/Analysis/97-envelope-detector.html
- https://www.uncini.com/dida/tsa/mod_tsa/Chap_08.pdf
  