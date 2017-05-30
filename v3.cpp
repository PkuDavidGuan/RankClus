#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <iterator>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <cstdlib>
using namespace std;
#define confMaxNum 5000                   // maximum  number of conferences
#define authorMaxNum 21000                // maximum  number of authors
#define clusterNum 15                     // the number of clusters
#define rankNum 10                        // the number of iterations in the authority ranking
#define emNum 5                           // the number of iterations in the EM algorithm
int rankMode = 1;                         // 0 is simple ranking, 1 is authority ranking
int loopNum = 40;                         // the number of iterations
int threadNum = 2;                        // the number of threads
const double ALPHA = 0.95;                // a parameter in the authority ranking for authors

struct rankObject
{
    short id;
    float score;
}ro[authorMaxNum];                        // used for the final print

short adjXY[confMaxNum][authorMaxNum];    // Wxy mentioned in the paper
short adjYY[authorMaxNum][authorMaxNum];  // Wyy mentioned in the paper
int confEdgeNum[confMaxNum];              // the number of edges in the conference i
short paperNum[confMaxNum];               // the paper number per conference
int sumWxy = 0;                           // the sum of all items in Wxy

map<string, int> conf;                    // a hash table for conferences
map<int, string> reverseConf;
map<string, int> author;                  // a hash table for authors
map<int, string> reverseAuthor;
short authorNum = 0;                      // the number of authors
short confNum = 0;                        // the number of conferences

double rankX[clusterNum][confMaxNum];     // the conditional rank of authors in cluster k
double rankY[clusterNum][authorMaxNum];   // the conditional rank of conferences in cluster k
int clusterSumEdge[clusterNum];           // the sum of edge of Wxy in Cluster k
double tmpRx[confMaxNum];                 // save temporary within-cluster ranking of authors per iteration
double tmpRy[authorMaxNum];               // save temporary within-cluster ranking of conferences per iteration
set<int> confCluster[clusterNum];         // confCluster[i] contains all conferences belong to Cluster k

float PZ[clusterNum];                     // P(z=k)
double sumPZ[confMaxNum][authorMaxNum];   // used in the  E-step
float theta[confMaxNum][clusterNum];      // used in the cluster process, the position of conference i
float SX[clusterNum][clusterNum];         // used in the cluster process, get the position of cluster k

bool stopFlag = false;

/*
* split a line by a '$'
*/
void split(std::string& s,std::vector< std::string >* ret) 
{  
    if(s[s.size()-1] == 13)              // delete the last '\r'(in linux)
        s.erase(s.find_last_not_of('\r')+1);
    size_t last = 0;  
    string delim = "$";
    size_t index=s.find_first_of(delim,last);
    ret->clear(); 
    while (index!=std::string::npos)  
    {  
        ret->push_back(s.substr(last,index-last));  
        last=index+1;  
        index=s.find_first_of(delim,last);  
    }  
    if (index-last>0)  
    {  
        ret->push_back(s.substr(last,index-last));  
    }  
}
/*
* a compare function used in qsort
*/
int cmp ( const void *a , const void *b )
{
    float tmp = (*(rankObject *)a).score - (*(rankObject *)b).score;
    if(tmp < 0)
        return 1;
    else if(tmp == 0)
        return 0;
    else  
        return -1;
}
/*
* note down the time
*/
int timer(void)
{
  struct timeval tv;
  gettimeofday(&tv, (struct timezone*)0);
  return (tv.tv_sec*1000000+tv.tv_usec);
}
/*
* Initialization:
* assign each target object with a cluster
* label from 1 to K randomly, no empty cluster
*/
void Init()
{
    bool flag = true;
    int index;

    sumWxy = 0;
    memset(confEdgeNum, 0, sizeof(confEdgeNum));
    srand((unsigned)time(0));
    
    /*initiaze k clusters*/
    while(true)
    {
        flag = true;
        for(int k = 0; k < clusterNum; ++k)
            confCluster[k].clear();
        for(int i = 0; i < confNum; ++i)
        {
            index = rand() % clusterNum;
            confCluster[index].insert(i);
        }

        /* make sure every cluster has at least one member */
        for(int i = 0; i < clusterNum; ++i)
        {
            if(confCluster[i].empty())
            {
                flag = false;
                break;
            }
        }
        if(flag)
            break;
    }

    /* calculate the edge sum of each conference and their sum */
    int tmp;
    for(int i = 0; i < confNum; ++i)
    {
        tmp = 0;
        for(int j = 0; j < authorNum; ++j)
            tmp += adjXY[i][j];
        sumWxy += tmp;
        confEdgeNum[i] = tmp;
    }
}
/*
* the last step of rank function, calculate rank(X|X')
*/
void withinClusterRank()
{ 
    /* calculate within-cluster rank of each conference */
    double sum[clusterNum];
    for(int k = 0; k < clusterNum; ++k)
    {
        sum[k] = 0;
        for(int i = 0; i < confNum; ++i)
        {
            for(int j = 0; j < authorNum; ++j)
            {
                sum[k] += adjXY[i][j]*rankY[k][j];
            }
        }
    }
    for(int k = 0; k < clusterNum; ++k)
    {
        for(int i = 0; i < confNum; ++i)
        {
            for(int j = 0; j < authorNum; ++j)
            {
                rankX[k][i] += adjXY[i][j]*rankY[k][j];
            }
            rankX[k][i] /= sum[k];
        }
    }
    return;
}
/*
* authority ranking
*/
void  authorityRank()
{
    omp_set_num_threads(threadNum);
    #pragma omp parallel for
    for(int k = 0; k < clusterNum; ++k)
    {
        float sum;
        float tmp;
        set<int>::iterator iter;
        float tmpRx[confMaxNum];
        float tmpRy[authorMaxNum];
        memset(tmpRx, 0, sizeof(tmpRx));
        memset(tmpRy, 0, sizeof(tmpRy));

        for(int t = 0; t < rankNum; ++t)
        {
            /* restore the result of last iteration */
            for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
                tmpRx[*iter] = rankX[k][*iter];
            for(int j = 0; j < authorNum; ++j)
                tmpRy[j] = rankY[k][j];

            /* calculate the conditional rank of conferences over Cluster k*/
            sum = 0;
            for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
            {
                tmp = 0;
                for(int j = 0; j < authorNum; ++j)
                    tmp += adjXY[*iter][j]*tmpRy[j];
                sum += tmp;
                rankX[k][*iter] = tmp;   
            }
            /* normalize */
            for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
                rankX[k][*iter] /= sum;
            
            /* calculate the conditional rank of authors over Cluster k*/
            sum = 0;
            for(int j = 0; j < authorNum; ++j)
            {
                tmp = 0;
                for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
                    tmp += adjXY[*iter][j] * tmpRx[*iter];
                rankY[k][j] = tmp*ALPHA;

                tmp = 0;
                for(int i = 0; i < authorNum; ++i)
                    tmp += adjYY[j][i]*tmpRy[i];
                rankY[k][j] += tmp*(1-ALPHA);
                sum += rankY[k][j];
            }
            /* normalize */
            for(int j = 0; j < authorNum; ++j)
                rankY[k][j] /= sum;
        }
    }
}
void Rank(int mode)
{
    /* the following code is simple rank function */
    memset(rankX, 0, sizeof(rankX));
    memset(rankY, 0, sizeof(rankY));

    set<int>::iterator iter;
    
    /* calculate clusterSumEdge */
    for(int k = 0; k < clusterNum; ++k)
    {
        clusterSumEdge[k] = 0;
        for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
        {
            clusterSumEdge[k] += confEdgeNum[*iter];
        }
    }

    /* calculate conditional rank of each conference over Cluster k */
    for(int k = 0; k < clusterNum; ++k)
    {
        for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
            rankX[k][*iter] /= confEdgeNum[*iter]*1.0/clusterSumEdge[k];
    }

    /* calculate conditional rank of each author over Cluster k*/
    for(int k = 0; k < clusterNum; ++k)
    {
        for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
        {
            for(int j = 0; j < authorNum; ++j)
                rankY[k][j] += adjXY[*iter][j];
        }
        for(int j = 0; j < authorNum; ++j)
            rankY[k][j] /= clusterSumEdge[k];
    }

    /* simple rank is the initiation of authority rank */
    if(mode == 1)
        authorityRank();
    
    withinClusterRank();
}

/*
* EM algorithm
* The array PZ is p(z = k).
*/
void EM()
{
    double sum = 0;
    double tmp = 0;

    /*calculate p0(z = k)*/
    for(int k = 0; k < clusterNum; ++k)
        PZ[k] = clusterSumEdge[k]*1.0/sumWxy;

    sum = 0;
    cout << "P0(z=k): ";
    for(int i = 0; i < clusterNum; ++i)
    {
        sum += PZ[i];
        cout << PZ[i] << ' ';
    }
    cout << endl << "Sum: " << sum << endl;
    
    for(int t = 0; t < emNum; ++t)
    {
        /* E-step */
        for(int i = 0; i < confNum; ++i)
        {
            for(int j = 0; j < authorNum; ++j)
            {
                tmp = 0;
                for(int l = 0; l < clusterNum; ++l)
                    tmp += rankX[l][i]*rankY[l][j]*PZ[l];
                sumPZ[i][j] = tmp;
            }
        }

        /* M-step */
        for(int k = 0; k < clusterNum; ++k)
        {
            tmp = 0;
            for(int i = 0; i < confNum; ++i)
            {
                for(int j = 0; j < authorNum; ++j)
                {
                    if(sumPZ[i][j] != 0)
                        tmp += adjXY[i][j]*(rankX[k][i]*rankY[k][j]*PZ[k]/sumPZ[i][j]);
                }
            }
            PZ[k] = tmp/sumWxy;
        }
        sum = 0;
        cout << "P(z=k): ";
        for(int i = 0; i < clusterNum; ++i)
        {
            sum += PZ[i];
            cout << PZ[i] << ' ';
        }
        cout << endl << "Sum: " << sum << endl;
    }

    /*calculate the mathix *theta* */
    for(int i = 0; i < confNum; ++i)
    {
        sum = 0;
        for(int l = 0; l < clusterNum; ++l)
        {
            sum += rankX[l][i]*PZ[l];
        }
        for(int k = 0; k < clusterNum; ++k)
        {
            theta[i][k] = rankX[k][i]*PZ[k]/sum;
        }
    }
    return;
}

/*
* re-cluster
*/
void Cluster()
{
    set<int>::iterator iter;
    /* calculate the matrix sX */
    memset(SX, 0, sizeof(SX));
    for(int k = 0; k < clusterNum; ++k)
    {
        for(iter = confCluster[k].begin(); iter != confCluster[k].end(); ++iter)
        {
            for(int l = 0; l < clusterNum; ++l)
                SX[k][l] += theta[*iter][l];
        }
        for(int l = 0; l < clusterNum; ++l)
            SX[k][l] /= confCluster[k].size();
    }

    /* calculate the distance and recluster*/
    int index = 0;
    double minDis = 1;
    double tmp1,tmp2;

    double center[clusterNum];
    for(int k = 0; k < clusterNum; ++k)
    {
        tmp1 = 0;
        for(int l = 0; l < clusterNum; ++l)
            tmp1 += SX[k][l]*SX[k][l];
        center[k] = sqrt(tmp1);
    }

    /* re-cluster */
    for(int k = 0; k < clusterNum; ++k)
        confCluster[k].clear();
    
    for(int i = 0; i < confNum; ++i)     
    {
        minDis = 1;
        index = 0;

        for(int k = 0; k < clusterNum; ++k)
        {
            tmp1 = 0;
            for(int l = 0; l < clusterNum; ++l)
                tmp1 += theta[i][l]*SX[k][l];

            tmp2 = 0;
            for(int l = 0; l < clusterNum; ++l)
                tmp2 += theta[i][l]*theta[i][l];
            
            if(minDis > (1 - (tmp1/sqrt(tmp2)/center[k])))
            {
                minDis = 1 - (tmp1/sqrt(tmp2)/center[k]);
                index = k;
            }
        }
        confCluster[index].insert(i);
    }

    /* a really low probablly situation mentioned in the paper */
    for(int k = 0; k < clusterNum; ++k)
    {
        if(confCluster[k].empty())
        {
            cout << "Sorry, but you have to run the program again." << endl;
            exit(1);
        }
    }
    return;
}
int main(int argc, char *argv[])
{
    if(argc != 5)
    {
        cout << "Format: <iterationNum> <rankMode> <threadNum> <outfile>";
        return 0;
    }
    loopNum = atoi(argv[1]);
    rankMode = atoi(argv[2]);
    threadNum = atoi(argv[3]);
    ifstream infile;
    infile.open("dblp_result.txt");

    if(!infile)
    {
        cout << "error" << endl;
        return 0;
    }

    string line;
    vector<string> content;

    conf.clear();
    author.clear();
    memset(adjXY, 0, sizeof(adjXY));
    memset(adjYY, 0, sizeof(adjYY));
    confNum = authorNum = 0;
    while(getline(infile, line))
    {
        split(line, &content);
        if(conf.find(content[0]) == conf.end())
        {
            reverseConf[confNum] = content[0];
            conf[content[0]] = confNum++;
        }
        for(int i = 1; i < content.size(); ++i)
        {
            if(author.find(content[i]) == author.end())
            {
                reverseAuthor[authorNum] = content[i];
                author[content[i]] = authorNum++;
            }
            adjXY[conf[content[0]]][author[content[i]]]++;
            for(int j = 1; j < i; ++j)
            {
                adjYY[author[content[i]]][author[content[j]]]++;
                adjYY[author[content[j]]][author[content[i]]]++;
            }
        }
    }
    cout << "conference number: " << conf.size() << " author number: " << author.size() << endl;
    infile.close();

    int time1, time2, time3, time4;
    float interval1,interval2,interval3;
    interval1 = interval2 = interval3 = 0;
    /*initiation*/
    Init();
    for(int i = 0; i < loopNum; ++i)
    {
        cout << "----------Loop:" << i << "----------" << endl;
        cout << "Start Ranking..." << endl;
        time1 = timer();
        Rank(rankMode);
        time2 = timer();
        cout << "Start EM..." << endl;
        EM();
        time3 = timer();
        cout << "Clustering..." << endl;
        Cluster();
        time4 = timer();
        interval1 += time2 - time1;
        interval2 += time3 - time2;
        interval3 += time4 - time3;
    }

    /*print the result*/
    ofstream outfile(argv[4]);
    outfile << interval1/loopNum << ' ' << interval2/loopNum << ' ' << interval3/loopNum << endl;
    std::map<string,int>::iterator iter;
    std::set<int>::iterator confIter;
    int count;
    double tmp;

    Rank(rankMode);
    outfile << "----------Result----------" << endl;
    for(int k = 0; k < clusterNum; ++k)
    {
        memset(ro, 0, sizeof(ro));
        count = 0;
        for(confIter = confCluster[k].begin(); confIter != confCluster[k].end(); ++confIter)
        {
            ro[count].id = *confIter;
            ro[count++].score = rankX[k][*confIter];
        }
        qsort(ro,count,sizeof(rankObject),cmp);
        outfile << endl << "######Cluster: " << k << "######" << endl;
        for(int i = 0; i < 10; ++i)
        {
            outfile << reverseConf[ro[i].id] << endl;
        }

        outfile << "-.-.-.-.-.-.-.-.-.-.-.-.-.-" << endl;
        
        memset(ro, 0, sizeof(ro));
        for(int i = 0; i < authorNum; ++i)
        {
            ro[i].id = i;
            ro[i].score = rankY[k][i];
        }
        qsort(ro,authorNum,sizeof(rankObject),cmp);
        for(int i = 0; i < 10; ++i)
        {
            outfile << reverseAuthor[ro[i].id] << endl;
        }
    }
    outfile.close();
    return 0;
}