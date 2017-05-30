#！/bin/bash
echo "build the program"
g++ -fopenmp v3.cpp -o v3 -O2
echo "Outfiles\' Format: <iterationNum>_<rankMode>_<threadNum>.txt"
echo "1 thread..."
./v3 20 1 1 20_1_1.txt
echo "2 thread..."
./v3 20 1 2 20_1_2.txt
echo "4 thread..."
./v3 20 1 4 20_1_4.txt
echo "8 thread..."
./v3 20 1 4 20_1_8.txt
echo "16 thread..."
./v3 20 1 4 20_1_16.txt
rm v3
