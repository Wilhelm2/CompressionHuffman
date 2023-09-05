gcc -o compress  compression.c commonFunctions.c
gcc -o decompress decompression.c commonFunctions.c

./compress test.txt
./decompress test.txt_compressed 
