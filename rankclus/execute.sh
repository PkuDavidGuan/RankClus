#！/bin/bash
echo "build the program"
g++ -fopenmp rankClus.cpp -o rankClus -O2
echo "Outfiles\' Format: <iterationNum>_<rankMode>_<threadNum>.txt"
echo "1 thread..."
./rankClus 20 1 1 20_1_1.txt
echo "2 thread..."
./rankClus 20 1 2 20_1_2.txt
echo "4 thread..."
./rankClus 20 1 4 20_1_4.txt
echo "8 thread..."
./rankClus 20 1 4 20_1_8.txt
echo "16 thread..."
./rankClus 20 1 4 20_1_16.txt
rm rankClus
