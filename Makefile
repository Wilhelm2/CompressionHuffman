compress: compression.c commonFunctions.c
  gcc -o huffmanCompression  compression.c commonFunctions.c
decompress: decompression.c commonFunctions.c
  gcc -o huffmanDecompression decompression.c commonFunctions.c

run: 
  ./huffmanCompression test.txt
  ./huffmanDecompression test.txt_compressed 
