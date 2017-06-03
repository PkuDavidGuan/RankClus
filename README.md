# RankClus

A Data Mining Algorithm in the Heterogeneous Information Network

## Files
* rankclus

    - parse1.py。对dblp.xml进行预处理，得到dblp\_tmp\_result.txt。

    - parse2.py。对dblp\_tmp\_result.txt进一步处理，得到实验所用数据集。

    - rankClus.cpp。算法主程序，可以选择rank function，迭代次数和并行线程数。

    - execute.sh。可以在linux下直接运行此程序。

    - dblp\_tmp\_result.txt。存储着1998到2007年的dblp记录的所有会议论文。

    - dblp\_result.txt。最终用于实验的数据集。
    - outfiles. 存储着不同迭代次数的实验结果。

* 其它为草稿文件，用于编写过程中的调试

## Usage

```console
$ g++ -fopenmp v3.cpp -o RankClus -O2
$ ./RankClus <iterationNum> <rank mode> <threadNum> <outfile>
```

## [Acknowledgement]
The algorithm was based on the paper:`RankClus:integrating clustering with ranking for heterogeneous information network analysis`.
